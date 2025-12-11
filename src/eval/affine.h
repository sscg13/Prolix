#include "../consts.h"
#include "arch.h"
#include <immintrin.h>
#include <string.h>

template <int inputsize, int outputsize> struct SparseAffineWeights {
  alignas(64) I8 weights[outputbuckets * inputsize * outputsize];
  alignas(64) I32 bias[outputbuckets * outputsize];
  static constexpr int size = outputbuckets * outputsize * (inputsize + 4);
  void load(const char *stream) {
    if (!__builtin_cpu_supports("avx2")) {
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
      _mm256_store_si256((__m256i *)output, outvec[i]);
    }
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

  static void
  transform(const U8 *input, I32 *output,
            const SparseAffineWeights<inputsize, outputsize> *weights,
            int bucket) {
    // avx2 gives better performance than avxvnni so fall back to avx2 for now
    if (!__builtin_cpu_supports("avx2")) {
      transform_scalar(input, output, weights, bucket);
    } else {
      transform_avx2(input, output, weights, bucket);
    }
  }
};