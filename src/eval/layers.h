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
    const int weightbytes = 4 * outputbuckets * inputsize * outputsize;
    if constexpr (outputsize == 1) {
      memcpy(weights, stream, weightbytes);
    } else {
      const I32 *src = (const I32 *)stream;
      for (int b = 0; b < outputbuckets; b++) {
        for (int j = 0; j < outputsize; j++) {
          for (int i = 0; i < inputsize; i++) {
            weights[b * inputsize * outputsize + i * outputsize + j] =
                src[b * inputsize * outputsize + j * inputsize + i];
          }
        }
      }
    }
    memcpy(bias, stream + weightbytes, 4 * outputbuckets * outputsize);
  }
};

template <int inputsize, int outputsize> struct DenseAffine {
  __attribute__((target("avx2"))) static void
  transform_avx2(const I32 *__restrict input, I32 *__restrict output,
                 const DenseAffineWeights<inputsize, outputsize> *weights,
                 int bucket) {
    int weightoffset = bucket * inputsize * outputsize;
    int biasoffset = bucket * outputsize;
    const I32 *weightptr = &weights->weights[weightoffset];
    if constexpr (outputsize == 1) {
      __m256i acc = _mm256_setzero_si256();
      for (int k = 0; k < inputsize; k += 8) {
        __m256i w = _mm256_load_si256((const __m256i *)(weightptr + k));
        __m256i in = _mm256_load_si256((const __m256i *)(input + k));
        acc = _mm256_add_epi32(acc, _mm256_mullo_epi32(w, in));
      }
      __m128i lo = _mm256_castsi256_si128(acc);
      __m128i hi = _mm256_extracti128_si256(acc, 1);
      __m128i sum = _mm_add_epi32(lo, hi);
      sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 0, 3, 2)));
      sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1)));
      output[0] = weights->bias[biasoffset] + _mm_cvtsi128_si32(sum);
    } else {
      constexpr int numaccums = outputsize / 8;
      __m256i outvec[numaccums];
      for (int i = 0; i < numaccums; i++)
        outvec[i] = _mm256_load_si256(
            (const __m256i *)(&weights->bias[biasoffset + 8 * i]));
      for (int k = 0; k < inputsize; k++) {
        __m256i in = _mm256_set1_epi32(input[k]);
        for (int i = 0; i < numaccums; i++) {
          __m256i w = _mm256_load_si256(
              (const __m256i *)(weightptr + k * outputsize + 8 * i));
          outvec[i] = _mm256_add_epi32(outvec[i], _mm256_mullo_epi32(w, in));
        }
      }
      for (int i = 0; i < numaccums; i++)
        _mm256_store_si256((__m256i *)(&output[8 * i]), outvec[i]);
    }
  }

  __attribute__((target("avx512f,avx512bw"))) static void
  transform_avx512(const I32 *__restrict input, I32 *__restrict output,
                   const DenseAffineWeights<inputsize, outputsize> *weights,
                   int bucket) {
    int weightoffset = bucket * inputsize * outputsize;
    int biasoffset = bucket * outputsize;
    const I32 *weightptr = &weights->weights[weightoffset];
    if constexpr (outputsize == 1) {
      __m512i acc = _mm512_setzero_si512();
      for (int k = 0; k < inputsize; k += 16) {
        __m512i w = _mm512_load_si512((const __m512i *)(weightptr + k));
        __m512i in = _mm512_load_si512((const __m512i *)(input + k));
        acc = _mm512_add_epi32(acc, _mm512_mullo_epi32(w, in));
      }
      __m128i q0 = _mm512_extracti32x4_epi32(acc, 0);
      __m128i q1 = _mm512_extracti32x4_epi32(acc, 1);
      __m128i q2 = _mm512_extracti32x4_epi32(acc, 2);
      __m128i q3 = _mm512_extracti32x4_epi32(acc, 3);
      __m128i sum = _mm_add_epi32(_mm_add_epi32(q0, q1), _mm_add_epi32(q2, q3));
      sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(1, 0, 3, 2)));
      sum = _mm_add_epi32(sum, _mm_shuffle_epi32(sum, _MM_SHUFFLE(2, 3, 0, 1)));
      output[0] = weights->bias[biasoffset] + _mm_cvtsi128_si32(sum);
    } else if constexpr (outputsize % 16 == 0) {
      constexpr int numaccums = outputsize / 16;
      __m512i outvec[numaccums];
      for (int i = 0; i < numaccums; i++)
        outvec[i] = _mm512_load_si512(
            (const __m512i *)(&weights->bias[biasoffset + 16 * i]));
      for (int k = 0; k < inputsize; k++) {
        __m512i in = _mm512_set1_epi32(input[k]);
        for (int i = 0; i < numaccums; i++) {
          __m512i w = _mm512_load_si512(
              (const __m512i *)(weightptr + k * outputsize + 16 * i));
          outvec[i] = _mm512_add_epi32(outvec[i], _mm512_mullo_epi32(w, in));
        }
      }
      for (int i = 0; i < numaccums; i++)
        _mm512_store_si512((__m512i *)(&output[16 * i]), outvec[i]);
    } else {
      transform_avx2(input, output, weights, bucket);
    }
  }

  static void
  transform(const I32 *input, I32 *output,
            const DenseAffineWeights<inputsize, outputsize> *weights,
            int bucket) {
    if (__builtin_cpu_supports("avx512f")) {
      transform_avx512(input, output, weights, bucket);
    } else {
      transform_avx2(input, output, weights, bucket);
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
  __attribute__((target("avx512f,avx512bw"))) static void
  pairwise_avx512(const I16 *__restrict input, U8 *__restrict output,
                  int color) {
    constexpr int halfL1 = L1size / 2;
    const __m512i v_one = _mm512_set1_epi16(L1Q);
    const __m512i v_zero = _mm512_setzero_si512();
    const int16_t *in0_lo = &input[color * L1size];
    const int16_t *in0_hi = &input[color * L1size + halfL1];
    const int16_t *in1_lo = &input[(color ^ 1) * L1size];
    const int16_t *in1_hi = &input[(color ^ 1) * L1size + halfL1];
    auto compute_avx512 =
        [&](__m512i a, __m512i b)
            __attribute__((target("avx512f,avx512bw"))) -> __m512i {
      a = _mm512_max_epi16(_mm512_min_epi16(a, v_one), v_zero);
      b = _mm512_min_epi16(b, v_one);
      a = _mm512_slli_epi16(a, 16 - l1shiftbits);
      return _mm512_mulhi_epi16(a, b);
    };
    for (int i = 0; i < halfL1; i += 64) {
      __m512i a0 = _mm512_load_si512((const __m512i *)&in0_lo[i]);
      __m512i b0 = _mm512_load_si512((const __m512i *)&in0_hi[i]);
      __m512i c0 = _mm512_load_si512((const __m512i *)&in0_lo[i + 32]);
      __m512i d0 = _mm512_load_si512((const __m512i *)&in0_hi[i + 32]);
      __m512i res0_p1 = compute_avx512(a0, b0);
      __m512i res0_p2 = compute_avx512(c0, d0);
      __m512i p0 = _mm512_packus_epi16(res0_p1, res0_p2);
      _mm512_store_si512((__m512i *)&output[i], p0);
      __m512i a1 = _mm512_load_si512((const __m512i *)&in1_lo[i]);
      __m512i b1 = _mm512_load_si512((const __m512i *)&in1_hi[i]);
      __m512i c1 = _mm512_load_si512((const __m512i *)&in1_lo[i + 32]);
      __m512i d1 = _mm512_load_si512((const __m512i *)&in1_hi[i + 32]);
      __m512i res1_p1 = compute_avx512(a1, b1);
      __m512i res1_p2 = compute_avx512(c1, d1);
      __m512i p1 = _mm512_packus_epi16(res1_p1, res1_p2);
      _mm512_store_si512((__m512i *)&output[halfL1 + i], p1);
    }
  }

  static void pairwise_avx2(const I16 *__restrict input, U8 *__restrict output,
                            int color) {
    constexpr int halfL1 = L1size / 2;
    const __m256i v_one = _mm256_set1_epi16(L1Q);
    const __m256i v_zero = _mm256_setzero_si256();
    const int16_t *in0_lo = &input[color * L1size];
    const int16_t *in0_hi = &input[color * L1size + halfL1];
    const int16_t *in1_lo = &input[(color ^ 1) * L1size];
    const int16_t *in1_hi = &input[(color ^ 1) * L1size + halfL1];
    auto compute_avx2 = [&](__m256i a, __m256i b) -> __m256i {
      a = _mm256_max_epi16(_mm256_min_epi16(a, v_one), v_zero);
      b = _mm256_min_epi16(b, v_one);
      a = _mm256_slli_epi16(a, 16 - l1shiftbits);
      return _mm256_mulhi_epi16(a, b);
    };
    for (int i = 0; i < halfL1; i += 32) {
      __m256i a0 = _mm256_load_si256((const __m256i *)&in0_lo[i]);
      __m256i b0 = _mm256_load_si256((const __m256i *)&in0_hi[i]);
      __m256i c0 = _mm256_load_si256((const __m256i *)&in0_lo[i + 16]);
      __m256i d0 = _mm256_load_si256((const __m256i *)&in0_hi[i + 16]);
      __m256i res0_p1 = compute_avx2(a0, b0);
      __m256i res0_p2 = compute_avx2(c0, d0);
      __m256i p0 = _mm256_packus_epi16(res0_p1, res0_p2);
      _mm256_store_si256((__m256i *)&output[i], p0);
      __m256i a1 = _mm256_load_si256((const __m256i *)&in1_lo[i]);
      __m256i b1 = _mm256_load_si256((const __m256i *)&in1_hi[i]);
      __m256i c1 = _mm256_load_si256((const __m256i *)&in1_lo[i + 16]);
      __m256i d1 = _mm256_load_si256((const __m256i *)&in1_hi[i + 16]);
      __m256i res1_p1 = compute_avx2(a1, b1);
      __m256i res1_p2 = compute_avx2(c1, d1);
      __m256i p1 = _mm256_packus_epi16(res1_p1, res1_p2);
      _mm256_store_si256((__m256i *)&output[halfL1 + i], p1);
    }
  }

  static void transform(const I16 *__restrict input, U8 *__restrict output,
                        int color) {
    if (pairwise) {
      if (__builtin_cpu_supports("avx512bw")) {
        pairwise_avx512(input, output, color);
      } else {
        pairwise_avx2(input, output, color);
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

template <int inputsize, int bits> struct CReLUDivide {
  static void transform(const I32 *input, I32 *output, I32 Q) {
    for (int i = 0; i < inputsize; i++) {
      output[i] = crelu<I32>(input[i], Q) >> bits;
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

template <int inputsize, int bits> struct CSqrDivide {
  static void transform(const I32 *input, I32 *output, I32 Q) {
    for (int i = 0; i < inputsize; i++) {
      I32 clipped = input[i];
      if (clipped < -Q)
        clipped = -Q;
      if (clipped > Q)
        clipped = Q;
      output[i] = (clipped * clipped) >> bits;
    }
  }
};

template <int inputsize, int bits> struct CSqrCReLUDivide {
  static void transform(const I32 *input, I32 *output, I32 Q) {
    for (int i = 0; i < inputsize; i++) {
      I32 clipped = input[i];
      if (clipped < -Q)
        clipped = -Q;
      if (clipped > Q)
        clipped = Q;
      output[i + inputsize] = (clipped * clipped) >> bits;
      if (clipped < 0)
        clipped = 0;
      output[i] = (Q * clipped) >> bits;
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