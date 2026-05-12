#include "eval.h"
void Evaluator::load(EvalParams &params) {
  PFR.load();
#ifdef HAS_KPFILE
  kp.load();
#endif
#ifdef HAS_EVALFILE
  EUNN.load(params.nnueweights);
#endif
}
void Evaluator::init(Board &Bitboards) {
  switch (level) {
  case 1:
    break;
  case 2:
    break;
  case 3:
    break;
  case 4:
    break;
  case 5:
    break;
  default:
#ifdef HAS_EVALFILE
    EUNN.initialize(Bitboards.Bitboards, Bitboards.pieces);
#endif
    break;
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
  case 4:
    break;
  case 5:
    break;
  default:
#ifdef HAS_EVALFILE
    EUNN.make(notation, Bitboards.Bitboards, Bitboards.pieces);
#endif
    break;
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
  case 4:
    break;
  case 5:
    break;
  default:
#ifdef HAS_EVALFILE
    EUNN.unmake(notation, Bitboards.Bitboards, Bitboards.pieces);
#endif
    break;
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
    return PST.evaluate(color, Bitboards.Bitboards, Bitboards.pieces);
  case 4:
#ifdef HAS_KPFILE
    return kp.evaluate(color, Bitboards.Bitboards, Bitboards.pieces);
#else
    return Bitboards.evaluate(color);
#endif
  case 5:
    return Bitboards.evaluate(color);
  default:
#ifdef HAS_EVALFILE
    return EUNN.evaluate(color, Bitboards.Bitboards, Bitboards.pieces);
#else
    return Bitboards.evaluate(color);
#endif
  }
}
