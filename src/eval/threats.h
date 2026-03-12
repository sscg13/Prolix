#include "../board.h"

#pragma once

constexpr int threatcount = 6624;
void initializethreats();

struct Threat {
    int attkr;
    int from;
    int to;
    int attkd;
};


int threatindex(int color, int ksq, Threat t);