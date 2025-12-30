#include "tt.h"
#include "consts.h"
#include <algorithm>
void TTentry::update(U64 hash, int gamelength, int depth, int ply, bool ttpv,
                     int score, int staticeval, int nodetype, int hashmove) {
  key = hash & (U64)0xFFFFFFFFFFFF0000;
  key |= (U64)((unsigned short int)staticeval);
  if (score > SCORE_MAX_EVAL) {
    score += ply;
  }
  if (score < -SCORE_MAX_EVAL) {
    score -= ply;
  }
  data = (U64)((unsigned short int)score);
  data |= (((U64)hashmove) << 16);
  data |= (((U64)nodetype) << 37);
  data |= (((U64)gamelength) << 39);
  data |= (((U64)depth) << 49);
  data |= (((U64)ttpv) << 55);
}
int TTentry::age(int gamelength) {
  return (gamelength - ((int)(data >> 39) & 1023));
}
int TTentry::hashmove() { return (int)(data >> 16) & 0x1FFFFF; }
int TTentry::depth() { return (int)(data >> 49) & 63; }
int TTentry::score(int ply) {
  int score = (int)(short int)(data & 0xFFFF);
  if (score > SCORE_MAX_EVAL) {
    return score - ply;
  }
  if (score < -SCORE_MAX_EVAL) {
    return score + ply;
  }
  return score;
}
int TTentry::staticeval() {
  return (int)(short int)(key & 0xFFFF);
}
int TTentry::nodetype() { return (int)(data >> 37) & 3; }
bool TTentry::isttPV() { return (data >> 55) & 1; }