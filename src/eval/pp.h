#include "../board.h"
#include "../consts.h"
#include "evalmethod.h"
#pragma once

// ====================================================================
//                    PP (Piece-Pair) Evaluation
// ====================================================================
// A weight for every unordered pair of pieces on the board.  Each piece
// is encoded as a feature (piece_type * 64 + square) in [0, 768):
//   piece_type in [0, 12)  -> 0-5 white, 6-11 black
//     (0 = pawn, 1 = bishop, 2 = ferz, 3 = knight, 4 = rook, 5 = king)
//   feat = piece_type * 64 + square
//
// The engine loads the decompressed weight table produced by the tuner's
// export_pp: a flat 768 * 768 array (symmetric, self-symmetric pairs are
// zero) followed by a single side-to-move bias.  For an unordered pair of
// features {fi, fj} the pair weight is weights[fi * 768 + fj].
//
// The tuner trains from the side-to-move perspective, and the weight table
// obeys the vertical symmetry w[i][j] = -w[flip(i)][flip(j)].  Because of
// this, the pair sum computed in a fixed (white) perspective, S, gives the
// side-to-move evaluation directly:
//   white to move:  eval = bias + S
//   black to move:  eval = bias - S   (mirroring negates every pair)
// So the running pair sum is perspective-independent and can be updated
// incrementally through make / unmake, then signed at evaluate time.
// ====================================================================

class PP : public EvalMethod {
  static constexpr int FEATURES = 12 * 64; // 768
  // Max search nesting depth; matches the NNUE accumulator stack bound.
  static constexpr int STACKSIZE = 128;

  const I16 *weights = nullptr; // decompressed 768 * 768 pair table
  int bias = 0;

  int accumulator[STACKSIZE]; // white-perspective pair sum per ply
  int ply = 0;

  static int feature(int sq, int piece);
  int pairsum(const U64 *Bitboards, const int *pieces);

public:
  void load();
  void init(Board &board) override;
  void make(int notation, Board &board) override;
  void unmake(int notation, Board &board) override;
  int evaluate(Board &board) override;
};
