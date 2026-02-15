#pragma once

constexpr int inputbuckets = 32;
constexpr bool mirrored = true;
constexpr bool pairwise = true;
constexpr bool perspectivecrelu = false;
constexpr int mirrordivisor = mirrored ? 2 : 1;
constexpr int realbuckets = inputbuckets / mirrordivisor;
constexpr int L1size = 768;
constexpr int activatedL1size = L1size * (2 - pairwise);
constexpr int L2size = 16;
constexpr int activatedL2size = 16;
constexpr int L3size = 32;
constexpr int outputbuckets = 8;
constexpr int evalscale = 400;
constexpr int L1Q = 255;
constexpr int L2Q = 64;
constexpr int L3Q = 128;
constexpr int L4Q = 64;
constexpr int l1shiftbits = 9;
constexpr int l2shiftbits = 8;
constexpr int totalL2Q = ((L1Q * L1Q) >> l1shiftbits) * L2Q;
constexpr int activatedL2Q = (totalL2Q * totalL2Q) >> l2shiftbits;
// clang-format off
constexpr int kingbuckets[64] = {
   0,  2,  4,  6,  7,  5,  3,  1,
   8, 10, 12, 14, 15, 13, 11,  9,
  16, 16, 18, 18, 19, 19, 17, 17,
  20, 20, 22, 22, 23, 23, 21, 21,
  24, 24, 26, 26, 27, 27, 25, 25,
  24, 24, 26, 26, 27, 27, 25, 25,
  28, 28, 30, 30, 31, 31, 29, 29,
  28, 28, 30, 30, 31, 31, 29, 29
};
// clang-format on
constexpr int material[6] = {1, 1, 1, 1, 1, 0};
constexpr int bucketdivisor = 32 / outputbuckets;
#define MULTI_LAYER
#ifdef MULTI_LAYER
constexpr bool multilayer = true;
#else
constexpr bool multilayer = false;
#endif
// #define THREAT_INPUTS