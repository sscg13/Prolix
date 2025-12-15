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
    memcpy(weights, stream,
           4 * outputbuckets * inputsize * outputsize);
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
  static void transform(const I16 *input, U8 *output, int color) {
    if (pairwise) {
      for (int i = 0; i < L1size / 2; i++) {
        output[i] = (crelu<I16>(input[color * L1size + i], L1Q) *
                     crelu<I16>(input[color * L1size + L1size / 2 + i], L1Q)) >>
                    l1shiftbits;
        output[L1size / 2 + i] =
            (crelu<I16>(input[(color ^ 1) * L1size + i], L1Q) *
             crelu<I16>(input[(color ^ 1) * L1size + L1size / 2 + i], L1Q)) >>
            l1shiftbits;
      }
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