#include "tt.h"
#include "consts.h"
#include <algorithm>
void TTentry::update(U64 hash, int gamelength, int depth, int ply, bool ttpv,
                     int score, int nodetype, int hashmove) {
  key = hash;
  if (score > SCORE_MAX_EVAL) {
    score += ply;
  }
  if (score < -SCORE_MAX_EVAL) {
    score -= ply;
  }
  data = (U64)((unsigned short int)score);
  data |= (((U64)hashmove) << 16);
  data |= (((U64)nodetype) << 42);
  data |= (((U64)gamelength) << 44);
  data |= (((U64)depth) << 54);
  data |= (((U64)ttpv) << 60);
}
int TTentry::age(int gamelength) {
  return (gamelength - ((int)(data >> 44) & 1023));
}
int TTentry::hashmove() { return (int)(data >> 16) & 0x03FFFFFF; }
int TTentry::depth() { return (int)(data >> 54) & 63; }
int TTentry::score(int ply) {
  int score = (int)(short int)(data & 0x000000000000FFFF);
  if (score > SCORE_MAX_EVAL) {
    return score - ply;
  }
  if (score < -SCORE_MAX_EVAL) {
    return score + ply;
  }
  return score;
}
int TTentry::nodetype() { return (int)(data >> 42) & 3; }
bool TTentry::isttPV() { return (data >> 60) & 1; }