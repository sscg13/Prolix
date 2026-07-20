#include "zobrist.h"

#include <random>

U64 Zobrist::piecekeys[2][6][64];

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
        Zobrist::piecekeys[color][type][square] =
            oldhashes[color][square] ^ oldhashes[type + 2][square];
      }
    }
  }
}

void Zobrist::modmove(int notation) {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int movedpiece = piece;

  if (notation & (1 << 20)) {
    movedpiece = 4;
  }

  int pieceindex = piece - 2;
  int movedindex = movedpiece - 2;
  U64 fromchange = piecekeys[color][pieceindex][from];
  U64 tochange = piecekeys[color][movedindex][to];

  if (pieceindex == movedindex) {
    piecehash[color][pieceindex] ^= fromchange ^ tochange;
  } else {
    piecehash[color][pieceindex] ^= fromchange;
    piecehash[color][movedindex] ^= tochange;
  }
  totalhash ^= fromchange ^ tochange;

  if (captured != 0) {
    int capturedindex = captured - 2;
    U64 capturechange = piecekeys[color ^ 1][capturedindex][to];
    piecehash[color ^ 1][capturedindex] ^= capturechange;
    totalhash ^= capturechange;
  }
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
    int piece = pieces[square];
    if (piece != 0) {
      int color = piece / 8;
      int type = (piece % 8) - 2;
      U64 change = piecekeys[color][type][square];
      piecehash[color][type] ^= change;
      totalhash ^= change;
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
  int movedpiece = piece;

  if (notation & (1 << 20)) {
    movedpiece = 4;
  }

  U64 result = totalhash;
  result ^= piecekeys[color][piece - 2][from];
  result ^= piecekeys[color][movedpiece - 2][to];
  if (captured != 0) {
    result ^= piecekeys[color ^ 1][captured - 2][to];
  }
  result ^= colorhash;
  return result;
}
