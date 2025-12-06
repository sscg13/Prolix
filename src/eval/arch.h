#pragma once

constexpr int inputbuckets = 32;
constexpr bool mirrored = true;
constexpr int mirrordivisor = mirrored ? 2 : 1;
constexpr int realbuckets = inputbuckets / mirrordivisor;
constexpr int L1size = 1792;
constexpr int L2size = 16;

constexpr int L3size = 32;
constexpr int outputbuckets = 8;
constexpr int evalscale = 400;
constexpr int L1Qbits = 8;
constexpr int L1Q = (1 << L1Qbits) - 1;
constexpr int L3Qbits = 6;
constexpr int L3Q = (1 << L3Qbits);
constexpr int L4Q = 64;
constexpr int pairwiseshiftbits = 9;
// clang-format off
constexpr int kingbuckets[64] = {
   0,  2,  4,  6,  7,  5,  3,  1,
   8, 10, 12, 14, 15, 13, 11,  9,
  16, 18, 20, 22, 23, 21, 19, 17,
  16, 18, 20, 22, 23, 21, 19, 17,
  24, 24, 26, 26, 27, 27, 25, 25,
  24, 24, 26, 26, 27, 27, 25, 25,
  28, 28, 30, 30, 31, 31, 29, 29,
  28, 28, 30, 30, 31, 31, 29, 29,
};
// clang-format on
constexpr int material[6] = {1, 1, 1, 1, 1, 0};
constexpr int bucketdivisor = 32 / outputbuckets;
#define MULTI_LAYER
#ifdef MULTI_LAYER
constexpr bool multilayer = true;
constexpr int L2Qbits = 7;
#define DUAL_ACTIVATION
#else
constexpr bool multilayer = false;
constexpr int L2Qbits = 6;
// #define SINGLE_LAYER_CRELU
#endif
constexpr int L2Q = (1 << L2Qbits);
#ifdef DUAL_ACTIVATION
constexpr int L2shiftbits = 2 * L1Qbits - pairwiseshiftbits + L2Qbits - L3Qbits;
constexpr int activatedL2size = L2size * 2;
constexpr int totalL2Q = L3Q;
constexpr int totalL3Q = L3Q * L3Q * L3Q;
constexpr int totalL4Q = totalL3Q * L4Q;
#else
constexpr int L2shiftbits = 0;
constexpr int activatedL2size = L2size;
constexpr int totalL2Q = ((L1Q * L1Q) >> pairwiseshiftbits) * L2Q;
constexpr int totalL3Q = totalL2Q * L3Q;
constexpr int totalL4Q = totalL3Q * L4Q;
#endif
// #define THREAT_INPUTS