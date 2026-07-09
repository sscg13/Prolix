#include "psqt.h"
int PSQT::evaluate(Board &board) {
  int color = board.position & 1;
  const U64 *Bitboards = board.Bitboards;
  const int *pieces = board.pieces;
  evals[0] = 0;
  evals[1] = 0;
  for (int i = 0; i < 2; i++) {
    U64 occ = Bitboards[i];
    while (occ) {
      int sq = __builtin_ctzll(occ);
      occ &= (occ - 1);
      evals[i] += piecesquaretable[pieces[sq] - 8 * i - 2][(56 * i) ^ sq];
    }
  }
  return evals[color] - evals[color ^ 1] + tempo;
}