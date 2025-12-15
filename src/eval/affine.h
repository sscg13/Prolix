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
            weights[b * inputsize * outputsize + dest_idx] = stream[b * inputsize * outputsize + src_idx];
          }
        }
      }
    }
    memcpy(bias, stream + outputbuckets * outputsize * inputsize, 4 * outputbuckets * outputsize);
  }
};

template <int inputsize, int outputsize> struct SparseAffine {
  static void
  transform(const U8 *input, I32 *output,
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
};