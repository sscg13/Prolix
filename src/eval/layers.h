#include "../consts.h"
#include "arch.h"
#include <cmath>
#include <string.h>
#pragma once

template<typename T> T crelu(T x, T Q);
I32 csqr(I32 x, I32 Q);

template <int inputsize, int outputsize> struct SparseAffineWeights {
  alignas(64) I8 weights[outputbuckets * inputsize * outputsize];
  alignas(64) I32 bias[outputbuckets * outputsize];
  static constexpr int size = outputbuckets * outputsize * (inputsize + 4);
  void load(const char* stream) {
    int offset = 0;
    memcpy(weights, stream + offset, outputbuckets * inputsize * outputsize);
    offset += outputbuckets * inputsize * outputsize;
    memcpy(bias, stream + offset, 4 * outputbuckets * outputsize);
  }
};

template <int inputsize, int outputsize> struct SparseAffine {
  static void transform(const U8 *input, I32 *output, const SparseAffineWeights<inputsize, outputsize>* weights, int bucket) {
    memcpy(output, weights->bias, 4 * outputsize);
    for (int i = 0; i < inputsize; i++) {
      if (input[i]) {
        const I8* vector = &(weights->weights[bucket * inputsize * outputsize + i * outputsize]);
        const int factor = input[i];
        for (int j = 0; j < outputsize; j++) {
          output[j] += vector[j] * factor;
        }
      }
    }
  }
};

template <int inputsize, int outputsize> struct DenseAffineWeights {
  alignas(64) I32 weights[outputbuckets * outputsize * inputsize];
  alignas(64) I32 bias[outputbuckets * outputsize];
  static constexpr int size = 4 * outputbuckets * outputsize * (inputsize + 1);
  void load(const char* stream) {
    int offset = 0;
    memcpy(weights, stream + offset, 4 * outputbuckets * inputsize * outputsize);
    offset += 4 * outputbuckets * inputsize * outputsize;
    memcpy(bias, stream + offset, 4 * outputbuckets * outputsize);
  }
};

template <int inputsize, int outputsize> struct DenseAffine {
  static void transform(const I32 *input, I32 *output, const DenseAffineWeights<inputsize, outputsize>* weights, int bucket) {
    memcpy(output, weights->bias, 4 * outputsize);
    for (int j = 0; j < outputsize; j++) {
      const I32* vector = &(weights->weights[bucket * outputsize * inputsize + j * inputsize]);
      for (int i = 0; i < inputsize; i++) {
        output[j] += input[i]*vector[i];
      }
    }
  }
};

template <int inputsize> struct PerspectiveWeights {
  alignas(64) I16 weights[2 * outputbuckets * inputsize];
  alignas(64) I16 bias[outputbuckets];
  static constexpr int size = outputbuckets * (4 * inputsize + 2);
  void load(const char* stream) {
    int offset = 0;
    memcpy(weights, stream + offset, 4 * outputbuckets * inputsize);
    offset += 4 * outputbuckets * inputsize;
    memcpy(bias, stream + offset, 2 * outputbuckets);
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
  static void transform(I32* input) {
    for (int i = 0; i < inputsize; i++) {
      input[i] >>= bits;
    }
  }
};

template <int inputsize> struct PerspectiveSCReLU {
  static I32 transform(const I16 *stminput, const I16* nstminput, const I16* stmweights, const I16* nstmweights, I16 bias, I16 Q) {
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
  static I32 transform(const I16 *stminput, const I16* nstminput, const I16* stmweights, const I16* nstmweights, I16 bias, I16 Q) {
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