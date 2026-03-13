#include "../board.h"

#pragma once

constexpr int threatcount = 6624;
constexpr int maxthreats = 96;
void initializethreats();

struct Threat {
    int attkr;
    int from;
    int to;
    int attkd;
};


int threatindex(int color, int ksq, Threat t);
void findthreatdiff(int notation, const U64* Bitboards, const int* pieces, Threat* diff);