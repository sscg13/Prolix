#include "prf.h"
void PRF::load() {
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 64; j++) {
      piecetable[i][j] = ranktable[i][j / 8] + filetable[i][j % 8];
    }
  }
}

int PRF::evaluate(const int color, const U64 *Bitboards, const int *pieces) {
  evals[0] = 0;
  evals[1] = 0;
  for (int i = 0; i < 2; i++) {
    U64 occ = Bitboards[i];
    while (occ) {
      int sq = __builtin_ctzll(occ);
      occ &= (occ - 1);
      evals[i] += piecetable[pieces[sq] - 8 * i - 2][(56 * i) ^ sq];
    }
  }
  return evals[color] - evals[color ^ 1] + bias;
}