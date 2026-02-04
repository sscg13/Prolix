#include "nnue.h"
#include <fstream>
#include <iostream>
#define INCBIN_PREFIX
#include "../external/incbin/incbin.h"

INCBIN(char, NNUE, EUNNfile);
template <typename T> T crelu(T x, T Q) {
  return std::max(std::min(x, Q), (T)0);
}
I32 csqr(I32 x, I32 Q) {
  I32 y = std::max(std::min(x, Q), -Q);
  return y * y;
}
void vectoradd(I16 *__restrict accptr, const I16 *__restrict addptr) {
  accptr = (I16 *)__builtin_assume_aligned(accptr, 64);
  addptr = (I16 *)__builtin_assume_aligned(addptr, 64);
  for (int i = 0; i < L1size; i++) {
    accptr[i] += addptr[i];
  }
}
void vectorsub(I16 *__restrict accptr, const I16 *__restrict subptr) {
  accptr = (I16 *)__builtin_assume_aligned(accptr, 64);
  subptr = (I16 *)__builtin_assume_aligned(subptr, 64);
  for (int i = 0; i < L1size; i++) {
    accptr[i] -= subptr[i];
  }
}
void vectoraddsub(I16 *__restrict oldaccptr, I16 *__restrict newaccptr,
                  const I16 *__restrict addptr, const I16 *__restrict subptr) {
  oldaccptr = (I16 *)__builtin_assume_aligned(oldaccptr, 64);
  newaccptr = (I16 *)__builtin_assume_aligned(newaccptr, 64);
  addptr = (I16 *)__builtin_assume_aligned(addptr, 64);
  subptr = (I16 *)__builtin_assume_aligned(subptr, 64);
  for (int i = 0; i < L1size; i++) {
    newaccptr[i] = oldaccptr[i] + addptr[i] - subptr[i];
  }
}
void vectoraddsubsub(I16 *__restrict oldaccptr, I16 *__restrict newaccptr,
                     const I16 *__restrict addptr,
                     const I16 *__restrict subptr1,
                     const I16 *__restrict subptr2) {
  oldaccptr = (I16 *)__builtin_assume_aligned(oldaccptr, 64);
  newaccptr = (I16 *)__builtin_assume_aligned(newaccptr, 64);
  addptr = (I16 *)__builtin_assume_aligned(addptr, 64);
  subptr1 = (I16 *)__builtin_assume_aligned(subptr1, 64);
  subptr2 = (I16 *)__builtin_assume_aligned(subptr2, 64);
  for (int i = 0; i < L1size; i++) {
    newaccptr[i] = oldaccptr[i] + addptr[i] - subptr1[i] - subptr2[i];
  }
}
void vectoraddaddsub(I16 *__restrict oldaccptr, I16 *__restrict newaccptr,
                     const I16 *__restrict addptr1,
                     const I16 *__restrict addptr2,
                     const I16 *__restrict subptr) {
  oldaccptr = (I16 *)__builtin_assume_aligned(oldaccptr, 64);
  newaccptr = (I16 *)__builtin_assume_aligned(newaccptr, 64);
  addptr1 = (I16 *)__builtin_assume_aligned(addptr1, 64);
  addptr2 = (I16 *)__builtin_assume_aligned(addptr2, 64);
  subptr = (I16 *)__builtin_assume_aligned(subptr, 64);
  for (int i = 0; i < L1size; i++) {
    newaccptr[i] = oldaccptr[i] + addptr1[i] + addptr2[i] - subptr[i];
  }
}
void packuspermute_avx2(I16 *weights) {
    for (int i = 0; i < L1size; i += 32) {
        __m256i reg0 = _mm256_loadu_si256((__m256i*)&weights[i]);
        __m256i reg1 = _mm256_loadu_si256((__m256i*)&weights[i + 16]);
        __m256i perm0 = _mm256_permute2x128_si256(reg0, reg1, 0x20);
        __m256i perm1 = _mm256_permute2x128_si256(reg0, reg1, 0x31);
        _mm256_storeu_si256((__m256i*)&weights[i], perm0);
        _mm256_storeu_si256((__m256i*)&weights[i + 16], perm1);
    }
}
void PSQFeatureWeights::load(const char *stream) {
  int offset = 0;
  for (int k = 0; k < realbuckets; k++) {
    for (int i = 0; i < 768; i++) {
      int piece = i / 64;
      int square = i % 64;
      int convert[12] = {0, 3, 1, 4, 2, 5, 6, 9, 7, 10, 8, 11};
      memcpy(nnuelayer1[k][64 * convert[piece] + square], stream + offset,
              2 * L1size);
      offset += 2 * L1size;
      if (pairwise) {
        packuspermute_avx2(nnuelayer1[k][64 * convert[piece] + square]);
      }
    }
  }
  memcpy(layer1bias, stream + offset, 2 * L1size);
  if (pairwise) {
    packuspermute_avx2(layer1bias);
  }
}
void ThreatFeatureWeights::load(const char *stream) {
  memcpy(nnuelayer1, stream, size);
}
void MultiLayerWeights::load(const char *stream) {
  int offset = 0;
  layer2weights.load(stream + offset);
  offset += layer2weights.size;
  layer3weights.load(stream + offset);
  // offset += layer3weights.size;
  // layer4weights.load(stream + offset);
}
void PSQNNUEWeights::loaddefaultnet() {
  psqweights.load(&NNUEData[0]);
  layerweights.load(&NNUEData[psqweights.size]);
}
void ThreatNNUEWeights::loaddefaultnet() {
  threatweights.load(&NNUEData[0]);
  psqweights.load(&NNUEData[threatweights.size]);
  layerweights.load(&NNUEData[threatweights.size + psqweights.size]);
}
void PSQNNUEWeights::readnnuefile(std::string file) {
  std::ifstream nnueweights;
  nnueweights.open(file, std::ifstream::binary);
  char *psqweightptr = new char[psqweights.size];
  nnueweights.read(psqweightptr, psqweights.size);
  char *layerweightptr = new char[layerweights.size];
  nnueweights.read(layerweightptr, layerweights.size);
  psqweights.load(psqweightptr);
  layerweights.load(layerweightptr);
  delete[] psqweightptr;
  delete[] layerweightptr;
  nnueweights.close();
}
void ThreatNNUEWeights::readnnuefile(std::string file) {
  std::ifstream nnueweights;
  nnueweights.open(file, std::ifstream::binary);
  char *threatweightptr = new char[threatweights.size];
  nnueweights.read(threatweightptr, threatweights.size);
  char *psqweightptr = new char[psqweights.size];
  nnueweights.read(psqweightptr, psqweights.size);
  char *layerweightptr = new char[layerweights.size];
  nnueweights.read(layerweightptr, layerweights.size);
  threatweights.load(threatweightptr);
  psqweights.load(psqweightptr);
  layerweights.load(layerweightptr);
  delete[] threatweightptr;
  delete[] psqweightptr;
  delete[] layerweightptr;
  nnueweights.close();
}
int PSQAccumulatorStack::getbucket(int kingsquare, int color) {
  return kingbuckets[(56 * color) ^ kingsquare];
}
int PSQAccumulatorStack::featureindex(int bucket, int color, int piece,
                                      int square) {
  int piececolor = piece / 6;
  int perspectivepiece = (color ^ piececolor) * 6 + (piece % 6);
  bool hmactive = mirrored && (bucket % 2 == 1);
  int perspectivesquare = (56 * color) ^ square ^ (7 * hmactive);
  return 64 * perspectivepiece + perspectivesquare;
}
int PSQAccumulatorStack::differencecount(int bucket, int color,
                                         const U64 *Bitboards) {
  return __builtin_popcountll(
      (cachebitboards[bucket][color][0] | cachebitboards[bucket][color][1]) ^
      (Bitboards[0] | Bitboards[1]));
}
const I16 *PSQAccumulatorStack::layer1weights(int kingsquare, int color,
                                              int piece, int square) {
  int bucket = getbucket(kingsquare, color);
  return weights->nnuelayer1[bucket / mirrordivisor]
                            [featureindex(bucket, color, piece, square)];
}

