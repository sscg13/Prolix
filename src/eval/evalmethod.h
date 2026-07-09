#include "../board.h"
#include "../consts.h"
#pragma once

// Common interface for every evaluation the engine can run.  A method reads a
// position through evaluate(); the incremental ones (PP, PPxK, NNUE) also keep
// per-ply state, so they override init / make / unmake to build and maintain
// their accumulators.  Stateless methods (material, PST, HCE, ...) inherit the
// empty defaults.  The Evaluator drives whichever one is active through a
// single pointer, so it needs no per-operation dispatch of its own.
class EvalMethod {
public:
  virtual void init(Board &) {}
  virtual void make(int, Board &) {}
  virtual void unmake(int, Board &) {}
  virtual int evaluate(Board &board) = 0;
  virtual ~EvalMethod() = default;
};
