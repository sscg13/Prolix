#include "../../consts.h"
#include "arch.h"
#include <immintrin.h>
#include <string.h>

// AVX2 consumes one bit per consecutive four-byte input block. AVX-512 VNNI
// consumes a compact list of the live block indices instead: that avoids a
// ctz/bit-clear iteration in the hot affine loop.
template <int inputsize> struct BlockNNZInfo {
  static_assert(inputsize % 4 == 0,
                "Sparse affine input must consist of four-byte blocks");
  static constexpr int blockcount = inputsize / 4;
  static constexpr int wordcount = (blockcount + 63) / 64;
  alignas(64) U64 bitset[wordcount];
  alignas(64) U16 nnz[blockcount];
  int count;

  void begin_avx2() { memset(bitset, 0, sizeof(bitset)); }

  __attribute__((target("avx2"), always_inline)) void
  record_avx2(__m256i values, int byte) {
    const __m256i zero = _mm256_setzero_si256();
    const __m256i equals_zero = _mm256_cmpeq_epi32(values, zero);
    const U64 nonzero_blocks =
        U64((~_mm256_movemask_ps(_mm256_castsi256_ps(equals_zero))) & 0xFF);
    const int block = byte / 4;
    bitset[block / 64] |= nonzero_blocks << (block % 64);
  }

  __attribute__((target("avx512f"), always_inline)) void
  record_avx512_bitset(__m512i values, int byte) {
    const __m512i zero = _mm512_setzero_si512();
    const U64 nonzero_blocks =
        U64(_mm512_cmpneq_epi32_mask(values, zero));
    const int block = byte / 4;
    bitset[block / 64] |= nonzero_blocks << (block % 64);
  }

  void begin_avx512() { count = 0; }

  __attribute__((target("avx2"))) void find_avx2(const U8 *input) {
    begin_avx2();
    for (int byte = 0; byte < inputsize; byte += 32) {
      const __m256i values =
          _mm256_load_si256((const __m256i *)(input + byte));
      record_avx2(values, byte);
    }
  }

  __attribute__((target("avx512f,avx512bw,avx512vl"), always_inline)) void
  record_avx512(__m512i values, int byte) {
    const __m512i zero = _mm512_setzero_si512();
    const __m512i indices = _mm512_add_epi32(
        _mm512_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
                           15),
        _mm512_set1_epi32(byte / 4));
    const __mmask16 nonzero_blocks = _mm512_cmpneq_epi32_mask(values, zero);
    const __m512i live_indices =
        _mm512_maskz_compress_epi32(nonzero_blocks, indices);
    _mm512_mask_cvtepi32_storeu_epi16(nnz + count, 0xFFFF, live_indices);
    count += __builtin_popcount(nonzero_blocks);
  }

  __attribute__((target("avx512f,avx512bw,avx512vl"))) void
  find_avx512(const U8 *input) {
    begin_avx512();
    for (int byte = 0; byte < inputsize; byte += 64) {
      const __m512i values =
          _mm512_load_si512((const __m512i *)(input + byte));
      record_avx512(values, byte);
    }
  }

  void find(const U8 *input) {
    if (__builtin_cpu_supports("avx512vnni")) {
      find_avx512(input);
    } else {
      find_avx2(input);
    }
  }
};

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
      const SparseAffineWeights<inputsize, outputsize> *weights, int bucket,
      const BlockNNZInfo<inputsize> *nnz) {
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
    int dependency_chain = 0;
    const U16 *index = nnz->nnz;
    const U16 *end = index + nnz->count;
    while (index != end) {
      const int k = *index++;
      const __m512i in = _mm512_set1_epi32(*(const int32_t *)(&input[4 * k]));
      for (int i = 0; i < numaccums; i++) {
        const __m512i w =
            _mm512_load_si512((__m512i *)(weightptr + k * numaccums + i));
        sums[i][dependency_chain] =
            _mm512_dpbusd_epi32(sums[i][dependency_chain], in, w);
      }
      dependency_chain = (dependency_chain + 1) & 3;
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
                 int bucket, const BlockNNZInfo<inputsize> *nnz) {
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
    for (int word = 0; word < BlockNNZInfo<inputsize>::wordcount; word++) {
      U64 blocks = nnz->bitset[word];
      while (blocks) {
        const int k = 64 * word + __builtin_ctzll(blocks);
        blocks &= blocks - 1;
        const __m256i in = _mm256_set1_epi32(*(const int32_t *)(&input[4 * k]));
        for (int i = 0; i < numaccums; i++) {
          __m256i w =
              _mm256_load_si256((__m256i *)(weightptr + k * numaccums + i));
          outvec[i] = _mm256_add_epi32(
              outvec[i], _mm256_madd_epi16(_mm256_maddubs_epi16(in, w),
                                           _mm256_set1_epi16(1)));
        }
      }
    }
    for (int i = 0; i < numaccums; i++) {
      _mm256_store_si256((__m256i *)(&output[8 * i]), outvec[i]);
    }
  }

  static void
  transform(const U8 *input, I32 *output,
            const SparseAffineWeights<inputsize, outputsize> *weights,
            int bucket, const BlockNNZInfo<inputsize> *nnz) {
    if (__builtin_cpu_supports("avx512vnni")) {
      transform_avx512vnni(input, output, weights, bucket, nnz);
    } else {
      transform_avx2(input, output, weights, bucket, nnz);
    }
  }
};
