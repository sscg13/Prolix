#pragma once

#include "consts.h"

struct Zobrist {
  U64 totalhash = 0ULL;
  U64 piecehash[2][6] = {};

  void modpiece(int piece, int square);
  void modturn();
  void reset(const int *pieces);
  U64 pawnhash() const;
  U64 keyafter(int notation) const;

  static U64 piecekey(int piece, int square);
};

void initializezobrist();
