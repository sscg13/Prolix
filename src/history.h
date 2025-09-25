#pragma once
class History {
  short int conthist[2][6][64][2][6][64];
  int quiethistory[2][6][64];
  int noisyhistory[2][6][6];
  const int quietlimit = 27000;
  const int noisylimit = 27000;
  const int contlimit = 27000;

public:
  void reset();
  int movescore(int move);
  int conthistscore(int priormove, int move);
  void updatemainhistory(int move, int bonus);
  void updateconthist(int priormove, int move, int bonus);
};
