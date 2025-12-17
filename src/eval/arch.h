#pragma once

constexpr int inputbuckets = 8;
constexpr bool mirrored = true;
constexpr bool pairwise = true;
constexpr bool perspectivecrelu = false;
constexpr int mirrordivisor = mirrored ? 2 : 1;
constexpr int realbuckets = inputbuckets / mirrordivisor;
constexpr int L1size = 768;
constexpr int activatedL1size = L1size * (2 - pairwise);
constexpr int L2size = 8;
constexpr int L3size = 32;
constexpr int outputbuckets = 8;
constexpr int evalscale = 400;
constexpr int L1Qbits = 8;
constexpr int L1Q = (1 << L1Qbits) - 1;
constexpr int L2Qbits = 6;
constexpr int L2Q = (1 << L2Qbits);
constexpr int L3Qbits = 6;
constexpr int L3Q = (1 << L3Qbits);
constexpr int L4Q = 64;
constexpr int l1shiftbits = 9;
// clang-format off
constexpr int kingbuckets[64] = {
   0,  0,  2,  2,  3,  3,  1,  1,
   0,  0,  2,  2,  3,  3,  1,  1,
   4,  4,  4,  4,  5,  5,  5,  5,
   4,  4,  4,  4,  5,  5,  5,  5,
   6,  6,  6,  6,  7,  7,  7,  7,
   6,  6,  6,  6,  7,  7,  7,  7,
   6,  6,  6,  6,  7,  7,  7,  7,
   6,  6,  6,  6,  7,  7,  7,  7
};
// clang-format on
constexpr int material[6] = {1, 1, 1, 1, 1, 0};
constexpr int bucketdivisor = 32 / outputbuckets;
// #define MULTI_LAYER
#ifdef MULTI_LAYER
constexpr bool multilayer = true;
// #define DUAL_ACTIVATION
#else
constexpr bool multilayer = false;
#endif
#ifdef DUAL_ACTIVATION
constexpr int L2shiftbits = 2 * L1Qbits - l1shiftbits + L2Qbits - L3Qbits;
constexpr int activatedL2size = L2size * 2;
constexpr int totalL2Q = L3Q;
constexpr int totalL3Q = L3Q * L3Q * L3Q;
constexpr int totalL4Q = totalL3Q * L4Q;
#else
constexpr int L2shiftbits = 0;
constexpr int activatedL2size = L2size;
constexpr int totalL2Q = ((L1Q * L1Q) >> l1shiftbits) * L2Q;
constexpr int totalL3Q = totalL2Q * L3Q;
constexpr int totalL4Q = totalL3Q * L4Q;
#endif
// #define THREAT_INPUTS