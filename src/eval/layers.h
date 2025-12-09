#include "../consts.h"
#include "arch.h"
#include <cmath>
#include <immintrin.h>
#include <string.h>
#pragma once

template <typename T> T crelu(T x, T Q);
I32 csqr(I32 x, I32 Q);

template <int inputsize, int outputsize> struct SparseAffineWeights {
  alignas(64) I8 weights[outputbuckets * inputsize * outputsize];
  alignas(64) I32 bias[outputbuckets * outputsize];
  static constexpr int size = outputbuckets * outputsize * (inputsize + 4);
  void load(const char *stream) {
    if (!__builtin_cpu_supports("avxvnni")) {
      load_scalar(stream);
      return;
    }
    int offset = 0;
    for (int b = 0; b < outputbuckets; b++) {
      for (int k = 0; k < inputsize; k += 4) {
        for (int j = 0; j < outputsize; j++) {
          for (int i = 0; i < 4; i++) {
            int dest_idx = outputsize * k + 4 * j + i;
            int src_idx = j * inputsize + (k + i);
            weights[offset + dest_idx] = stream[offset + src_idx];
          }
        }
      }
      offset += inputsize * outputsize;
    }
    memcpy(bias, stream + offset, 4 * outputbuckets * outputsize);
  }
  void load_scalar(const char *stream) {
    int offset = 0;
    memcpy(weights, stream + offset, outputbuckets * inputsize * outputsize);
    offset += outputbuckets * inputsize * outputsize;
    memcpy(bias, stream + offset, 4 * outputbuckets * outputsize);
  }
};

template <int inputsize, int outputsize> struct SparseAffine {
  static void
  transform(const U8 *input, I32 *output,
            const SparseAffineWeights<inputsize, outputsize> *weights,
            int bucket) {
    if (!__builtin_cpu_supports("avxvnni")) {
      transform_scalar(input, output, weights, bucket);
      return;
    }
    for (int i = 0; i < outputsize; i++) {
      output[i] = weights->bias[bucket * outputsize + i];
    }
    int offset = bucket * inputsize * outputsize;
    const __m256i *weightptr = (const __m256i *)(&(weights->weights[offset]));
    // Use two accumulators to break dependency chains
    __m256i outvec1 = _mm256_loadu_si256((__m256i *)output);
    __m256i outvec2 = _mm256_setzero_si256();

    // Process 8 steps of K per iteration (2 blocks of 4)
    for (int k = 0; k < inputsize / 4; k += 2) {
      // --- Block 1 (k) ---
      __m256i w1 = _mm256_load_si256((__m256i *)(weightptr + k));
      __m256i in1 = _mm256_set1_epi32(*(const int32_t *)&input[4 * k]);
      outvec1 = _mm256_dpbusd_epi32(outvec1, in1, w1);

      // --- Block 2 (k+1) ---
      // This can execute while Block 1 is still calculating!
      __m256i w2 = _mm256_load_si256((__m256i *)(weightptr + k + 1));
      __m256i in2 = _mm256_set1_epi32(*(const int32_t *)&input[4 * (k + 1)]);
      outvec2 = _mm256_dpbusd_epi32(outvec2, in2, w2);
    }

    // Sum the two accumulators at the end
    outvec1 = _mm256_add_epi32(outvec1, outvec2);
    _mm256_storeu_si256((__m256i *)output, outvec1);
  }
  static void
  transform_scalar(const U8 *input, I32 *output,
                   const SparseAffineWeights<inputsize, outputsize> *weights,
                   int bucket) {
    for (int i = 0; i < outputsize; i++) {
      output[i] = weights->bias[bucket * outputsize + i];
    }
    for (int i = 0; i < inputsize; i++) {
      if (input[i]) {
        const I8 *vector =
            &(weights->weights[bucket * inputsize * outputsize + i]);
        const int factor = input[i];
        for (int j = 0; j < outputsize; j++) {
          output[j] += vector[j * inputsize] * factor;
        }
      }
    }
  }
};

template <int inputsize> struct VectorAdd {
  static void transform(const I32 *input, I32 *output) {
    for (int i = 0; i < inputsize; i++) {
      output[i] += input[i];
    }
  }
};

