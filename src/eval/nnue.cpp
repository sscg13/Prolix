#include "nnue.h"
#include <fstream>
#include <iostream>
#define INCBIN_PREFIX
#include "../external/incbin/incbin.h"

INCBIN(char, NNUE, EUNNfile);
template <typename T> T crelu(T x, T Q) {
  return std::max(std::min(x, Q), (T)0);
}
I32 csqr(I32 x, I32 Q) { return std::min(x * x, Q * Q); }
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
    }
  }
  memcpy(layer1bias, stream + offset, 2 * L1size);
}
void ThreatFeatureWeights::load(const char *stream) {
  memcpy(nnuelayer1, stream, size);
}
void MultiLayerWeights::load(const char *stream) {
  int offset = 0;
  layer2weights.load(stream + offset);
  offset += layer2weights.size;
  layer3weights.load(stream + offset);
  offset += layer3weights.size;
  layer4weights.load(stream + offset);
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
}
void PSQAccumulatorStack::initializennue(const U64 *Bitboards) {
  ply = 0;
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
void PSQAccumulatorStack::forwardaccumulators(int notation,
                                              const U64 *Bitboards) {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 20) & 1;
  int piece2 = (promoted > 0) ? 2 : piece - 2;
  int oppksq = __builtin_ctzll(Bitboards[7] & Bitboards[color ^ 1]);
  int ourksq = __builtin_ctzll(Bitboards[7] & Bitboards[color]);
  I16 *newaccptr = accumulation[2 * (ply + 1) + (color ^ 1)];
  I16 *oldaccptr = accumulation[2 * ply + (color ^ 1)];
  const I16 *addweightsopp =
      layer1weights(oppksq, color ^ 1, 6 * color + piece2, to);
  const I16 *subweightsopp =
      layer1weights(oppksq, color ^ 1, 6 * color + piece - 2, from);
  ply++;
  if (captured > 0) {
    const I16 *subweights2opp =
        layer1weights(oppksq, color ^ 1, 6 * (color ^ 1) + captured - 2, to);
    vectoraddsubsub(oldaccptr, newaccptr, addweightsopp, subweightsopp,
                    subweights2opp);
  } else {
    vectoraddsub(oldaccptr, newaccptr, addweightsopp, subweightsopp);
  }
  if (piece == 7 && getbucket(to, color) != getbucket(from, color)) {
    int scratchrefreshtime = __builtin_popcountll(Bitboards[0] | Bitboards[1]);
    int cacherefreshtime =
        differencecount(getbucket(to, color), color, Bitboards);
    if (scratchrefreshtime <= cacherefreshtime) {
      refreshfromscratch(to, color, Bitboards);
    } else {
      refreshfromcache(to, color, Bitboards);
    }
  } else {
    newaccptr = accumulation[2 * ply + color];
    oldaccptr = accumulation[2 * (ply - 1) + color];
    const I16 *addweightsus =
        layer1weights(ourksq, color, 6 * color + piece2, to);
    const I16 *subweightsus =
        layer1weights(ourksq, color, 6 * color + piece - 2, from);
    if (captured > 0) {
      const I16 *subweights2us =
          layer1weights(ourksq, color, 6 * (color ^ 1) + captured - 2, to);
      vectoraddsubsub(oldaccptr, newaccptr, addweightsus, subweightsus,
                      subweights2us);
    } else {
      vectoraddsub(oldaccptr, newaccptr, addweightsus, subweightsus);
    }
  }
}
void PSQAccumulatorStack::backwardaccumulators(int notation,
                                               const U64 *Bitboards) {
  ply--;
}
void SingleAccumulatorStack::load(NNUEWeights *EUNNweights) {
  psqaccumulators.weights = &(EUNNweights->psqweights);
}
void SingleAccumulatorStack::initialize(const U64 *Bitboards) {
  psqaccumulators.initializennue(Bitboards);
}
void SingleAccumulatorStack::make(int notation, const U64 *Bitboards) {
  psqaccumulators.forwardaccumulators(notation, Bitboards);
}
void SingleAccumulatorStack::unmake(int notation, const U64 *Bitboards) {
  psqaccumulators.backwardaccumulators(notation, Bitboards);
}
const I16 *SingleAccumulatorStack::transform(int color) {
  return psqaccumulators.currentaccumulator();
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
  Layer2Shift::transform(layer2raw);
  Layer2Activation::transform(layer2raw, layer2activated, totalL2Q);
  Layer3Affine::transform(layer2activated, layer3raw, &(weights->layer3weights),
                          bucket);
  Layer3Activation::transform(layer3raw, layer3activated, totalL3Q);
  Layer4Affine::transform(layer3activated, output, &(weights->layer4weights),
                          bucket);
  int eval = output[0];
  eval /= (L4Q);
  eval *= evalscale;
  eval /= totalL3Q;
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
int NNUE::evaluate(int color) {
  int bucket = std::min(totalmaterial / bucketdivisor, outputbuckets - 1);
  const I16 *layerstackinput = accumulators.transform(color);
  return layers.propagate(bucket, color, layerstackinput);
}
