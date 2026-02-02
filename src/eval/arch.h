#pragma once

constexpr int inputbuckets = 12;
constexpr bool mirrored = true;
constexpr bool pairwise = true;
constexpr bool perspectivecrelu = false;
constexpr int mirrordivisor = mirrored ? 2 : 1;
constexpr int realbuckets = inputbuckets / mirrordivisor;
constexpr int L1size = 512;
constexpr int activatedL1size = L1size * (2 - pairwise);
constexpr int L2size = 8;
constexpr int activatedL2size = 8;
constexpr int L3size = 32;
constexpr int outputbuckets = 8;
constexpr int evalscale = 400;
constexpr int L1Q = 255;
constexpr int L2Q = 64;
constexpr int L3Q = 64;
constexpr int L4Q = 64;
constexpr int l1shiftbits = 9;
constexpr int totalL2Q = ((L1Q * L1Q) >> l1shiftbits) * L2Q;
constexpr int activatedL2Q = ((L1Q * L1Q) >> l1shiftbits) * ((L1Q * L1Q) >> l1shiftbits);
// clang-format off
constexpr int kingbuckets[64] = {
   0,  0,  2,  2,  3,  3,  1,  1,
   4,  4,  6,  6,  7,  7,  5,  5,
   8,  8,  8,  8,  9,  9,  9,  9,
   8,  8,  8,  8,  9,  9,  9,  9,
  10, 10, 10, 10, 11, 11, 11, 11,
  10, 10, 10, 10, 11, 11, 11, 11,
  10, 10, 10, 10, 11, 11, 11, 11,
  10, 10, 10, 10, 11, 11, 11, 11
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