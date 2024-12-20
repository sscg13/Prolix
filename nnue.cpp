#include <cmath>
#include <fstream>
#define INCBIN_PREFIX
#include "external/incbin/incbin.h"

const int nnuesize = 128;
const int outputbuckets = 8;
const int evalscale = 400;
const int evalQA = 255;
const int evalQB = 64;
const int material[6] = {1, 1, 1, 1, 1, 0};
const int bucketdivisor = 32 / outputbuckets;
const int nnuefilesize =
    (1538 * nnuesize + 4 * nnuesize * outputbuckets + 2 * outputbuckets);
INCBIN(char, NNUE, EUNNfile);
int screlu(short int x) {
  return std::pow(std::max(std::min((int)x, 255), 0), 2);
}
class NNUE {
  short int nnuelayer1[768][nnuesize];
  short int layer1bias[nnuesize];
  int ourlayer2[outputbuckets][nnuesize];
  int theirlayer2[outputbuckets][nnuesize];
  short int whitehidden[nnuesize];
  short int blackhidden[nnuesize];
  int finalbias[outputbuckets];
  int totalmaterial;

public:
  void loaddefaultnet();
  void readnnuefile(std::string file);
  void activatepiece(int color, int piece, int square);
  void deactivatepiece(int color, int piece, int square);
  void initializennue(uint64_t *Bitboards);
  void forwardaccumulators(int notation);
  void backwardaccumulators(int notation);
  int evaluate(int color);
};

void NNUE::loaddefaultnet() {
  int offset = 0;
  for (int i = 0; i < 768; i++) {
    int piece = i / 64;
    int square = i % 64;
    int convert[12] = {0, 3, 1, 4, 2, 5, 6, 9, 7, 10, 8, 11};
    for (int j = 0; j < nnuesize; j++) {
      short int weight = 256 * (short int)(NNUEData[offset + 1]) +
                         (short int)(unsigned char)(NNUEData[offset]);
      nnuelayer1[64 * convert[piece] + square][j] = weight;
      offset += 2;
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
  for (int i = 0; i < 768; i++) {
    int piece = i / 64;
    int square = i % 64;
    int convert[12] = {0, 3, 1, 4, 2, 5, 6, 9, 7, 10, 8, 11};
    for (int j = 0; j < nnuesize; j++) {
      short int weight = 256 * (short int)(weights[offset + 1]) +
                         (short int)(unsigned char)(weights[offset]);
      nnuelayer1[64 * convert[piece] + square][j] = weight;
      offset += 2;
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
void NNUE::activatepiece(int color, int piece, int square) {
  for (int i = 0; i < nnuesize; i++) {
    whitehidden[i] += nnuelayer1[64 * (6 * color + piece) + square][i];
    blackhidden[i] +=
        nnuelayer1[64 * (6 * (color ^ 1) + piece) + 56 ^ square][i];
  }
  totalmaterial += material[piece];
}
void NNUE::deactivatepiece(int color, int piece, int square) {
  for (int i = 0; i < nnuesize; i++) {
    whitehidden[i] -= nnuelayer1[64 * (6 * color + piece) + square][i];
    blackhidden[i] -=
        nnuelayer1[64 * (6 * (color ^ 1) + piece) + 56 ^ square][i];
  }
  totalmaterial -= material[piece];
}
void NNUE::initializennue(uint64_t *Bitboards) {
  totalmaterial = 0;
  for (int i = 0; i < nnuesize; i++) {
    whitehidden[i] = layer1bias[i];
    blackhidden[i] = layer1bias[i];
  }
  for (int i = 0; i < 12; i++) {
    uint64_t pieces = (Bitboards[i / 6] & Bitboards[2 + (i % 6)]);
    int piececount = __builtin_popcountll(pieces);
    for (int j = 0; j < piececount; j++) {
      int square = __builtin_popcountll((pieces & -pieces) - 1);
      activatepiece(i / 6, i % 6, square);
      pieces ^= (1ULL << square);
    }
  }
}
void NNUE::forwardaccumulators(int notation) {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 21) & 3;
  int piece2 = (promoted > 0) ? piece : piece - 2;
  NNUE::activatepiece(color, piece2, to);
  NNUE::deactivatepiece(color, piece - 2, from);
  if (captured > 0) {
    NNUE::deactivatepiece(color ^ 1, captured - 2, to);
  }
}
void NNUE::backwardaccumulators(int notation) {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 21) & 3;
  int piece2 = promoted ? piece : piece - 2;
  NNUE::deactivatepiece(color, piece2, to);
  NNUE::activatepiece(color, piece - 2, from);
  if (captured > 0) {
    NNUE::activatepiece(color ^ 1, captured - 2, to);
  }
}
int NNUE::evaluate(int color) {
  int bucket = std::min(totalmaterial / bucketdivisor, outputbuckets - 1);
  int eval = finalbias[bucket] * evalQA;
  if (color == 0) {
    for (int i = 0; i < nnuesize; i++) {
      eval += screlu(whitehidden[i]) * ourlayer2[bucket][i];
      eval += screlu(blackhidden[i]) * theirlayer2[bucket][i];
    }
  } else {
    for (int i = 0; i < nnuesize; i++) {
      eval += screlu(whitehidden[i]) * theirlayer2[bucket][i];
      eval += screlu(blackhidden[i]) * ourlayer2[bucket][i];
    }
  }
  eval /= evalQA;
  eval *= evalscale;
  eval /= (evalQA * evalQB);
  return eval;
}
