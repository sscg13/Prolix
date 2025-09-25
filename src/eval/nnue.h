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
  short int accumulation[2 * maxmaxdepth + 64][nnuesize];

public:
  int featureindex(int bucket, int color, int piece, int square);
  const short int *layer1weights(int kingsquare, int color, int piece,
                                 int square);
  void activatepiece(int kingsquare, int color, int piece, int square);
  void deactivatepiece(int kingsquare, int color, int piece, int square);
  void refreshfromscratch(int kingsquare, int color, const uint64_t *Bitboards);
  void initializennue(const uint64_t *Bitboards);
  void forwardaccumulators(const int notation, const uint64_t *Bitboards);
  void backwardaccumulators(int notation, const uint64_t *Bitboards);
  int evaluate(int color);

  friend class Searcher;
};
