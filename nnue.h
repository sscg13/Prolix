#include "arch.h"
#include <cstdint>
#pragma once
class NNUE {
  short int nnuelayer1[realbuckets][768][nnuesize];
  short int layer1bias[nnuesize];
  int ourlayer2[outputbuckets][nnuesize];
  int theirlayer2[outputbuckets][nnuesize];
  short int accumulator[inputbuckets][2][nnuesize];
  int finalbias[outputbuckets];
  int activebucket[2];
  int totalmaterial;

public:
  void loaddefaultnet();
  void readnnuefile(std::string file);
  void activatepiece(int bucket, int color, int piece, int square);
  void deactivatepiece(int bucket, int color, int piece, int square);
  void refreshfromscratch(int bucket, int color, uint64_t *Bitboards);
  void initializennue(uint64_t *Bitboards);
  void forwardaccumulators(int notation, uint64_t *Bitboards);
  void backwardaccumulators(int notation, uint64_t *Bitboards);
  int evaluate(int color);
};
