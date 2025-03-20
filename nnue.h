#include "arch.h"
#include <string>
#include <cstdint>
#pragma once
struct Accumulator {
  short int accumulation[2][nnuesize];
};

class NNUE {
  short int nnuelayer1[realbuckets][768][nnuesize];
  short int layer1bias[nnuesize];
  int ourlayer2[outputbuckets][nnuesize];
  int theirlayer2[outputbuckets][nnuesize];
  int finalbias[outputbuckets];
  int totalmaterial;
  int ply;
  Accumulator accumulators[64];


public:
  void loaddefaultnet();
  void readnnuefile(std::string file);
  void activatepiece(int kingsquare, int color, int piece, int square);
  void deactivatepiece(int kingsquare, int color, int piece, int square);
  void refreshfromscratch(int kingsquare, int color, const uint64_t *Bitboards);
  void initializennue(const uint64_t *Bitboards);
  void forwardaccumulators(const int notation, const uint64_t *Bitboards);
  void backwardaccumulators(int notation, const uint64_t *Bitboards);
  int evaluate(int color);
};
