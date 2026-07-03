#include "../board.h"
#include "../consts.h"
#include "evalmethod.h"
#pragma once

// ====================================================================
//                 PPxK (Piece-Pair x King) Evaluation
// ====================================================================
// A king-modulated refinement of the piece-pair idea.  Pieces are encoded as
// feat = piece_type * 64 + sq in [0, 768) (0-5 own, 6-11 enemy, K = 5 / 11).
// For a given perspective:
//   I[sq] = sum over ordered piece pairs (a, b), a != b, with sq_a == sq of
//           W[feat_a * 768 + feat_b]
//   reduced = sum over sq of K[king_sq * 64 + sq] * I[sq]
//   eval = reduced_stm - reduced_nstm + bias
//
// Like NNUE this is dual-perspective, so it is maintained incrementally with
// two fixed-frame accumulators that do NOT depend on the side to move:
//   accw[sq] : white's own frame (white = own 0-5, board squares unflipped)
//   accb[sq] : black's own frame (black = own 0-5, squares mirrored ^56)
// I[sq] depends only on piece placement, not on the king: the king enters
// solely through the K-row dot product at evaluate() time.  Hence:
//   reduced_white = sum_sq K[wking * 64 + sq] * accw[sq]
//   reduced_black = sum_sq K[(bking ^ 56) * 64 + sq] * accb[sq]
//   eval = bias + (white to move ? reduced_white - reduced_black
//                                : reduced_black - reduced_white)
// King moves need no special refresh, and null moves (no piece change) stay
// correct automatically.
//
// The engine loads the tuner's raw ppxk weights (no symmetry compression):
//   W    : weights[0 .. 768*768)
//   K    : weights[768*768 .. 768*768 + 64*64)
//   bias : weights[768*768 + 64*64]
//
// All weights are exported scaled by the tuner factor k (SCALE below), so the
// bilinear reduced term is scaled by k*k while the bias is scaled by k; the
// reduced term is divided by k to bring everything into engine (centipawn)
// units.  SCALE must match the k used when the weights were tuned.
// ====================================================================

class PPXK : public EvalMethod {
  static constexpr int FEATURES = 12 * 64; // 768
  static constexpr int NUM_PP = FEATURES * FEATURES;
  static constexpr int NUM_K = 64 * 64;
  static constexpr int SCALE = 400; // tuner sigmoid / export factor k
  // Max search nesting depth; matches the NNUE accumulator stack bound.
  static constexpr int STACKSIZE = 128;

  const I16 *W = nullptr; // [768 * 768] piece-pair weights
  const I16 *K = nullptr; // [64 * 64] king x square weights
  int bias = 0;

  // Per-ply fixed-frame accumulators (I[sq] for each perspective).
  int accw[STACKSIZE][64]; // white-frame per-square pair sums
  int accb[STACKSIZE][64]; // black-frame per-square pair sums
  int ply = 0;

  struct Piece {
    int fw, wsq; // white-frame feature and square
    int fb, bsq; // black-frame feature and square
  };
  static Piece makepiece(int color, int type, int sq);
  void contribute(int *cw, int *cb, int coeff, const Piece &a, const Piece &b);

public:
  void load();
  void init(Board &board) override;
  void make(int notation, Board &board) override;
  void unmake(int notation, Board &board) override;
  int evaluate(Board &board) override;
};
