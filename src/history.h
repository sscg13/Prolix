#pragma once
class History {
  int quiethistory[2][6][64];
  int noisyhistory[2][6][6];
  const int quietlimit = 27000;
  const int noisylimit = 27000;

public:
  void reset();
  int movescore(int move);
  void updatenoisyhistory(int move, int bonus);
  void updatequiethistory(int move, int bonus);
};
