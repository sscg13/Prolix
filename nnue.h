#include "arch.h"
#include <string>
#include <cstdint>
#pragma once
struct Accumulator {
  short int accumulation[2][nnuesize];
  Accumulator* previous;
  Accumulator* next;
};

class NNUE {
  short int nnuelayer1[realbuckets][768][nnuesize];
  short int layer1bias[nnuesize];
  int ourlayer2[outputbuckets][nnuesize];
  int theirlayer2[outputbuckets][nnuesize];
  int finalbias[outputbuckets];
  int activebucket[2];
  int totalmaterial;
  Accumulator* acc;

public:
  void loaddefaultnet();
  void readnnuefile(std::string file);
  void activatepiece(int bucket, int color, int piece, int square);
  void deactivatepiece(int bucket, int color, int piece, int square);
  void refreshfromscratch(int bucket, int color, const uint64_t *Bitboards);
  void initializennue(const uint64_t *Bitboards);
  void forwardaccumulators(int notation, const uint64_t *Bitboards);
  void backwardaccumulators(int notation, const uint64_t *Bitboards);
  int evaluate(int color);
};
