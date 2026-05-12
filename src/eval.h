#include "board.h"
#include "consts.h"
#include "eval/kp.h"
#include "eval/nnue.h"
#include "eval/prf.h"
#include "eval/psqt.h"

#pragma once
struct EvalParams {
#ifdef HAS_EVALFILE
  NNUEWeights *nnueweights = new NNUEWeights;
#endif
};
class Evaluator {
  NNUE EUNN;
  KP kp;
  PRF PFR;
  PSQT PST;

public:
  int level;
  void load(EvalParams &params);
  void init(Board &Bitboards);
  void make(int notation, Board &Bitboards);
  void unmake(int notation, Board &Bitboards);
  int evaluate(Board &Bitboards);
};