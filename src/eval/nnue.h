#include "../consts.h"
#include "arch.h"
#include "layers.h"
#include <cstdint>
#include <string>
#pragma once

class Searcher;

using Layer2Affine = SparseAffine<L1size, L2size>;
#ifdef DUAL_ACTIVATION
using Layer2Activation = DualActivation<L2size>;
using Layer2Shift = DivideShift<L2size, 2 * L1bits - pairwiseshiftbits>;
constexpr int activatedL2size = L2size * 2;
constexpr int totalL2Q = L2Q;
constexpr int totalL3Q = L2Q * L2Q * L3Q;
constexpr int totalL4Q = totalL3Q * L4Q;
#else
using Layer2Activation = CReLUActivation<L2size>;
using Layer2Shift = DivideShift<L2size, 0>;
constexpr int activatedL2size = L2size;
constexpr int totalL2Q = ((L1Q * L1Q) >> pairwiseshiftbits) * L2Q;
constexpr int totalL3Q = totalL2Q * L3Q;
constexpr int totalL4Q = totalL3Q * L4Q;
#endif
using Layer3Affine = DenseAffine<activatedL2size, L3size>;
using Layer3Activation = CReLUActivation<L3size>;
using Layer4Affine = DenseAffine<L3size, 1>;

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
  SparseAffineWeights<L1size, L2size> layer2weights;
  DenseAffineWeights<activatedL2size, L3size> layer3weights;
  DenseAffineWeights<L3size, 1> layer4weights;
  static constexpr int size = SparseAffineWeights<L1size, L2size>::size 
  + DenseAffineWeights<activatedL2size, L3size>::size
  + DenseAffineWeights<L3size, 1>::size;

  void load(const char *stream);
};

#ifdef MULTI_LAYER
using LayerWeights = MultiLayerWeights;
using AccumulatorOutputType = U8;
#else
using LayerWeights = PerspectiveWeights<L1size>;
using AccumulatorOutputType = I16;
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
  int propagate(const int bucket, const int color,
                const I16 *input);
};

struct MultiLayerStack {
  MultiLayerWeights *weights;
  I32 layer2raw[L2size];
  I32 layer2activated[activatedL2size];
  I32 layer3raw[L3size];
  I32 layer3activated[L3size];
  I32 output[1];

  void load(NNUEWeights *EUNNweights);
  int propagate(const int bucket, const int color,
                const U8 *input);
};

struct PSQAccumulatorStack {
  PSQFeatureWeights *weights;
  int ply;
  I16 cacheaccumulators[inputbuckets][2][L1size];
  U64 cachebitboards[inputbuckets][2][8];
  I16 accumulation[2 * (maxmaxdepth + 32)][L1size];

  int getbucket(const int kingsquare, const int color);
  int featureindex(const int bucket, const int color, const int piece,
                   const int square);
  int differencecount(const int bucket, const int color, const U64 *Bitboards);
  const I16 *layer1weights(const int kingsquare, const int color,
                           const int piece, const int square);
  void add(I16 *accptr, const I16 *addptr);
  void sub(I16 *accptr, const I16 *subptr);
  void addsub(I16 *oldaccptr, I16 *newaccptr, const I16 *addptr,
              const I16 *subptr);
  void addsubsub(I16 *oldaccptr, I16 *newaccptr, const I16 *addptr,
                 const I16 *subptr1, const I16 *subptr2);
  void activatepiece(I16 *accptr, const int kingsquare, const int color,
                     const int piece, const int square);
  void deactivatepiece(I16 *accptr, const int kingsquare, const int color,
                       const int piece, const int square);
  void refreshfromcache(const int kingsquare, const int color,
                        const U64 *Bitboards);
  void refreshfromscratch(const int kingsquare, const int color,
                          const U64 *Bitboards);
  void initializennue(const U64 *Bitboards);
  void forwardaccumulators(const int notation, const U64 *Bitboards);
  void backwardaccumulators(const int notation, const U64 *Bitboards);
  const I16 *currentaccumulator() { return accumulation[2 * ply]; }
};

struct ThreatAccumulatorStack {
  ThreatFeatureWeights *weights;
  int ply;
  I16 accumulation[2 * (maxmaxdepth + 32)][L1size];
};

struct SingleAccumulatorStack {
  PSQAccumulatorStack psqaccumulators;
  U8 pairwise[L1size];

  void load(NNUEWeights *EUNNweights);
  void initialize(const U64 *Bitboards);
  void make(const int notation, const U64 *Bitboards);
  void unmake(const int notation, const U64 *Bitboards);
  const AccumulatorOutputType *transform(int color);
};

struct DualAccumulatorStack {
  PSQAccumulatorStack psqaccumulators;
  ThreatAccumulatorStack threataccumulators;
  AccumulatorOutputType output[L1size * (1 + multilayer)];

  void load(NNUEWeights *EUNNweights);
  void initialize(const U64 *Bitboards);
  void make(const int notation, const U64 *Bitboards);
  void unmake(const int notation, const U64 *Bitboards);
  const AccumulatorOutputType *transform(int color);
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
  int evaluate(const int color);
};
