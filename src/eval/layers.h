#include "../consts.h"
#include "affine.h"
#include "arch.h"
#include <cmath>
#pragma once

template <typename T> T crelu(T x, T Q);
I32 csqr(I32 x, I32 Q);

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
    int offset = 4 * outputbuckets * inputsize * outputsize;
    memcpy(weights, stream, 4 * outputbuckets * inputsize * outputsize);
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

struct PerspectiveTransform {
  static void pairwise_avx2(const I16 *__restrict input, U8 *__restrict output, int color) {
    constexpr int halfL1 = L1size / 2;
    const __m256i v_one  = _mm256_set1_epi16(L1Q);
    const __m256i v_zero = _mm256_setzero_si256();
    const int16_t* in0_lo = &input[color * L1size];
    const int16_t* in0_hi = &input[color * L1size + halfL1];
    const int16_t* in1_lo = &input[(color ^ 1) * L1size];
    const int16_t* in1_hi = &input[(color ^ 1) * L1size + halfL1];
    for (int i = 0; i < halfL1; i += 32) {
        __m256i a0 = _mm256_load_si256((const __m256i*)&in0_lo[i]);
        __m256i b0 = _mm256_load_si256((const __m256i*)&in0_hi[i]);
        __m256i c0 = _mm256_load_si256((const __m256i*)&in0_lo[i + 16]);
        __m256i d0 = _mm256_load_si256((const __m256i*)&in0_hi[i + 16]);
        a0 = _mm256_max_epi16(_mm256_min_epi16(a0, v_one), v_zero);
        c0 = _mm256_max_epi16(_mm256_min_epi16(c0, v_one), v_zero);
        b0 = _mm256_min_epi16(b0, v_one);
        d0 = _mm256_min_epi16(d0, v_one);
        a0 = _mm256_slli_epi16(a0, 16 - l1shiftbits);
        c0 = _mm256_slli_epi16(c0, 16 - l1shiftbits);
        __m256i res0_p1 = _mm256_mulhi_epi16(a0, b0);
        __m256i res0_p2 = _mm256_mulhi_epi16(c0, d0);
        __m256i p0 = _mm256_packus_epi16(res0_p1, res0_p2);
        _mm256_store_si256((__m256i*)&output[i], p0);
        __m256i a1 = _mm256_load_si256((const __m256i*)&in1_lo[i]);
        __m256i b1 = _mm256_load_si256((const __m256i*)&in1_hi[i]);
        __m256i c1 = _mm256_load_si256((const __m256i*)&in1_lo[i + 16]);
        __m256i d1 = _mm256_load_si256((const __m256i*)&in1_hi[i + 16]);
        a1 = _mm256_max_epi16(_mm256_min_epi16(a1, v_one), v_zero);
        c1 = _mm256_max_epi16(_mm256_min_epi16(c1, v_one), v_zero);
        b1 = _mm256_min_epi16(b1, v_one);
        d1 = _mm256_min_epi16(d1, v_one);
        a1 = _mm256_slli_epi16(a1, 16 - l1shiftbits);
        c1 = _mm256_slli_epi16(c1, 16 - l1shiftbits);
        __m256i res1_p1 = _mm256_mulhi_epi16(a1, b1);
        __m256i res1_p2 = _mm256_mulhi_epi16(c1, d1);
        __m256i p1 = _mm256_packus_epi16(res1_p1, res1_p2);
        _mm256_store_si256((__m256i*)&output[halfL1 + i], p1);
    }
  }

  static void transform(const I16 *__restrict input, U8 *__restrict output,
                        int color) {
    if (pairwise) {
      pairwise_avx2(input, output, color);
    } else if (perspectivecrelu) {
      for (int i = 0; i < L1size; i++) {
        output[i] = (crelu<I16>(input[color * L1size + i], L1Q) >> l1shiftbits);
        output[L1size + i] =
            (crelu<I16>(input[(color ^ 1) * L1size + i], L1Q) >> l1shiftbits);
      }
    } else {
      for (int i = 0; i < L1size; i++) {
        output[i] = (crelu<I16>(input[color * L1size + i], L1Q) *
                     crelu<I16>(input[color * L1size + i], L1Q)) >>
                    l1shiftbits;
        output[L1size + i] =
            (crelu<I16>(input[(color ^ 1) * L1size + i], L1Q) *
             crelu<I16>(input[(color ^ 1) * L1size + i], L1Q)) >>
            l1shiftbits;
      }
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

template <int inputsize> struct CSqrActivation {
  static void transform(const I32 *input, I32 *output, I32 Q) {
    for (int i = 0; i < inputsize; i++) {
      output[i] = csqr(input[i], Q);
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

template <int inputsize> struct SingleLayerAffine {
  static I32 transform(const I16 *stminput, const I16 *nstminput,
                       const I16 *stmweights, const I16 *nstmweights, I16 bias,
                       I16 Q) {
    if (perspectivecrelu) {
      int eval = bias;
      for (int i = 0; i < inputsize; i++) {
        I16 stmclipped = crelu<I16>(stminput[i], Q);
        I16 nstmclipped = crelu<I16>(nstminput[i], Q);
        eval += I16(stmclipped * stmweights[i]);
        eval += I16(nstmclipped * nstmweights[i]);
      }
      return eval;
    } else {
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
  }
};