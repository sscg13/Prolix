#include "../board.h"
#include "evalmethod.h"
#pragma once

// The trivial, stateless eval methods that are just thin views over the Board:
// a hash-based "random" eval, plain material counting, and the hand-crafted
// evaluation.  They need no accumulator state, so they only implement
// evaluate() and inherit the empty init / make / unmake.

class RandomEval : public EvalMethod {
public:
  int evaluate(Board &board) override { return board.zobristhash % 64; }
};

class MaterialEval : public EvalMethod {
public:
  int evaluate(Board &board) override {
    return board.piecevaluediff(board.position & 1);
  }
};

class HCEEval : public EvalMethod {
public:
  int evaluate(Board &board) override {
    return board.evaluate(board.position & 1);
  }
};
