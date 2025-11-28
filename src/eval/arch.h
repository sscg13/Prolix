constexpr int inputbuckets = 4;
constexpr bool mirrored = true;
constexpr int mirrordivisor = mirrored ? 2 : 1;
constexpr int realbuckets = inputbuckets / mirrordivisor;
constexpr int L1size = 1024;
constexpr int L2size = 8;
constexpr int L3size = 32;
constexpr int outputbuckets = 8;
constexpr int evalscale = 400;
constexpr int L1Q = 255;
constexpr int pairwiseshiftbits = 9;
// clang-format off
constexpr int kingbuckets[64] = {
  0, 0, 0, 0, 1, 1, 1, 1,
  0, 0, 0, 0, 1, 1, 1, 1,
  0, 0, 0, 0, 1, 1, 1, 1,
  2, 2, 2, 2, 3, 3, 3, 3,
  2, 2, 2, 2, 3, 3, 3, 3,
  2, 2, 2, 2, 3, 3, 3, 3,
  2, 2, 2, 2, 3, 3, 3, 3,
  2, 2, 2, 2, 3, 3, 3, 3
};
// clang-format on
constexpr int material[6] = {1, 1, 1, 1, 1, 0};
constexpr int bucketdivisor = 32 / outputbuckets;
constexpr bool dualactivation = false;
// #define MULTI_LAYER
#ifdef MULTI_LAYER
constexpr bool multilayer = true;
constexpr int L2Q = 128;
constexpr int L3Q = 64;
constexpr int L4Q = 64;
#else
constexpr bool multilayer = false;
constexpr int L2Q = 64;
#endif
// #define THREAT_INPUTS