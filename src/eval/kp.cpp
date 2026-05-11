#define INCBIN_PREFIX
#include "kp.h"
#include "../external/incbin/incbin.h"

INCBIN(I16, KP, KPfile);

int KP::bucket(int sq) { return ((sq & 56) >> 1) | (sq & 3); }

void KP::load() {
  weights = KPData;
  tempo = weights[22528];
}

int KP::evaluate(const int color, const U64 *Bitboards, const int *pieces) {
  int evals[2] = {0, 0};

  for (int c = 0; c < 2; c++) {
    const int ksq = __builtin_ctzll(Bitboards[7] & Bitboards[c]);
    const int transform = (56 * c) ^ ((ksq & 7) >= 4 ? 7 : 0);

    for (int side = 0; side < 2; side++) {
      U64 occ = Bitboards[side];
      while (occ) {
        int sq = __builtin_ctzll(occ);
        occ &= occ - 1;
        if (sq == ksq) {
          continue;
        }

        int pt = (side == c) ? (pieces[sq] & 7) - 2 : (pieces[sq] & 7) + 3;
        evals[c] += weights[(bucket(ksq ^ transform) * 11 + pt) * 64 +
                            (sq ^ transform)];
      }
    }
  }

  return evals[color] - evals[color ^ 1] + tempo;
}
