#include "threats.h"

U16 pieceoffset[16][2];
U16 sourceoffset[16][64];
U8 targetoffset[16][64][64];

void initializethreats() {
  int total = 0;
  for (int c = 0; c < 2; c++) {
    for (int t = 2; t < 7; t++) {
      int p = 8 * c + t;
      pieceoffset[p][0] = total;
      int piecetotal = 0;
      int start = (t == 2) ? 8 : 0;
      int end = (t == 2) ? 56 : 64;
      for (int s = start; s < end; s++) {
        sourceoffset[p][s] = piecetotal;
        U64 attacks = PseudoAttacks(p, s);
        int squaretotal = 0;
        while (attacks) {
          int target = __builtin_ctzll(attacks);
          targetoffset[p][s][target] = squaretotal;
          squaretotal++;
          attacks &= (attacks - 1);
        }
        piecetotal += squaretotal;
      }
      pieceoffset[p][1] = piecetotal;
      total += 2 * piecetotal;
    }
  }
  if (total != threatcount) {
    std::cout << "threat count mismatch\n";
    std::cout << "Expected: " << threatcount << " Got: " << total << "\n";
  }
}

int threatindex(int color, int ksq, Threat t) {
  int orientation = (56 * color) ^ (7 * (ksq % 8 > 3));
  int from = t.from ^ orientation;
  int to = t.to ^ orientation;
  int attkr = t.attkr ^ (8 * color);
  bool enemy = ((t.attkr ^ t.attkd) > 7);

  return pieceoffset[attkr][0] + enemy * pieceoffset[attkr][1] +
         sourceoffset[attkr][from] + targetoffset[attkr][from][to];
}

void findthreatdiff(int notation, U64 *Bitboards, int *pieces,
                    ThreatDiff &change) {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int movecolor = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 20) & 1;
  int piece2 = (promoted > 0) ? 2 : piece - 2;
  int addcount = 0;
  int subcount = 0;
}