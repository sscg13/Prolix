#define INCBIN_PREFIX
#include "pp.h"
#include "../external/incbin/incbin.h"

#ifdef HAS_PPFILE
INCBIN(I16, PP, PPfile);
#endif

// Feature index for the piece on `sq` in the fixed white perspective:
//   type  = (piece & 7) - 2   in [0, 6)
//   color = (piece >> 3) & 1  (0 = white, 1 = black)
//   feat  = (type + 6 * color) * 64 + sq
int PP::feature(int sq, int piece) {
  int type = (piece & 7) - 2;
  int color = (piece >> 3) & 1;
  return (type + 6 * color) * 64 + sq;
}

void PP::load() {
#ifdef HAS_PPFILE
  weights = PPData;
  bias = weights[FEATURES * FEATURES];
#endif
}

// Full recomputation of the white-perspective pair sum (no bias).
int PP::pairsum(const U64 *Bitboards, const int *pieces) {
  int feats[32];
  int n = 0;
  U64 occ = Bitboards[0] | Bitboards[1];
  while (occ) {
    int sq = __builtin_ctzll(occ);
    occ &= occ - 1;
    feats[n++] = feature(sq, pieces[sq]);
  }

  long long sum = 0;
  for (int i = 0; i < n; i++) {
    const I16 *row = weights + feats[i] * FEATURES;
    for (int j = i + 1; j < n; j++) {
      sum += row[feats[j]];
    }
  }
  return static_cast<int>(sum);
}

void PP::init(Board &board) {
  ply = 0;
#ifdef HAS_PPFILE
  accumulator[0] = pairsum(board.Bitboards, board.pieces);
#else
  accumulator[0] = 0;
#endif
}

// Incrementally update the pair sum after a move.  The board passed in is
// already in the post-move state.
//
// Let C be all pieces except the one that just moved (now on `to`).  A move
// removes the mover's old feature (on `from`) and, on a capture, the enemy's
// feature (on `to`); it adds the mover's new feature (on `to`, promoted if
// applicable).  With A the added feature and R the removed feature(s):
//   delta = pairs(C, A) - pairs(C, R) - pairs(R, R)
void PP::make(int notation, Board &board) {
#ifdef HAS_PPFILE
  const U64 *Bitboards = board.Bitboards;
  const int *pieces = board.pieces;
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;    // mover's pre-move piece code
  int captured = (notation >> 17) & 7; // captured piece code (if any)
  bool iscapture = notation & (1 << 16);

  // C: every piece except the mover that now sits on `to`.
  int cfeats[32];
  int nc = 0;
  U64 occ = Bitboards[0] | Bitboards[1];
  while (occ) {
    int sq = __builtin_ctzll(occ);
    occ &= occ - 1;
    if (sq == to) {
      continue;
    }
    cfeats[nc++] = feature(sq, pieces[sq]);
  }

  // Added feature: mover after the move (post-move `pieces[to]` already
  // reflects any promotion).
  int added = feature(to, pieces[to]);
  // Removed feature: mover before the move, on its origin square.
  int removedfrom = (piece - 2 + 6 * color) * 64 + from;

  const I16 *rowadd = weights + added * FEATURES;
  const I16 *rowfrom = weights + removedfrom * FEATURES;

  long long delta = 0;
  for (int i = 0; i < nc; i++) {
    delta += rowadd[cfeats[i]] - rowfrom[cfeats[i]];
  }

  if (iscapture) {
    // Removed feature: captured enemy piece, on `to`.
    int removedcap = (captured - 2 + 6 * (color ^ 1)) * 64 + to;
    const I16 *rowcap = weights + removedcap * FEATURES;
    for (int i = 0; i < nc; i++) {
      delta -= rowcap[cfeats[i]];
    }
    // pairs(R, R): the mover and the captured piece coexisted pre-move.
    delta -= weights[removedfrom * FEATURES + removedcap];
  }

  ply++;
  accumulator[ply] = accumulator[ply - 1] + static_cast<int>(delta);
#endif
}

void PP::unmake(int, Board &) {
#ifdef HAS_PPFILE
  ply--;
#endif
}

int PP::evaluate(Board &board) {
  int color = board.position & 1;
  int sum = accumulator[ply];
  return (color == 0) ? (bias + sum) : (bias - sum);
}
