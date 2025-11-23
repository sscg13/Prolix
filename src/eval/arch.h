constexpr int inputbuckets = 4;
constexpr bool mirrored = true;
constexpr int mirrordivisor = mirrored ? 2 : 1;
constexpr int realbuckets = inputbuckets / mirrordivisor;
constexpr int nnuesize = 768;
constexpr int outputbuckets = 8;
constexpr int evalscale = 400;
constexpr int evalQA = 255;
constexpr int evalQB = 64;
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
constexpr int nnuefilesize = (realbuckets * 1536 * nnuesize + 2 * nnuesize +
                              4 * nnuesize * outputbuckets + 2 * outputbuckets);