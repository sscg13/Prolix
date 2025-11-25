#include "../consts.h"
#include "arch.h"
#include <cstdint>
#include <string>
#pragma once

class Searcher;
struct PSQFeatureWeights {
  alignas(64) I16 nnuelayer1[realbuckets][768][L1size];
  alignas(64) I16 layer1bias[L1size];
  static constexpr int size = L1size * (1536 * realbuckets + 2);

  void load(unsigned char *stream);
};
//not correct right now
static constexpr int threatcount = 7504;
struct ThreatFeatureWeights {
  alignas(64) I8 nnuelayer1[threatcount][L1size];
  static constexpr int size = threatcount * L1size;

  void load(unsigned char *stream);
};

struct MultiLayerWeights {
  alignas(64) I8 nnuelayer2[outputbuckets * L1size * L2size];
  alignas(64) I32 layer2bias[outputbuckets * L2size];
  alignas(64) I32 nnuelayer3[outputbuckets * L2size * L3size];
  alignas(64) I32 layer3bias[outputbuckets * L3size];
  alignas(64) I32 nnuelayer4[outputbuckets * L3size];
  alignas(64) I32 finalbias[outputbuckets];
  static constexpr int size = outputbuckets * (L2size * (L1size + 4) + 4 * (L3size * (L2size + 2) + 1));

  void load(unsigned char *stream);
};

struct SingleLayerWeights {
  alignas(64) I16 nnuelayer2[outputbuckets * 2 * L1size];
  alignas(64) I16 finalbias[outputbuckets];
  static constexpr int size = outputbuckets * (4 * L1size + 2);

  void load(unsigned char *stream);
};

#ifdef MULTI_LAYER
using LayerWeights = MultiLayerWeights;
using AccumulatorOutputType = U8;
#else
using LayerWeights = SingleLayerWeights;
using AccumulatorOutputType = I16;
#endif

struct PSQNNUEWeights {
  PSQFeatureWeights psqweights;
  LayerWeights layerweights;
  static constexpr int size = PSQFeatureWeights::size + LayerWeights::size;

  void loaddefaultnet();
  PSQNNUEWeights() { loaddefaultnet(); }
  void readnnuefile(std::string file);
};

struct ThreatNNUEWeights {
  ThreatFeatureWeights threatweights;
  PSQFeatureWeights psqweights;
  LayerWeights layerweights;
  static constexpr int size = ThreatFeatureWeights::size + PSQFeatureWeights::size + LayerWeights::size;

  void loaddefaultnet();
  ThreatNNUEWeights() { loaddefaultnet(); }
  void readnnuefile(std::string file);
};

#ifdef THREAT_INPUTS
using NNUEWeights = ThreatNNUEWeights;
#else
using NNUEWeights = PSQNNUEWeights;
#endif

struct PSQAccumulatorStack {
  PSQFeatureWeights *weights;
  int totalmaterial;
  int ply;
  I16 cacheaccumulators[inputbuckets][2][L1size];
  U64 cachebitboards[inputbuckets][2][8];
  I16 accumulation[2 * (maxmaxdepth + 32)][L1size];

  int getbucket(int kingsquare, int color);
  int featureindex(int bucket, int color, int piece, int square);
  int differencecount(int bucket, int color, const U64 *Bitboards);
  const I16 *layer1weights(int kingsquare, int color, int piece, int square);
  void add(I16 *accptr, const I16 *addptr);
  void sub(I16 *accptr, const I16 *subptr);
  void addsub(I16 *oldaccptr, I16 *newaccptr, const I16 *addptr,
              const I16 *subptr);
  void addsubsub(I16 *oldaccptr, I16 *newaccptr, const I16 *addptr,
                 const I16 *subptr1, const I16 *subptr2);
  void activatepiece(I16 *accptr, int kingsquare, int color, int piece,
                     int square);
  void deactivatepiece(I16 *accptr, int kingsquare, int color, int piece,
                       int square);                   
  void refreshfromcache(int kingsquare, int color, const U64 *Bitboards);
  void refreshfromscratch(int kingsquare, int color, const U64 *Bitboards);
  void initializennue(const U64 *Bitboards);
  void forwardaccumulators(const int notation, const U64 *Bitboards);
  void backwardaccumulators(int notation, const U64 *Bitboards);
};

struct ThreatAccumulatorStack {
  ThreatFeatureWeights *weights;
  int ply;
  I16 accumulation[2 * (maxmaxdepth + 32)][L1size];
};

struct SingleAccumulatorStack {
  PSQAccumulatorStack psqaccumulators;

  void initialize(const U64* Bitboards);
  void make(const int notation, const U64 *Bitboards);
  void unmake(const int notation, const U64 *Bitboards);
  const AccumulatorOutputType *transform(int color);
};

struct DualAccumulatorStack {
  PSQAccumulatorStack psqaccumulators;
  ThreatAccumulatorStack threataccumulators;

  void initialize(const U64* Bitboards);
  void make(const int notation, const U64 *Bitboards);
  void unmake(const int notation, const U64 *Bitboards);
  const AccumulatorOutputType *transform(int color);
};

#ifdef THREAT_INPUTS
using AccumulatorStack = DualAccumulatorStack;
#else
using AccumulatorStack = SingleAccumulatorStack;
#endif

class NNUE {
  AccumulatorStack accumulators;

public:
  int evaluate(int color);
  int scratcheval(int color, const U64 *Bitboards);
};
