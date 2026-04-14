#include "eval.h"
void Evaluator::load(EvalParams &params) {
  PFR.load();
  EUNN.load(params.nnueweights);
}
void Evaluator::init(Board &Bitboards) {
  switch (level) {
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  default:
    EUNN.initialize(Bitboards.Bitboards, Bitboards.pieces);
  }
}
void Evaluator::make(int notation, Board &Bitboards) {
  switch (level) {
  case 0:
    break;
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  default:
    EUNN.make(notation, Bitboards.Bitboards, Bitboards.pieces);
  }
}
void Evaluator::unmake(int notation, Board &Bitboards) {
  switch (level) {
  case 0:
    break;
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  default:
    EUNN.unmake(notation, Bitboards.Bitboards, Bitboards.pieces);
  }
}
int Evaluator::evaluate(Board &Bitboards) {
  int color = Bitboards.position & 1;
  switch (level) {
  case 0:
    return Bitboards.zobristhash % 64;
  case 1:
    return Bitboards.piecevaluediff(color);
  case 2:
    return PFR.evaluate(color, Bitboards.Bitboards, Bitboards.pieces);
  case 3:
    return Bitboards.evaluate(color);
  default:
    return EUNN.evaluate(color, Bitboards.Bitboards, Bitboards.pieces);
  }
}