void PSQAccumulatorStack::activatepiece(I16 *__restrict accptr, int kingsquare,
                                        int color, int piece, int square) {
  const I16 *weightsptr = layer1weights(kingsquare, color, piece, square);
  vectoradd(accptr, weightsptr);
}
void PSQAccumulatorStack::deactivatepiece(I16 *__restrict accptr,
                                          int kingsquare, int color, int piece,
                                          int square) {
  const I16 *weightsptr = layer1weights(kingsquare, color, piece, square);
  vectorsub(accptr, weightsptr);
}
void PSQAccumulatorStack::refreshfromcache(int kingsquare, int color,
                                           const U64 *Bitboards) {
  U64 add[12];
  U64 remove[12];
  int bucket = getbucket(kingsquare, color);
  I16 *accptr = accumulation[2 * ply + color];
  I16 *cacheaccptr = cacheaccumulators[bucket][color];
  for (int i = 0; i < 12; i++) {
    add[i] = (Bitboards[i / 6] & Bitboards[2 + (i % 6)]) &
             ~(cachebitboards[bucket][color][i / 6] &
               cachebitboards[bucket][color][2 + (i % 6)]);
    remove[i] = (cachebitboards[bucket][color][i / 6] &
                 cachebitboards[bucket][color][2 + (i % 6)]) &
                ~(Bitboards[i / 6] & Bitboards[2 + (i % 6)]);
  }
  for (int i = 0; i < 12; i++) {
    int addcount = __builtin_popcountll(add[i]);
    for (int j = 0; j < addcount; j++) {
      int square = __builtin_ctzll(add[i]);
      activatepiece(cacheaccptr, kingsquare, color, i, square);
      add[i] ^= (1ULL << square);
    }
    int removecount = __builtin_popcountll(remove[i]);
    for (int j = 0; j < removecount; j++) {
      int square = __builtin_ctzll(remove[i]);
      deactivatepiece(cacheaccptr, kingsquare, color, i, square);
      remove[i] ^= (1ULL << square);
    }
  }
  for (int i = 0; i < 8; i++) {
    cachebitboards[bucket][color][i] = Bitboards[i];
  }
  for (int i = 0; i < L1size; i++) {
    accptr[i] = cacheaccptr[i];
  }
  kingsquares[2 * ply + color] = kingsquare;
  computed[2 * ply + color] = true;
}
void PSQAccumulatorStack::refreshfromscratch(int kingsquare, int color,
                                             const U64 *Bitboards) {
  I16 *accptr = accumulation[2 * ply + color];
  I16 *cacheaccptr = cacheaccumulators[getbucket(kingsquare, color)][color];
  for (int i = 0; i < L1size; i++) {
    accptr[i] = weights->layer1bias[i];
  }
  for (int i = 0; i < 8; i++) {
    cachebitboards[getbucket(kingsquare, color)][color][i] = Bitboards[i];
  }
  for (int i = 0; i < 12; i++) {
    U64 pieces = (Bitboards[i / 6] & Bitboards[2 + (i % 6)]);
    int piececount = __builtin_popcountll(pieces);
    for (int j = 0; j < piececount; j++) {
      int square = __builtin_popcountll((pieces & -pieces) - 1);
      activatepiece(accptr, kingsquare, color, i, square);
      pieces ^= (1ULL << square);
    }
  }
  for (int i = 0; i < L1size; i++) {
    cacheaccptr[i] = accptr[i];
  }
  kingsquares[2 * ply + color] = kingsquare;
  computed[2 * ply + color] = true;
}
void PSQAccumulatorStack::initializennue(const U64 *Bitboards) {
  ply = 0;
  for (int i = 0; i < 2 * (maxmaxdepth + 32); i++) {
    computed[i] = false;
    moves[i / 2] = 0;
    kingsquares[i] = -1;
  }
  for (int color = 0; color < 2; color++) {
    refreshfromscratch(__builtin_ctzll(Bitboards[7] & Bitboards[color]), color,
                       Bitboards);
    for (int bucket = 0; bucket < inputbuckets; bucket++) {
      for (int i = 0; i < 8; i++) {
        cachebitboards[bucket][color][i] = 0ULL;
      }
      for (int i = 0; i < L1size; i++) {
        cacheaccumulators[bucket][color][i] = weights->layer1bias[i];
      }
    }
  }
}
void PSQAccumulatorStack::reversechange(int accply, int color) {
  int notation = moves[accply];
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int movecolor = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 20) & 1;
  int piece2 = (promoted > 0) ? 2 : piece - 2;
  I16 *oldaccptr = accumulation[2 * (accply + 1) + color];
  I16 *newaccptr = accumulation[2 * accply + color];
  int kingsq = kingsquares[2 * accply + color];
  const I16 *subweights =
      layer1weights(kingsq, color, 6 * movecolor + piece2, to);
  const I16 *addweights =
      layer1weights(kingsq, color, 6 * movecolor + piece - 2, from);
  if (captured > 0) {
    const I16 *addweights2 =
        layer1weights(kingsq, color, 6 * (movecolor ^ 1) + captured - 2, to);
    vectoraddaddsub(oldaccptr, newaccptr, addweights, addweights2, subweights);
  } else {
    vectoraddsub(oldaccptr, newaccptr, addweights, subweights);
  }
  computed[2 * accply + color] = true;
}
void PSQAccumulatorStack::applychange(int accply, int color) {
  int notation = moves[accply];
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int movecolor = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 20) & 1;
  int piece2 = (promoted > 0) ? 2 : piece - 2;
  I16 *newaccptr = accumulation[2 * (accply + 1) + color];
  I16 *oldaccptr = accumulation[2 * accply + color];
  int kingsq = kingsquares[2 * accply + color];
  const I16 *addweights =
      layer1weights(kingsq, color, 6 * movecolor + piece2, to);
  const I16 *subweights =
      layer1weights(kingsq, color, 6 * movecolor + piece - 2, from);
  if (captured > 0) {
    const I16 *subweights2 =
        layer1weights(kingsq, color, 6 * (movecolor ^ 1) + captured - 2, to);
    vectoraddsubsub(oldaccptr, newaccptr, addweights, subweights, subweights2);
  } else {
    vectoraddsub(oldaccptr, newaccptr, addweights, subweights);
  }
  computed[2 * (accply + 1) + color] = true;
}
void PSQAccumulatorStack::forwardaccumulators(int notation) {
  moves[ply] = notation;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  kingsquares[2 * (ply + 1) + (color ^ 1)] = kingsquares[2 * ply + (color ^ 1)];
  if (piece == 7) {
    kingsquares[2 * (ply + 1) + color] = to;
  } else {
    kingsquares[2 * (ply + 1) + color] = kingsquares[2 * ply + color];
  }
  ply++;
  computed[2 * ply] = false;
  computed[2 * ply + 1] = false;
}
void PSQAccumulatorStack::backwardaccumulators() { ply--; }
void PSQAccumulatorStack::computeaccumulator(int color, const U64 *Bitboards) {
  int accply = ply;
  int bucket = getbucket(kingsquares[2 * ply + color], color);
  while (!computed[2 * accply + color] && accply > 0) {
    accply--;
    if (getbucket(kingsquares[2 * accply + color], color) != bucket) {
      int scratchrefreshtime =
          __builtin_popcountll(Bitboards[0] | Bitboards[1]);
      int cacherefreshtime = differencecount(bucket, color, Bitboards);
      if (scratchrefreshtime <= cacherefreshtime) {
        refreshfromscratch(kingsquares[2 * ply + color], color, Bitboards);
      } else {
        refreshfromcache(kingsquares[2 * ply + color], color, Bitboards);
      }
      for (int j = ply - 1; j > accply; j--) {
        reversechange(j, color);
      }
      return;
    }
  }
  while (accply < ply) {
    applychange(accply, color);
    accply++;
  }
}
const I16 *PSQAccumulatorStack::currentaccumulator(const U64 *Bitboards) {
  computeaccumulator(0, Bitboards);
  computeaccumulator(1, Bitboards);
  return accumulation[2 * ply];
}
void SingleAccumulatorStack::load(NNUEWeights *EUNNweights) {
  psqaccumulators.weights = &(EUNNweights->psqweights);
}
void SingleAccumulatorStack::initialize(const U64 *Bitboards) {
  psqaccumulators.initializennue(Bitboards);
}
void SingleAccumulatorStack::make(int notation, const U64 *Bitboards) {
  psqaccumulators.forwardaccumulators(notation);
}
void SingleAccumulatorStack::unmake(int notation, const U64 *Bitboards) {
  psqaccumulators.backwardaccumulators();
}
const I16 *SingleAccumulatorStack::transform(int color, const U64 *Bitboards) {
  return psqaccumulators.currentaccumulator(Bitboards);
}
void LayerStack::load(NNUEWeights *EUNNweights) {
  weights = &(EUNNweights->layerweights);
}
int SingleLayerStack::propagate(int bucket, int color, const I16 *input) {
  // int eval = 0;
  const I16 *stmaccptr = input + color * L1size;
  const I16 *nstmaccptr = input + (color ^ 1) * L1size;
  const I16 *stmweightsptr = &(weights->weights[bucket * 2 * L1size]);
  const I16 *nstmweightsptr = &(weights->weights[bucket * 2 * L1size + L1size]);
  int eval = SingleLayerAffine<L1size>::transform(stmaccptr, nstmaccptr,
                                                  stmweightsptr, nstmweightsptr,
                                                  weights->bias[bucket], L1Q);
  eval *= evalscale;
  eval /= (L1Q * L2Q);
  return eval;
}
int MultiLayerStack::propagate(int bucket, int color, const I16 *input) {
  PerspectiveTransform::transform(input, layer1activated, color);
  Layer2Affine::transform(layer1activated, layer2raw, &(weights->layer2weights),
                          bucket);
  Layer2Activation::transform(layer2raw, layer2activated, totalL2Q);
  Layer2Shift::transform(layer2activated);
  Layer3Affine::transform(layer2activated, output, &(weights->layer3weights),
                          bucket);
  // Layer3Activation::transform(layer3raw, layer3activated, totalL3Q);
  // Layer4Affine::transform(layer3activated, output, &(weights->layer4weights),
  //                         bucket);
  int eval = output[0];
  eval /= (L3Q);
  eval *= evalscale;
  eval /= activatedL2Q;
  return eval;
}
void NNUE::load(NNUEWeights *EUNNweights) {
  accumulators.load(EUNNweights);
  layers.load(EUNNweights);
}
void NNUE::initialize(const U64 *Bitboards) {
  totalmaterial = 0;
  for (int i = 0; i < 6; i++) {
    totalmaterial += material[i] * __builtin_popcountll(Bitboards[2 + i]);
  }
  accumulators.initialize(Bitboards);
}
void NNUE::make(int notation, const U64 *Bitboards) {
  int captured = (notation >> 17) & 7;
  if (captured > 0) {
    totalmaterial -= material[captured - 2];
  }
  accumulators.make(notation, Bitboards);
}
void NNUE::unmake(int notation, const U64 *Bitboards) {
  int captured = (notation >> 17) & 7;
  if (captured > 0) {
    totalmaterial += material[captured - 2];
  }
  accumulators.unmake(notation, Bitboards);
}
int NNUE::evaluate(int color, const U64 *Bitboards) {
  int bucket = std::min(totalmaterial / bucketdivisor, outputbuckets - 1);
  const I16 *layerstackinput = accumulators.transform(color, Bitboards);
  return layers.propagate(bucket, color, layerstackinput);
}
