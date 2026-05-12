#include "../board.h"
#include "../consts.h"
#pragma once

class KP {
  const I16 *weights = nullptr;
  int tempo;

public:
  void load();
  int evaluate(const int color, const U64 *Bitboards, const int *pieces);
  static int bucket(int sq);
};
