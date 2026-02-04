#include "../consts.h"
#include "arch.h"
#include "layers.h"
#include <cstdint>
#include <string>
#pragma once

class Searcher;

using Layer2Affine = SparseAffine<activatedL1size, L2size>;
using Layer2Activation = CSqrActivation<L2size>;
using Layer2Shift = DivideShift<L2size, 12>;
using Layer3Affine = DenseAffine<activatedL2size, 1>;
// using Layer3Activation = CReLUActivation<L3size>;
// using Layer4Affine = DenseAffine<L3size, 1>;

struct PSQFeatureWeights {
  alignas(64) I16 nnuelayer1[realbuckets][768][L1size];
  alignas(64) I16 layer1bias[L1size];
  static constexpr int size = L1size * (1536 * realbuckets + 2);

  void load(const char *stream);
};
// not correct right now
static constexpr int threatcount = 7504;
struct ThreatFeatureWeights {
  alignas(64) I8 nnuelayer1[threatcount][L1size];
  static constexpr int size = threatcount * L1size;

  void load(const char *stream);
};

struct MultiLayerWeights {
  SparseAffineWeights<activatedL1size, L2size> layer2weights;
  DenseAffineWeights<activatedL2size, 1> layer3weights;
  // DenseAffineWeights<L3size, 1> layer4weights;
  static constexpr int size =
      SparseAffineWeights<activatedL1size, L2size>::size +
      DenseAffineWeights<activatedL2size, 1>::size;
  // DenseAffineWeights<L3size, 1>::size;

  void load(const char *stream);
};

#ifdef MULTI_LAYER
using LayerWeights = MultiLayerWeights;
#else
using LayerWeights = PerspectiveWeights<L1size>;
#endif

struct PSQNNUEWeights {
  PSQFeatureWeights psqweights;
  LayerWeights layerweights;

  void loaddefaultnet();
  PSQNNUEWeights() { loaddefaultnet(); }
  void readnnuefile(std::string file);
};

struct ThreatNNUEWeights {
  ThreatFeatureWeights threatweights;
  PSQFeatureWeights psqweights;
  LayerWeights layerweights;

  void loaddefaultnet();
  ThreatNNUEWeights() { loaddefaultnet(); }
  void readnnuefile(std::string file);
};

#ifdef THREAT_INPUTS
using NNUEWeights = ThreatNNUEWeights;
#else
using NNUEWeights = PSQNNUEWeights;
#endif

struct SingleLayerStack {
  PerspectiveWeights<L1size> *weights;

  void load(NNUEWeights *EUNNweights);
  int propagate(const int bucket, const int color, const I16 *input);
};

struct MultiLayerStack {
  MultiLayerWeights *weights;
  alignas(64) U8 layer1activated[activatedL1size];
  alignas(64) I32 layer2raw[L2size];
  alignas(64) I32 layer2activated[activatedL2size];
  // alignas(64) I32 layer3raw[L3size];
  // alignas(64) I32 layer3activated[L3size];
  I32 output[1];

  void load(NNUEWeights *EUNNweights);
  int propagate(const int bucket, const int color, const I16 *input);
};

struct PSQAccumulatorStack {
  PSQFeatureWeights *weights;
  int ply;
  alignas(64) I16 cacheaccumulators[inputbuckets][2][L1size];
  alignas(64) U64 cachebitboards[inputbuckets][2][8];
  alignas(64) I16 accumulation[2 * (maxmaxdepth + 32)][L1size];
  int moves[(maxmaxdepth + 32)];
  int kingsquares[2 * (maxmaxdepth + 32)];
  bool computed[2 * (maxmaxdepth + 32)];

  int getbucket(const int kingsquare, const int color);
  int featureindex(const int bucket, const int color, const int piece,
                   const int square);
  int differencecount(const int bucket, const int color, const U64 *Bitboards);
  const I16 *layer1weights(const int kingsquare, const int color,
                           const int piece, const int square);
  void activatepiece(I16 *__restrict accptr, const int kingsquare,
                     const int color, const int piece, const int square);
  void deactivatepiece(I16 *__restrict accptr, const int kingsquare,
                       const int color, const int piece, const int square);
  void refreshfromcache(const int kingsquare, const int color,
                        const U64 *Bitboards);
  void refreshfromscratch(const int kingsquare, const int color,
                          const U64 *Bitboards);
  void initializennue(const U64 *Bitboards);
  void reversechange(const int accply, int color);
  void applychange(const int accply, int color);
  void forwardaccumulators(const int notation);
  void backwardaccumulators();
  void computeaccumulator(int color, const U64 *Bitboards);
  const I16 *currentaccumulator(const U64 *Bitboards);
};

struct ThreatAccumulatorStack {
  ThreatFeatureWeights *weights;
  int ply;
  I16 accumulation[2 * (maxmaxdepth + 32)][L1size];
};

struct SingleAccumulatorStack {
  PSQAccumulatorStack psqaccumulators;

  void load(NNUEWeights *EUNNweights);
  void initialize(const U64 *Bitboards);
  void make(const int notation, const U64 *Bitboards);
  void unmake(const int notation, const U64 *Bitboards);
  const I16 *transform(int color, const U64 *Bitboards);
};

struct DualAccumulatorStack {
  PSQAccumulatorStack psqaccumulators;
  ThreatAccumulatorStack threataccumulators;
  I16 output[L1size * (1 + multilayer)];

  void load(NNUEWeights *EUNNweights);
  void initialize(const U64 *Bitboards);
  void make(const int notation, const U64 *Bitboards);
  void unmake(const int notation, const U64 *Bitboards);
  const I16 *transform(int color);
};

#ifdef THREAT_INPUTS
using AccumulatorStack = DualAccumulatorStack;
#else
using AccumulatorStack = SingleAccumulatorStack;
#endif

#ifdef MULTI_LAYER
using LayerStack = MultiLayerStack;
#else
using LayerStack = SingleLayerStack;
#endif

class NNUE {
  AccumulatorStack accumulators;
  LayerStack layers;

  int totalmaterial;

public:
  void load(NNUEWeights *EUNNweights);
  void initialize(const U64 *Bitboards);
  void make(const int notation, const U64 *Bitboards);
  void unmake(const int notation, const U64 *Bitboards);
  int evaluate(const int color, const U64 *Bitboards);
};
