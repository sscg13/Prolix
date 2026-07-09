#include "../board.h"
#include "../consts.h"
#include "evalmethod.h"
#pragma once

class KP : public EvalMethod {
  const I16 *weights = nullptr;
  int tempo;

public:
  void load();
  int evaluate(Board &board) override;
  static int bucket(int sq);
};
