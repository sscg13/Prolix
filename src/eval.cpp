#include "eval.h"

// Resolve a requested eval level to one that is actually available in this
// build.  Levels 0-3 and 5 are always present; 4 (KP), 6 (PP), 7 (PPxK) and
// 8 (NNUE) each require their weight file.  Anything unavailable (or out of
// range) falls back to the highest level that exists (`topevallevel`).  This is
// the single place the HAS_*FILE macros gate the level set.
int resolveevallevel(int requested) {
  bool ok = (requested >= 0 && requested <= 3) || requested == 5;
#ifdef HAS_KPFILE
  ok = ok || requested == 4;
#endif
#ifdef HAS_PPFILE
  ok = ok || requested == 6;
#endif
#ifdef HAS_PPXKFILE
  ok = ok || requested == 7;
#endif
#ifdef HAS_EVALFILE
  ok = ok || requested == 8;
#endif
  return ok ? requested : topevallevel;
}

const char *evallevelname(int level) {
  switch (level) {
  case 0:
    return "Random";
  case 1:
    return "Material Count + Random";
  case 2:
    return "Piece Rank + Piece File";
  case 3:
    return "Piece Square Table";
  case 4:
    return "King Bucketed Piece Square Table";
  case 6:
    return "Piece Pair";
  case 7:
    return "Piece Pair x King";
  case 8:
    return "NNUE";
  default: // 5
    return "Piece Square Table + Partially Safe Mobility";
  }
}

void Evaluator::setlevel(int requested) {
  level = resolveevallevel(requested);
  active = methods[level];
}

void Evaluator::load(EvalParams &params) {
  methods[0] = &randomeval;
  methods[1] = &material;
  methods[2] = &PFR;
  methods[3] = &PST;
  methods[4] = &kp;
  methods[5] = &hce;
  methods[6] = &pp;
  methods[7] = &ppxk;
  methods[8] = &EUNN;

  PFR.load();
#ifdef HAS_KPFILE
  kp.load();
#endif
#ifdef HAS_PPFILE
  pp.load();
#endif
#ifdef HAS_PPXKFILE
  ppxk.load();
#endif
#ifdef HAS_EVALFILE
  EUNN.load(params.nnueweights);
#endif
}

void Evaluator::init(Board &Bitboards) { active->init(Bitboards); }

void Evaluator::make(int notation, Board &Bitboards) {
  active->make(notation, Bitboards);
}

void Evaluator::unmake(int notation, Board &Bitboards) {
  active->unmake(notation, Bitboards);
}

int Evaluator::evaluate(Board &Bitboards) {
  return active->evaluate(Bitboards);
}
