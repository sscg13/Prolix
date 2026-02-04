#include "../consts.h"
#include "arch.h"
#include <immintrin.h>
#include <string.h>

template <int inputsize, int outputsize> struct SparseAffineWeights {
  alignas(64) I8 weights[outputbuckets * inputsize * outputsize];
  alignas(64) I32 bias[outputbuckets * outputsize];
  static constexpr int size = outputbuckets * outputsize * (inputsize + 4);
  void load(const char *stream) {
    for (int b = 0; b < outputbuckets; b++) {
      for (int k = 0; k < inputsize; k += 4) {
        for (int j = 0; j < outputsize; j++) {
          for (int i = 0; i < 4; i++) {
            int dest_idx = outputsize * k + 4 * j + i;
            int src_idx = j * inputsize + (k + i);
            weights[b * inputsize * outputsize + dest_idx] =
                stream[b * inputsize * outputsize + src_idx];
          }
        }
      }
    }
    memcpy(bias, stream + outputbuckets * outputsize * inputsize,
           4 * outputbuckets * outputsize);
  }
};

template <int inputsize, int outputsize> struct SparseAffine {
  __attribute__((target("avx512f,avx512bw,avx512vl,avx512vnni"))) static void
  transform_avx512vnni(
      const U8 *input, I32 *output,
      const SparseAffineWeights<inputsize, outputsize> *weights, int bucket) {
    constexpr int numaccums = outputsize / 16;
    int weightoffset = bucket * inputsize * outputsize;
    int biasoffset = bucket * outputsize;
    const __m512i *weightptr =
        (const __m512i *)(&(weights->weights[weightoffset]));
    __m512i sums[numaccums][4];
    for (int i = 0; i < numaccums; i++) {
      sums[i][0] =
          _mm512_load_si512((__m512i *)(&(weights->bias[biasoffset + 16 * i])));
      for (int u = 1; u < 4; u++) {
        sums[i][u] = _mm512_setzero_si512();
      }
    }
    for (int k = 0; k < inputsize / 4; k += 4) {
      __m512i in0 = _mm512_set1_epi32(*(const int32_t *)(&input[4 * (k + 0)]));
      __m512i in1 = _mm512_set1_epi32(*(const int32_t *)(&input[4 * (k + 1)]));
      __m512i in2 = _mm512_set1_epi32(*(const int32_t *)(&input[4 * (k + 2)]));
      __m512i in3 = _mm512_set1_epi32(*(const int32_t *)(&input[4 * (k + 3)]));
      for (int i = 0; i < numaccums; i++) {
        __m512i w0 =
            _mm512_load_si512((__m512i *)(weightptr + (k + 0) * numaccums + i));
        __m512i w1 =
            _mm512_load_si512((__m512i *)(weightptr + (k + 1) * numaccums + i));
        __m512i w2 =
            _mm512_load_si512((__m512i *)(weightptr + (k + 2) * numaccums + i));
        __m512i w3 =
            _mm512_load_si512((__m512i *)(weightptr + (k + 3) * numaccums + i));
        sums[i][0] = _mm512_dpbusd_epi32(sums[i][0], in0, w0);
        sums[i][1] = _mm512_dpbusd_epi32(sums[i][1], in1, w1);
        sums[i][2] = _mm512_dpbusd_epi32(sums[i][2], in2, w2);
        sums[i][3] = _mm512_dpbusd_epi32(sums[i][3], in3, w3);
      }
    }
    for (int i = 0; i < numaccums; i++) {
      __m512i sum01 = _mm512_add_epi32(sums[i][0], sums[i][1]);
      __m512i sum23 = _mm512_add_epi32(sums[i][2], sums[i][3]);
      __m512i final_acc = _mm512_add_epi32(sum01, sum23);
      _mm512_store_si512((__m512i *)(&output[16 * i]), final_acc);
    }
  }

  __attribute__((target("avx2,fma"))) static void
  transform_avx2(const U8 *input, I32 *output,
                 const SparseAffineWeights<inputsize, outputsize> *weights,
                 int bucket) {
    int weightoffset = bucket * inputsize * outputsize;
    int biasoffset = bucket * outputsize;
    const __m256i *weightptr =
        (const __m256i *)(&(weights->weights[weightoffset]));
    constexpr int numaccums = outputsize / 8;
    __m256i outvec[numaccums];
    for (int i = 0; i < numaccums; i++) {
      outvec[i] =
          _mm256_load_si256((__m256i *)(&(weights->bias[biasoffset + 8 * i])));
    }
    for (int k = 0; k < inputsize / 4; k++) {
      for (int i = 0; i < numaccums; i++) {
        __m256i w =
            _mm256_load_si256((__m256i *)(weightptr + k * numaccums + i));
        __m256i in = _mm256_set1_epi32(*(const int32_t *)(&input[4 * k]));
        outvec[i] = _mm256_add_epi32(
            outvec[i], _mm256_madd_epi16(_mm256_maddubs_epi16(in, w),
                                         _mm256_set1_epi16(1)));
      }
    }
    for (int i = 0; i < numaccums; i++) {
      _mm256_store_si256((__m256i *)(&output[8 * i]), outvec[i]);
    }
  }

  static void
  transform(const U8 *input, I32 *output,
            const SparseAffineWeights<inputsize, outputsize> *weights,
            int bucket) {
    if (__builtin_cpu_supports("avx512vnni")) {
      transform_avx512vnni(input, output, weights, bucket);
    } else {
      transform_avx2(input, output, weights, bucket);
    }
  }
};