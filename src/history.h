#include "consts.h"
#pragma once
int pawncorrectionbonus(int searchEval, int correctedEval, int depth);
class History {
  static constexpr int pawncorrectionbits = 14;
  static constexpr int pawncorrectionsize = 1 << pawncorrectionbits;
  static constexpr int pawncorrectionlimit = 1024;
  short int conthist[2][6][64][2][6][64];
  int quiethistory[2][6][64];
  int noisyhistory[2][6][6];
  short int pawncorrection[2][pawncorrectionsize];
  const int quietlimit = 27000;
  const int noisylimit = 27000;
  const int contlimit = 27000;

public:
  void reset();
  int movescore(int move);
  int conthistscore(int priormove, int move);
  int pawncorrectionscore(U64 pawnkey, int color);
  void updatemainhistory(int move, int bonus);
  void updateconthist(int priormove, int move, int bonus);
  void updatepawncorrection(U64 pawnkey, int color, int bonus);
};
