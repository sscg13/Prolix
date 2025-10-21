#include "../consts.h"
#include "arch.h"
#include <cstdint>
#include <string>
#pragma once

class Searcher;
struct alignas(64) NNUEWeights {
  I16 nnuelayer1[realbuckets][768][nnuesize];
  I16 layer1bias[nnuesize];
  int ourlayer2[outputbuckets][nnuesize];
  int theirlayer2[outputbuckets][nnuesize];
  int finalbias[outputbuckets];

  void loaddefaultnet();
  NNUEWeights() { loaddefaultnet(); }
  void readnnuefile(std::string file);
};

class NNUE {
  NNUEWeights *weights;
  int totalmaterial;
  int ply;
  I16 cacheaccumulators[inputbuckets][2][nnuesize];
  U64 cachebitboards[inputbuckets][2][8];
  I16 accumulation[2 * maxmaxdepth + 64][nnuesize];

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
