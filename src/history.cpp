#include "history.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>

constexpr int pawncorrectionbonuslimit = 256;
constexpr int pawncorrectionweight = 150;
constexpr int pawncorrectionscale = 1024;

int pawncorrectionbonus(int searchEval, int correctedEval, int depth) {
  return std::clamp((searchEval - correctedEval) * depth * 12 / 128,
                    -pawncorrectionbonuslimit, pawncorrectionbonuslimit);
}

void History::reset() {
  memset(quiethistory, 0, sizeof(quiethistory));
  memset(noisyhistory, 0, sizeof(noisyhistory));
  memset(conthist, 0, sizeof(conthist));
  memset(pawncorrection, 0, sizeof(pawncorrection));
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
    noisyhistory[color][piece - 2][captured - 2] +=
        (bonus > 0)
            ? (bonus - (bonus * noisyhistory[color][piece - 2][captured - 2]) /
                           noisylimit)
            : bonus;
  } else {
    quiethistory[color][piece - 2][to] +=
        (bonus > 0) ? (bonus - (bonus * quiethistory[color][piece - 2][to]) /
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
    conthist[priorcolor][priorpiece - 2][priorto][color][piece - 2][to] +=
        (bonus > 0)
            ? (bonus - (bonus * conthist[priorcolor][priorpiece - 2][priorto]
                                        [color][piece - 2][to]) /
                           contlimit)
            : bonus;
  }
}

void History::updatepawncorrection(U64 pawnkey, int color, int bonus) {
  int index = pawnkey & (pawncorrectionsize - 1);
  int clampedbonus = std::max(-pawncorrectionlimit,
                              std::min(pawncorrectionlimit, bonus));
  int value = pawncorrection[color][index];
  value += clampedbonus -
           value * std::abs(clampedbonus) / pawncorrectionlimit;
  pawncorrection[color][index] = value;
}

int History::pawncorrectionscore(U64 pawnkey, int color) {
  return pawncorrection[color][pawnkey & (pawncorrectionsize - 1)] *
         pawncorrectionweight / pawncorrectionscale;
}
