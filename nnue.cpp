#include "nnue.h"
#include <cmath>
#include <fstream>
#define INCBIN_PREFIX
#include "external/incbin/incbin.h"

INCBIN(char, NNUE, EUNNfile);
int screlu(short int x) {
  return std::pow(std::max(std::min((int)x, 255), 0), 2);
}
int featureindex(int bucket, int piece, int color, int square) {
  return 64 * piece +
         ((56 * color) ^ square ^ (7 * (mirrored && (bucket % 2 == 1))));
}

void NNUE::loaddefaultnet() {
  int offset = 0;
  for (int k = 0; k < realbuckets; k++) {
    for (int i = 0; i < 768; i++) {
      int piece = i / 64;
      int square = i % 64;
      int convert[12] = {0, 3, 1, 4, 2, 5, 6, 9, 7, 10, 8, 11};
      for (int j = 0; j < nnuesize; j++) {
        short int weight = 256 * (short int)(NNUEData[offset + 1]) +
                           (short int)(unsigned char)(NNUEData[offset]);
        nnuelayer1[k][64 * convert[piece] + square][j] = weight;
        offset += 2;
      }
    }
  }
  for (int i = 0; i < nnuesize; i++) {
    short int bias = 256 * (short int)(NNUEData[offset + 1]) +
                     (short int)(unsigned char)(NNUEData[offset]);
    layer1bias[i] = bias;
    offset += 2;
  }
  for (int j = 0; j < outputbuckets; j++) {
    for (int i = 0; i < nnuesize; i++) {
      short int weight = 256 * (short int)(NNUEData[offset + 1]) +
                         (short int)(unsigned char)(NNUEData[offset]);
      ourlayer2[j][i] = (int)weight;
      offset += 2;
    }
    for (int i = 0; i < nnuesize; i++) {
      short int weight = 256 * (short int)(NNUEData[offset + 1]) +
                         (short int)(unsigned char)(NNUEData[offset]);
      theirlayer2[j][i] = (int)weight;
      offset += 2;
    }
  }
  for (int j = 0; j < outputbuckets; j++) {
    short int base = 256 * (short int)(NNUEData[offset + 1]) +
                     (short int)(unsigned char)(NNUEData[offset]);
    finalbias[j] = base;
    offset += 2;
  }
}
void NNUE::readnnuefile(std::string file) {
  std::ifstream nnueweights;
  nnueweights.open(file, std::ifstream::binary);
  char *weights = new char[nnuefilesize];
  nnueweights.read(weights, nnuefilesize);
  int offset = 0;
  for (int k = 0; k < realbuckets; k++) {
    for (int i = 0; i < 768; i++) {
      int piece = i / 64;
      int square = i % 64;
      int convert[12] = {0, 3, 1, 4, 2, 5, 6, 9, 7, 10, 8, 11};
      for (int j = 0; j < nnuesize; j++) {
        short int weight = 256 * (short int)(weights[offset + 1]) +
                           (short int)(unsigned char)(weights[offset]);
        nnuelayer1[k][64 * convert[piece] + square][j] = weight;
        offset += 2;
      }
    }
  }
  for (int i = 0; i < nnuesize; i++) {
    short int bias = 256 * (short int)(weights[offset + 1]) +
                     (short int)(unsigned char)(weights[offset]);
    layer1bias[i] = bias;
    offset += 2;
  }
  for (int j = 0; j < outputbuckets; j++) {
    for (int i = 0; i < nnuesize; i++) {
      short int weight = 256 * (short int)(weights[offset + 1]) +
                         (short int)(unsigned char)(weights[offset]);
      ourlayer2[j][i] = (int)weight;
      offset += 2;
    }
    for (int i = 0; i < nnuesize; i++) {
      short int weight = 256 * (short int)(weights[offset + 1]) +
                         (short int)(unsigned char)(weights[offset]);
      theirlayer2[j][i] = (int)weight;
      offset += 2;
    }
  }
  for (int j = 0; j < outputbuckets; j++) {
    short int base = 256 * (short int)(weights[offset + 1]) +
                     (short int)(unsigned char)(weights[offset]);
    finalbias[j] = base;
    offset += 2;
  }
  delete[] weights;
  nnueweights.close();
}
void NNUE::activatepiece(int bucket, int color, int piece, int square) {
  short int* accptr = (acc->accumulation)[color];
  short int* weightsptr = nnuelayer1[bucket / mirrordivisor][featureindex(bucket, piece, color, square)];
  for (int i = 0; i < nnuesize; i++) {
    accptr[i] += weightsptr[i];
  }
}
void NNUE::deactivatepiece(int bucket, int color, int piece, int square) {
  short int* accptr = (acc->accumulation)[color];
  short int* weightsptr = nnuelayer1[bucket / mirrordivisor][featureindex(bucket, piece, color, square)];
  for (int i = 0; i < nnuesize; i++) {
    accptr[i] -= weightsptr[i];
  }
}
void NNUE::refreshfromscratch(int bucket, int color, const uint64_t *Bitboards) {
  short int* accptr = (acc->accumulation)[color];
  for (int i = 0; i < nnuesize; i++) {
    accptr[i] = layer1bias[i];
  }
  for (int i = 0; i < 12; i++) {
    uint64_t pieces = (Bitboards[i / 6] & Bitboards[2 + (i % 6)]);
    int piececount = __builtin_popcountll(pieces);
    for (int j = 0; j < piececount; j++) {
      int square = __builtin_popcountll((pieces & -pieces) - 1);
      activatepiece(bucket, color, (i % 6) + 6 * (color ^ (i / 6)), square);
      pieces ^= (1ULL << square);
    }
  }
}
void NNUE::initializennue(const uint64_t *Bitboards) {
  totalmaterial = 0;
  for (int i = 0; i < inputbuckets; i++) {
    refreshfromscratch(i, 0, Bitboards);
    refreshfromscratch(i, 1, Bitboards);
  }
  for (int i = 0; i < 12; i++) {
    uint64_t pieces = (Bitboards[i / 6] & Bitboards[2 + (i % 6)]);
    int piececount = __builtin_popcountll(pieces);
    totalmaterial += piececount * material[i % 6];
  }
  activebucket[0] =
      kingbuckets[__builtin_popcountll((Bitboards[0] & Bitboards[7]) - 1)];
  activebucket[1] =
      kingbuckets[56 ^ __builtin_popcountll((Bitboards[1] & Bitboards[7]) - 1)];
}
void NNUE::forwardaccumulators(int notation, const uint64_t *Bitboards) {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 21) & 3;
  int piece2 = (promoted > 0) ? piece : piece - 2;
  activatepiece(activebucket[color ^ 1], color ^ 1, 6 + piece2, to);
  deactivatepiece(activebucket[color ^ 1], color ^ 1, 4 + piece, from);
  if (captured > 0) {
    totalmaterial -= material[captured - 2];
    deactivatepiece(activebucket[color ^ 1], color ^ 1, captured - 2, to);
  }
  if (piece == 7 &&
      kingbuckets[to ^ (56 * color)] != kingbuckets[from ^ (56 * color)]) {
    refreshfromscratch(kingbuckets[to ^ (56 * color)], color, Bitboards);
    activebucket[color] = kingbuckets[to ^ (56 * color)];
  } else {
    activatepiece(activebucket[color], color, piece2, to);
    deactivatepiece(activebucket[color], color, piece - 2, from);
    if (captured > 0) {
      deactivatepiece(activebucket[color], color, 4 + captured, to);
    }
  }
}
void NNUE::backwardaccumulators(int notation, const uint64_t *Bitboards) {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 21) & 3;
  int piece2 = promoted ? piece : piece - 2;
  deactivatepiece(activebucket[color ^ 1], color ^ 1, 6 + piece2, to);
  activatepiece(activebucket[color ^ 1], color ^ 1, 4 + piece, from);
  if (captured > 0) {
    totalmaterial += material[captured - 2];
    activatepiece(activebucket[color ^ 1], color ^ 1, captured - 2, to);
  }
  if (piece == 7 &&
      kingbuckets[to ^ (56 * color)] != kingbuckets[from ^ (56 * color)]) {
    refreshfromscratch(kingbuckets[from ^ (56 * color)], color, Bitboards);
    activebucket[color] = kingbuckets[from ^ (56 * color)];
  } else {
    deactivatepiece(activebucket[color], color, piece2, to);
    activatepiece(activebucket[color], color, piece - 2, from);
    if (captured > 0) {
      activatepiece(activebucket[color], color, 4 + captured, to);
    }
  }
}
int NNUE::evaluate(int color) {
  int bucket = std::min(totalmaterial / bucketdivisor, outputbuckets - 1);
  int eval = finalbias[bucket] * evalQA;
  for (int i = 0; i < nnuesize; i++) {
    eval += screlu(accumulator[activebucket[color]][color][i]) *
            ourlayer2[bucket][i];
  }
  for (int i = 0; i < nnuesize; i++) {
    eval += screlu(accumulator[activebucket[color ^ 1]][color ^ 1][i]) *
            theirlayer2[bucket][i];
  }
  eval /= evalQA;
  eval *= evalscale;
  eval /= (evalQA * evalQB);
  return eval;
}
