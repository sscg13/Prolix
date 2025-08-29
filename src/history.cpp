#include "history.h"
#include <cstring>
#include <cmath>

void History::reset() {
  memset(quiethistory, 0, sizeof(quiethistory));
  memset(noisyhistory, 0, sizeof(noisyhistory));
  memset(conthist, 0, sizeof(conthist));
}

int History::movescore(int move) {
  int color = (move >> 12) & 1;
  int to = (move >> 6) & 63;
  int piece = (move >> 13) & 7;
  int captured = (move >> 17) & 7;
  if (captured) {
    return 30000 + 10000 * captured +
           noisyhistory[color][piece - 2][captured - 2];
  } else {
    return quiethistory[color][piece - 2][to];
  }
}

int History::conthistscore(int priormove, int move) {
  int priorcolor = (priormove >> 12) & 1;
  int priorto = (priormove >> 6) & 63;
  int priorpiece = (priormove >> 13) & 7;
  int color = (move >> 12) & 1;
  int to = (move >> 6) & 63;
  int piece = (move >> 13) & 7;
  int captured = (move >> 17) & 7;
  if (captured || priormove == 0) {
    return 0;
  } else {
    return conthist[priorcolor][priorpiece - 2][priorto][color][piece - 2][to];
  }
}

void History::updatemainhistory(int move, int bonus) {
  int color = (move >> 12) & 1;
  int to = (move >> 6) & 63;
  int piece = (move >> 13) & 7;
  int captured = (move >> 17) & 7;
  if (captured > 0) {
    int& value = noisyhistory[color][piece - 2][captured - 2];
    value +=
        (bonus * value > 0)
            ? (bonus - (bonus * std::abs(value)) /
                           noisylimit)
            : bonus;
  } else {
    int& value = quiethistory[color][piece - 2][to];
    value +=
        (bonus * value > 0) ? (bonus - (bonus * std::abs(value)) /
                                   quietlimit)
                    : bonus;
  }
}

void History::updateconthist(int priormove, int move, int bonus) {
  int priorcolor = (priormove >> 12) & 1;
  int priorto = (priormove >> 6) & 63;
  int priorpiece = (priormove >> 13) & 7;
  int color = (move >> 12) & 1;
  int to = (move >> 6) & 63;
  int piece = (move >> 13) & 7;
  int captured = (move >> 17) & 7;
  if (!captured && priormove) {
    int &value = conthist[priorcolor][priorpiece - 2][priorto][color][piece - 2][to];
    value +=
        (bonus * value > 0)
            ? (bonus - (bonus * std::abs(value)) /
                           contlimit)
            : bonus;
  }
}