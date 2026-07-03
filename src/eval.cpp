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

int Evaluator::evaluate(Board &Bitboards) { return active->evaluate(Bitboards); }
