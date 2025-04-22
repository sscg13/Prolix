#include <cstdint>
#pragma once
using U64 = uint64_t;
const U64 FileA = 0x0101010101010101;
const U64 FileB = FileA << 1;
const U64 FileC = FileA << 2;
const U64 FileD = FileA << 3;
const U64 FileE = FileA << 4;
const U64 FileF = FileA << 5;
const U64 FileG = FileA << 6;
const U64 FileH = FileA << 7;
const U64 Rank1 = 0xFF;
const U64 Rank2 = Rank1 << 8;
const U64 Rank3 = Rank1 << 16;
const U64 Rank4 = Rank1 << 24;
const U64 Rank5 = Rank1 << 32;
const U64 Rank6 = Rank1 << 40;
const U64 Rank7 = Rank1 << 48;
const U64 Rank8 = Rank1 << 56;
const U64 Files[8] = {FileA, FileB, FileC, FileD, FileE, FileF, FileG, FileH};
const U64 Ranks[8] = {Rank1, Rank2, Rank3, Rank4, Rank5, Rank6, Rank7, Rank8};
constexpr int maxmaxdepth = 32;