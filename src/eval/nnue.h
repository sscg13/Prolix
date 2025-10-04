#include "../consts.h"
#include "arch.h"
#include <cstdint>
#include <string>
#pragma once

class Searcher;
struct NNUEWeights {
  short int nnuelayer1[realbuckets][768][nnuesize];
  short int layer1bias[nnuesize];
  int ourlayer2[outputbuckets][nnuesize];
  int theirlayer2[outputbuckets][nnuesize];
  int finalbias[outputbuckets];

  void loaddefaultnet(); 
  NNUEWeights() { loaddefaultnet(); }
  void readnnuefile(std::string file);
};

class NNUE {
  NNUEWeights* weights;
  int totalmaterial;
  int ply;
  short int cacheaccumulators[inputbuckets][2][nnuesize];
  uint64_t cachebitboards[inputbuckets][2][8];
  short int accumulation[2 * maxmaxdepth + 64][nnuesize];

public:
  int getbucket(int kingsquare, int color);
  int featureindex(int bucket, int color, int piece, int square);
  int differencecount(int bucket, int color, const uint64_t *Bitboards);
  const short int *layer1weights(int kingsquare, int color, int piece,
                                 int square);
  void activatepiece(short int* accptr, int kingsquare, int color, int piece, int square);
  void deactivatepiece(short int* accptr, int kingsquare, int color, int piece, int square);
  void refreshfromcache(int kingsquare, int color, const uint64_t *Bitboards);
  void refreshfromscratch(int kingsquare, int color, const uint64_t *Bitboards);
  void initializennue(const uint64_t *Bitboards);
  void forwardaccumulators(const int notation, const uint64_t *Bitboards, const int* pieces);
  void backwardaccumulators(int notation);
  int evaluate(int color);

  friend class Searcher;
};
