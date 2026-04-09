#include "../board.h"

#pragma once

constexpr int threatcount = 6624;
constexpr int maxthreatchange = 64;
void initializethreats();

struct Threat {
  int attkr;
  int from;
  int to;
  int attkd;
};

struct ThreatDiff {
  Threat change[2][maxthreatchange];
  int added;
  int removed;
};

int threatindex(int color, int ksq, Threat t);
void findthreatdiff(int notation, const U64 *Bitboards, const int *pieces,
                    ThreatDiff &change);