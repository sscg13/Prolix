#include "../board.h"
#include "evalmethod.h"
#pragma once

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
