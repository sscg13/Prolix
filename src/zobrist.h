#pragma once

#include "consts.h"

struct Zobrist {
private:
  static U64 piecekeys[2][6][64];
  static constexpr U64 colorhash = 0xE344F58E0F3B26E5ULL;
  friend void initializezobrist();

public:
  U64 totalhash = 0ULL;
  U64 piecehash[2][6] = {};

  void modmove(int notation);
  void modturn();
  void reset(const int *pieces);
  U64 pawnhash() const;
  U64 keyafter(int notation) const;
};

void initializezobrist();