template <int inputsize, int outputsize> struct DenseAffineWeights {
  alignas(64) I32 weights[outputbuckets * outputsize * inputsize];
  alignas(64) I32 bias[outputbuckets * outputsize];
  static constexpr int size = 4 * outputbuckets * outputsize * (inputsize + 1);
  void load(const char *stream) {
    int offset = 0;
    memcpy(weights, stream + offset,
           4 * outputbuckets * inputsize * outputsize);
    offset += 4 * outputbuckets * inputsize * outputsize;
    memcpy(bias, stream + offset, 4 * outputbuckets * outputsize);
  }
};

template <int inputsize, int outputsize> struct DenseAffine {
  static void
  transform(const I32 *input, I32 *output,
            const DenseAffineWeights<inputsize, outputsize> *weights,
            int bucket) {
    for (int i = 0; i < outputsize; i++) {
      output[i] = weights->bias[bucket * outputsize + i];
    }
    for (int j = 0; j < outputsize; j++) {
      const I32 *vector =
          &(weights->weights[bucket * inputsize * outputsize + j * inputsize]);
      for (int i = 0; i < inputsize; i++) {
        output[j] += vector[i] * input[i];
      }
    }
  }
};

template <int inputsize> struct PerspectiveWeights {
  alignas(64) I16 weights[2 * outputbuckets * inputsize];
  alignas(64) I16 bias[outputbuckets];
  static constexpr int size = outputbuckets * (4 * inputsize + 2);
  void load(const char *stream) {
    int offset = 0;
    memcpy(weights, stream + offset, 4 * outputbuckets * inputsize);
    offset += 4 * outputbuckets * inputsize;
    memcpy(bias, stream + offset, 2 * outputbuckets);
  }
};

struct PerspectivePairwise {
  static void transform(const I16 *input, U8 *output, int color) {
    for (int i = 0; i < L1size / 2; i++) {
      output[i] = crelu<I16>(input[color * L1size + i], L1Q) *
                  crelu<I16>(input[color * L1size + L1size / 2 + i], L1Q) /
                  (1 << pairwiseshiftbits);
      output[L1size / 2 + i] =
          crelu<I16>(input[(color ^ 1) * L1size + i], L1Q) *
          crelu<I16>(input[(color ^ 1) * L1size + L1size / 2 + i], L1Q) /
          (1 << pairwiseshiftbits);
    }
  }
};

template <int inputsize> struct CReLUActivation {
  static void transform(const I32 *input, I32 *output, I32 Q) {
    for (int i = 0; i < inputsize; i++) {
      output[i] = crelu<I32>(input[i], Q);
    }
  }
};

template <int inputsize> struct DualActivation {
  static void transform(const I32 *input, I32 *output, I32 Q) {
    for (int i = 0; i < inputsize; i++) {
      I32 *creluoutput = output;
      I32 *csqroutput = &(output[inputsize]);
      creluoutput[i] = Q * crelu<I32>(input[i], Q);
      csqroutput[i] = csqr(input[i], Q);
    }
  }
};

template <int inputsize, int bits> struct DivideShift {
  static void transform(I32 *input) {
    for (int i = 0; i < inputsize; i++) {
      input[i] >>= bits;
    }
  }
};

template <int inputsize> struct PerspectiveSCReLU {
  static I32 transform(const I16 *stminput, const I16 *nstminput,
                       const I16 *stmweights, const I16 *nstmweights, I16 bias,
                       I16 Q) {
    int eval = 0;
    for (int i = 0; i < inputsize; i++) {
      I16 stmclipped = crelu<I16>(stminput[i], Q);
      I16 nstmclipped = crelu<I16>(nstminput[i], Q);
      eval += I16(stmclipped * stmweights[i]) * stmclipped;
      eval += I16(nstmclipped * nstmweights[i]) * nstmclipped;
    }
    eval /= Q;
    eval += bias;
    return eval;
  }
};

template <int inputsize> struct PerspectiveCReLU {
  static I32 transform(const I16 *stminput, const I16 *nstminput,
                       const I16 *stmweights, const I16 *nstmweights, I16 bias,
                       I16 Q) {
    int eval = bias;
    for (int i = 0; i < inputsize; i++) {
      I16 stmclipped = crelu<I16>(stminput[i], Q);
      I16 nstmclipped = crelu<I16>(nstminput[i], Q);
      eval += I16(stmclipped * stmweights[i]);
      eval += I16(nstmclipped * nstmweights[i]);
    }
    return eval;
  }
};

#ifdef SINGLE_LAYER_CRELU
using SingleLayerAffine = PerspectiveCReLU<L1size>;
#else
using SingleLayerAffine = PerspectiveSCReLU<L1size>;
#endif