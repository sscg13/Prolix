#include "board.h"
#include "consts.h"
#include "eval/nnue.h"
#include "eval/prf.h"

#pragma once
struct EvalParams {
  NNUEWeights *nnueweights = new NNUEWeights;
};
class Evaluator {
  NNUE EUNN;
  PRF PFR;

public:
  int level;
  void load(EvalParams &params);
  void init(Board &Bitboards);
  void make(int notation, Board &Bitboards);
  void unmake(int notation, Board &Bitboards);
  int evaluate(Board &Bitboards);
};