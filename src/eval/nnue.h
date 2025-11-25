#include "../consts.h"
#include "arch.h"
#include <cstdint>
#include <string>
#pragma once

class Searcher;
#ifdef MULTI_LAYER
#else
#endif
#ifdef MULTI_LAYER
//clang-format off
constexpr int nnuefilesize = L1size * (1536 * realbuckets + 2) + outputbuckets *
(L2size * (L1size + 4) + 4 * (L3size * (L2size + 2) + 1));
//clang-format on
struct NNUEWeights {
  alignas(64) I16 nnuelayer1[realbuckets * 768 * L1size];
  alignas(64) I16 layer1bias[L1size];
  alignas(64) I8 nnuelayer2[outputbuckets * L1size * L2size];
  alignas(64) I32 layer2bias[outputbuckets * L2size];
  alignas(64) I32 nnuelayer3[outputbuckets * L2size * L3size];
  alignas(64) I32 layer3bias[outputbuckets * L3size];
  alignas(64) I32 nnuelayer4[outputbuckets * L3size];
  alignas(64) I32 finalbias[outputbuckets];

};
#else
//clang-format off
constexpr int nnuefilesize = L1size * (1536 * realbuckets + 2) + outputbuckets * (4 * L1size + 2);
//clang-format on
struct NNUEWeights {
  alignas(64) I16 nnuelayer1[realbuckets][768][L1size];
  alignas(64) I16 layer1bias[L1size];
  alignas(64) I16 nnuelayer2[outputbuckets * 2 * L1size];
  alignas(64) I16 finalbias[outputbuckets];

  void loaddefaultnet();
  NNUEWeights() { loaddefaultnet(); }
  void readnnuefile(std::string file);
};
#endif



class NNUE {
  NNUEWeights *weights;
  int totalmaterial;
  int ply;
  I16 cacheaccumulators[inputbuckets][2][L1size];
  U64 cachebitboards[inputbuckets][2][8];
  I16 accumulation[2 * maxmaxdepth + 64][L1size];
  #ifdef MULTI_LAYER
  I8 L1pairwise[L1size];
  #endif

public:
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
  int evaluate(int color);

  friend class Searcher;
};
