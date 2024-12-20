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

void History::reset() {
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 64; j++) {
      quiethistory[0][i][j] = 0;
      quiethistory[1][i][j] = 0;
    }
    for (int j = 0; j < 6; j++) {
      noisyhistory[0][i][j] = 0;
      noisyhistory[1][i][j] = 0;
    }
  }
}

int History::movescore(int move) {
  int color = (move >> 12) & 1;
  int to = (move >> 6) & 63;
  int piece = (move >> 13) & 7;
  int captured = (move >> 17) & 7;
  if (captured) {
    return 10000 * captured + noisyhistory[color][piece - 2][captured - 2];
  } else {
    return quiethistory[color][piece - 2][to];
  }
}

void History::updatenoisyhistory(int move, int bonus) {
  int piece = (move >> 13) & 7;
  int captured = (move >> 17) & 7;
  int color = (move >> 12) & 1;
  noisyhistory[color][piece - 2][captured - 2] +=
      (bonus > 0)
          ? (bonus - (bonus * noisyhistory[color][piece - 2][captured - 2]) /
                         noisylimit)
          : bonus;
}

void History::updatequiethistory(int move, int bonus) {
  int target = (move >> 6) & 63;
  int piece = (move >> 13) & 7;
  int color = (move >> 12) & 1;
  quiethistory[color][piece - 2][target] +=
      (bonus > 0) ? (bonus - (bonus * quiethistory[color][piece - 2][target]) /
                                 quietlimit)
                  : bonus;
}