#include "../consts.h"
#include "arch.h"
#include <cstdint>
#include <string>
#pragma once

class NNUE {
  short int nnuelayer1[realbuckets][768][nnuesize];
  short int layer1bias[nnuesize];
  short int accumulation[4 * maxmaxdepth][nnuesize];
  int ourlayer2[outputbuckets][nnuesize];
  int theirlayer2[outputbuckets][nnuesize];
  short int cacheaccumulators[inputbuckets][2][nnuesize];
  uint64_t cachebitboards[inputbuckets][2][8];
  int finalbias[outputbuckets];
  int totalmaterial;
  int ply;

public:
  void loaddefaultnet();
  void readnnuefile(std::string file);
  int getbucket(int kingsquare, int color);
  int featureindex(int bucket, int color, int piece, int square);
  int differencecount(int bucket, int color, const uint64_t *Bitboards);
  const short int *layer1weights(int kingsquare, int color, int piece,
                                 int square);
  void activatepiece(int kingsquare, int color, int piece, int square);
  void deactivatepiece(int kingsquare, int color, int piece, int square);
  void refreshfromcache(int kingsquare, int color, const uint64_t *Bitboards);
  void refreshfromscratch(int kingsquare, int color, const uint64_t *Bitboards);
  void initializennue(const uint64_t *Bitboards);
  void forwardaccumulators(const int notation, const uint64_t *Bitboards);
  void backwardaccumulators(int notation, const uint64_t *Bitboards);
  int evaluate(int color);
};
