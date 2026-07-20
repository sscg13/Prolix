#include "zobrist.h"

#include <random>

namespace {
U64 piecekeys[2][6][64];
constexpr U64 colorhash = 0xE344F58E0F3B26E5ULL;

bool validpiece(int piece) {
  int color = piece / 8;
  int type = (piece % 8) - 2;
  return color >= 0 && color < 2 && type >= 0 && type < 6;
}

bool validsquare(int square) { return square >= 0 && square < 64; }
} // namespace

void initializezobrist() {
  std::mt19937_64 mt(20346892);
  U64 oldhashes[8][64];

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 64; j++) {
      oldhashes[i][j] = mt();
    }
  }

  for (int color = 0; color < 2; color++) {
    for (int type = 0; type < 6; type++) {
      for (int square = 0; square < 64; square++) {
        piecekeys[color][type][square] =
            oldhashes[color][square] ^ oldhashes[type + 2][square];
      }
    }
  }
}

U64 Zobrist::piecekey(int piece, int square) {
  if (!validpiece(piece) || !validsquare(square)) {
    return 0ULL;
  }

  return piecekeys[piece / 8][(piece % 8) - 2][square];
}

void Zobrist::modpiece(int piece, int square) {
  if (!validpiece(piece) || !validsquare(square)) {
    return;
  }

  int color = piece / 8;
  int type = (piece % 8) - 2;
  U64 change = piecekeys[color][type][square];
  piecehash[color][type] ^= change;
  totalhash ^= change;
}

void Zobrist::modturn() { totalhash ^= colorhash; }

void Zobrist::reset(const int *pieces) {
  totalhash = 0ULL;
  for (int color = 0; color < 2; color++) {
    for (int type = 0; type < 6; type++) {
      piecehash[color][type] = 0ULL;
    }
  }

  if (pieces == nullptr) {
    return;
  }

  for (int square = 0; square < 64; square++) {
    if (validpiece(pieces[square])) {
      modpiece(pieces[square], square);
    }
  }
}

U64 Zobrist::pawnhash() const {
  return piecehash[0][0] ^ piecehash[1][0];
}

U64 Zobrist::keyafter(int notation) const {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int movedpiece = 8 * color + piece;

  if (notation & (1 << 20)) {
    movedpiece = 8 * color + 4;
  }

  U64 result = totalhash;
  result ^= Zobrist::piecekey(8 * color + piece, from);
  result ^= Zobrist::piecekey(movedpiece, to);
  if (captured != 0) {
    result ^= Zobrist::piecekey(8 * (color ^ 1) + captured, to);
  }
  result ^= colorhash;
  return result;
}
