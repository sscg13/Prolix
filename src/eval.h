#include "board.h"
#include "consts.h"
#include "eval/basic.h"
#include "eval/evalmethod.h"
#include "eval/kp.h"
#include "eval/nnue/nnue.h"
#include "eval/pp.h"
#include "eval/ppxk.h"
#include "eval/prf.h"
#include "eval/psqt.h"

#pragma once

#if defined(HAS_EVALFILE)
constexpr int topevallevel = 8;
#elif defined(HAS_PPXKFILE)
constexpr int topevallevel = 7;
#elif defined(HAS_PPFILE)
constexpr int topevallevel = 6;
#else
constexpr int topevallevel = 5;
#endif

int resolveevallevel(int requested);

struct EvalParams {
#ifdef HAS_EVALFILE
  NNUEWeights *nnueweights = new NNUEWeights;
#endif
};
class Evaluator {
  RandomEval randomeval; // 0
  MaterialEval material;  // 1
  PRF PFR;                // 2
  PSQT PST;               // 3
  KP kp;                  // 4
  HCEEval hce;            // 5
  PP pp;                  // 6
  PPXK ppxk;              // 7
  NNUE EUNN;              // 8
  // Level -> method table, filled in load().  The active method is selected by
  // setlevel and every operation dispatches through it, so the Evaluator needs
  // no per-level switch.  The pointers point into this object, so they must be
  // (re)established after the Evaluator reaches its final home; load() runs per
  // search (via syncwith), before setlevel and any use.
  EvalMethod *methods[9];
  EvalMethod *active = &hce;

public:
  int level = topevallevel;
  void setlevel(int requested);
  void load(EvalParams &params);
  void init(Board &Bitboards);
  void make(int notation, Board &Bitboards);
  void unmake(int notation, Board &Bitboards);
  int evaluate(Board &Bitboards);
};