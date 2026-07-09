#define INCBIN_PREFIX
#include "ppxk.h"
#include "../external/incbin/incbin.h"
#include <cstring>

#ifdef HAS_PPXKFILE
INCBIN(I16, PPXK, PPXKfile);
#endif

void PPXK::load() {
#ifdef HAS_PPXKFILE
  const I16 *weights = PPXKData;
  W = weights;
  K = weights + NUM_PP;
  bias = weights[NUM_PP + NUM_K];
#endif
}

// A piece (color, type, board square) in both fixed frames.
//   white frame: white pieces own (0-5), squares unflipped.
//   black frame: black pieces own (0-5), squares mirrored (^56).
PPXK::Piece PPXK::makepiece(int color, int type, int sq) {
  Piece p;
  p.fw = (type + 6 * color) * 64 + sq;
  p.wsq = sq;
  p.fb = (type + 6 * (color ^ 1)) * 64 + (sq ^ 56);
  p.bsq = sq ^ 56;
  return p;
}

// Apply the (unordered) pair {a, b} to the target accumulators, scaled by
// coeff.  Each pair feeds I[sq_a] via W[a][b] and I[sq_b] via W[b][a], per
// frame.
void PPXK::contribute(int *cw, int *cb, int coeff, const Piece &a,
                      const Piece &b) {
  cw[a.wsq] += coeff * W[a.fw * FEATURES + b.fw];
  cw[b.wsq] += coeff * W[b.fw * FEATURES + a.fw];
  cb[a.bsq] += coeff * W[a.fb * FEATURES + b.fb];
  cb[b.bsq] += coeff * W[b.fb * FEATURES + a.fb];
}

void PPXK::init(Board &board) {
#ifdef HAS_PPXKFILE
  const U64 *Bitboards = board.Bitboards;
  const int *pieces = board.pieces;
  ply = 0;
  int *cw = accw[0];
  int *cb = accb[0];
  for (int sq = 0; sq < 64; sq++) {
    cw[sq] = 0;
    cb[sq] = 0;
  }
  Piece list[32];
  int n = 0;
  U64 occ = Bitboards[0] | Bitboards[1];
  while (occ) {
    int sq = __builtin_ctzll(occ);
    occ &= occ - 1;
    int code = pieces[sq];
    list[n++] = makepiece((code >> 3) & 1, (code & 7) - 2, sq);
  }
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      contribute(cw, cb, 1, list[i], list[j]);
    }
  }
#endif
}

// Push a new ply: copy the parent accumulators and apply the move's delta.
// C is the untouched pieces (post-move occupancy minus {from, to}); A is the
// mover's new placement; R is the mover's old placement plus any captured
// piece.  This is the piece-pair delta scattered into both frames:
//   delta = pairs(C, A) - pairs(C, R) - pairs(R, R)
void PPXK::make(int notation, Board &board) {
#ifdef HAS_PPXKFILE
  const U64 *Bitboards = board.Bitboards;
  const int *pieces = board.pieces;
  int *cw = accw[ply + 1];
  int *cb = accb[ply + 1];
  std::memcpy(cw, accw[ply], 64 * sizeof(int));
  std::memcpy(cb, accb[ply], 64 * sizeof(int));
  ply++;

  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;    // mover's pre-move piece code
  int captured = (notation >> 17) & 7; // captured piece code (if any)
  bool iscapture = notation & (1 << 16);
  bool promotion = notation & (1 << 20);

  Piece added = makepiece(color, promotion ? 2 : piece - 2, to);
  Piece removedfrom = makepiece(color, piece - 2, from);
  Piece removedcap;
  if (iscapture) {
    removedcap = makepiece(color ^ 1, captured - 2, to);
  }

  U64 occ = Bitboards[0] | Bitboards[1];
  while (occ) {
    int sq = __builtin_ctzll(occ);
    occ &= occ - 1;
    if (sq == from || sq == to) {
      continue;
    }
    int code = pieces[sq];
    Piece c = makepiece((code >> 3) & 1, (code & 7) - 2, sq);
    contribute(cw, cb, 1, added, c);
    contribute(cw, cb, -1, removedfrom, c);
    if (iscapture) {
      contribute(cw, cb, -1, removedcap, c);
    }
  }
  if (iscapture) {
    contribute(cw, cb, -1, removedfrom, removedcap);
  }
#endif
}

void PPXK::unmake(int, Board &) {
#ifdef HAS_PPXKFILE
  ply--;
#endif
}

int PPXK::evaluate(Board &board) {
  int color = board.position & 1;
  const U64 *Bitboards = board.Bitboards;
  int wksq = __builtin_ctzll(Bitboards[7] & Bitboards[0]);
  int bksq = __builtin_ctzll(Bitboards[7] & Bitboards[1]);
  int bkf = bksq ^ 56; // black king in black's frame

  const int *cw = accw[ply];
  const int *cb = accb[ply];
  long long reducedwhite = 0;
  long long reducedblack = 0;
  const I16 *kw = K + wksq * 64;
  const I16 *kb = K + bkf * 64;
  for (int sq = 0; sq < 64; sq++) {
    reducedwhite += (long long)kw[sq] * cw[sq];
    reducedblack += (long long)kb[sq] * cb[sq];
  }

  long long diff = (color == 0) ? (reducedwhite - reducedblack)
                                : (reducedblack - reducedwhite);
  return bias + (int)(diff / SCALE);
}
