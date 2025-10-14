#include "nnue.h"
#include <cmath>
#include <fstream>
#define INCBIN_PREFIX
#include "../external/incbin/incbin.h"

INCBIN(char, NNUE, EUNNfile);
int screlu(I16 x) {
  I16 y = std::max(std::min(x, (I16)255), (I16)0);
  return y * y;
}

void NNUEWeights::loaddefaultnet() {
  int offset = 0;
  for (int k = 0; k < realbuckets; k++) {
    for (int i = 0; i < 768; i++) {
      int piece = i / 64;
      int square = i % 64;
      int convert[12] = {0, 3, 1, 4, 2, 5, 6, 9, 7, 10, 8, 11};
      for (int j = 0; j < nnuesize; j++) {
        I16 weight = 256 * (I16)(NNUEData[offset + 1]) +
                     (I16)(unsigned char)(NNUEData[offset]);
        nnuelayer1[k][64 * convert[piece] + square][j] = weight;
        offset += 2;
      }
    }
  }
  for (int i = 0; i < nnuesize; i++) {
    I16 bias = 256 * (I16)(NNUEData[offset + 1]) +
               (I16)(unsigned char)(NNUEData[offset]);
    layer1bias[i] = bias;
    offset += 2;
  }
  for (int j = 0; j < outputbuckets; j++) {
    for (int i = 0; i < nnuesize; i++) {
      I16 weight = 256 * (I16)(NNUEData[offset + 1]) +
                   (I16)(unsigned char)(NNUEData[offset]);
      ourlayer2[j][i] = (int)weight;
      offset += 2;
    }
    for (int i = 0; i < nnuesize; i++) {
      I16 weight = 256 * (I16)(NNUEData[offset + 1]) +
                   (I16)(unsigned char)(NNUEData[offset]);
      theirlayer2[j][i] = (int)weight;
      offset += 2;
    }
  }
  for (int j = 0; j < outputbuckets; j++) {
    I16 base = 256 * (I16)(NNUEData[offset + 1]) +
               (I16)(unsigned char)(NNUEData[offset]);
    finalbias[j] = base;
    offset += 2;
  }
}
void NNUEWeights::readnnuefile(std::string file) {
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
        I16 weight = 256 * (I16)(weights[offset + 1]) +
                     (I16)(unsigned char)(weights[offset]);
        nnuelayer1[k][64 * convert[piece] + square][j] = weight;
        offset += 2;
      }
    }
  }
  for (int i = 0; i < nnuesize; i++) {
    I16 bias = 256 * (I16)(weights[offset + 1]) +
               (I16)(unsigned char)(weights[offset]);
    layer1bias[i] = bias;
    offset += 2;
  }
  for (int j = 0; j < outputbuckets; j++) {
    for (int i = 0; i < nnuesize; i++) {
      I16 weight = 256 * (I16)(weights[offset + 1]) +
                   (I16)(unsigned char)(weights[offset]);
      ourlayer2[j][i] = (int)weight;
      offset += 2;
    }
    for (int i = 0; i < nnuesize; i++) {
      I16 weight = 256 * (I16)(weights[offset + 1]) +
                   (I16)(unsigned char)(weights[offset]);
      theirlayer2[j][i] = (int)weight;
      offset += 2;
    }
  }
  for (int j = 0; j < outputbuckets; j++) {
    I16 base = 256 * (I16)(weights[offset + 1]) +
               (I16)(unsigned char)(weights[offset]);
    finalbias[j] = base;
    offset += 2;
  }
  delete[] weights;
  nnueweights.close();
}
int NNUE::getbucket(int kingsquare, int color) {
  return kingbuckets[(56 * color) ^ kingsquare];
}
int NNUE::featureindex(int bucket, int color, int piece, int square) {
  int piececolor = piece / 6;
  int perspectivepiece = (color ^ piececolor) * 6 + (piece % 6);
  bool hmactive = mirrored && (bucket % 2 == 1);
  int perspectivesquare = (56 * color) ^ square ^ (7 * hmactive);
  return 64 * perspectivepiece + perspectivesquare;
}
int NNUE::differencecount(int bucket, int color, const U64 *Bitboards) {
  return __builtin_popcountll(
      (cachebitboards[bucket][color][0] | cachebitboards[bucket][color][1]) ^
      (Bitboards[0] | Bitboards[1]));
}
const I16 *NNUE::layer1weights(int kingsquare, int color, int piece,
                               int square) {
  int bucket = getbucket(kingsquare, color);
  return weights->nnuelayer1[bucket / mirrordivisor]
                            [featureindex(bucket, color, piece, square)];
}
void NNUE::add(I16 *accptr, const I16 *addptr) {
  for (int i = 0; i < nnuesize; i++) {
    accptr[i] += addptr[i];
  }
}
void NNUE::sub(I16 *accptr, const I16 *subptr) {
  for (int i = 0; i < nnuesize; i++) {
    accptr[i] -= subptr[i];
  }
}
void NNUE::addsub(I16 *oldaccptr, I16 *newaccptr, const I16 *addptr,
                  const I16 *subptr) {
  for (int i = 0; i < nnuesize; i++) {
    newaccptr[i] = oldaccptr[i] + addptr[i] - subptr[i];
  }
}
void NNUE::addsubsub(I16 *oldaccptr, I16 *newaccptr, const I16 *addptr,
                     const I16 *subptr1, const I16 *subptr2) {
  for (int i = 0; i < nnuesize; i++) {
    newaccptr[i] = oldaccptr[i] + addptr[i] - subptr1[i] - subptr2[i];
  }
}
void NNUE::activatepiece(I16 *accptr, int kingsquare, int color, int piece,
                         int square) {
  const I16 *weightsptr = layer1weights(kingsquare, color, piece, square);
  for (int i = 0; i < nnuesize; i++) {
    accptr[i] += weightsptr[i];
  }
}
void NNUE::deactivatepiece(I16 *accptr, int kingsquare, int color, int piece,
                           int square) {
  const I16 *weightsptr = layer1weights(kingsquare, color, piece, square);
  for (int i = 0; i < nnuesize; i++) {
    accptr[i] -= weightsptr[i];
  }
}
void NNUE::refreshfromcache(int kingsquare, int color, const U64 *Bitboards) {
  U64 add[12];
  U64 remove[12];
  int bucket = getbucket(kingsquare, color);
  I16 *accptr = accumulation[2 * ply + color];
  I16 *cacheaccptr = cacheaccumulators[bucket][color];
  for (int i = 0; i < 12; i++) {
    add[i] = (Bitboards[i / 6] & Bitboards[2 + (i % 6)]) &
             ~(cachebitboards[bucket][color][i / 6] &
               cachebitboards[bucket][color][2 + (i % 6)]);
    remove[i] = (cachebitboards[bucket][color][i / 6] &
                 cachebitboards[bucket][color][2 + (i % 6)]) &
                ~(Bitboards[i / 6] & Bitboards[2 + (i % 6)]);
  }
  for (int i = 0; i < 12; i++) {
    int addcount = __builtin_popcountll(add[i]);
    for (int j = 0; j < addcount; j++) {
      int square = __builtin_ctzll(add[i]);
      activatepiece(cacheaccptr, kingsquare, color, i, square);
      add[i] ^= (1ULL << square);
    }
    int removecount = __builtin_popcountll(remove[i]);
    for (int j = 0; j < removecount; j++) {
      int square = __builtin_ctzll(remove[i]);
      deactivatepiece(cacheaccptr, kingsquare, color, i, square);
      remove[i] ^= (1ULL << square);
    }
  }
  for (int i = 0; i < 8; i++) {
    cachebitboards[bucket][color][i] = Bitboards[i];
  }
  for (int i = 0; i < nnuesize; i++) {
    accptr[i] = cacheaccptr[i];
  }
}
void NNUE::refreshfromscratch(int kingsquare, int color, const U64 *Bitboards) {
  I16 *accptr = accumulation[2 * ply + color];
  I16 *cacheaccptr = cacheaccumulators[getbucket(kingsquare, color)][color];
  for (int i = 0; i < nnuesize; i++) {
    accptr[i] = weights->layer1bias[i];
  }
  for (int i = 0; i < 8; i++) {
    cachebitboards[getbucket(kingsquare, color)][color][i] = Bitboards[i];
  }
  for (int i = 0; i < 12; i++) {
    U64 pieces = (Bitboards[i / 6] & Bitboards[2 + (i % 6)]);
    int piececount = __builtin_popcountll(pieces);
    for (int j = 0; j < piececount; j++) {
      int square = __builtin_popcountll((pieces & -pieces) - 1);
      activatepiece(accptr, kingsquare, color, i, square);
      pieces ^= (1ULL << square);
    }
  }
  for (int i = 0; i < nnuesize; i++) {
    cacheaccptr[i] = accptr[i];
  }
}
void NNUE::initializennue(const U64 *Bitboards) {
  totalmaterial = 0;
  ply = 0;
  for (int color = 0; color < 2; color++) {
    refreshfromscratch(__builtin_ctzll(Bitboards[7] & Bitboards[color]), color,
                       Bitboards);
    for (int bucket = 0; bucket < inputbuckets; bucket++) {
      for (int i = 0; i < 8; i++) {
        cachebitboards[bucket][color][i] = 0ULL;
      }
      for (int i = 0; i < nnuesize; i++) {
        cacheaccumulators[bucket][color][i] = weights->layer1bias[i];
      }
    }
  }
  for (int i = 0; i < 12; i++) {
    U64 pieces = (Bitboards[i / 6] & Bitboards[2 + (i % 6)]);
    int piececount = __builtin_popcountll(pieces);
    totalmaterial += piececount * material[i % 6];
  }
}
void NNUE::forwardaccumulators(const int notation, const U64 *Bitboards) {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 21) & 3;
  int piece2 = (promoted > 0) ? piece : piece - 2;
  int oppksq = __builtin_ctzll(Bitboards[7] & Bitboards[color ^ 1]);
  int ourksq = __builtin_ctzll(Bitboards[7] & Bitboards[color]);
  I16 *newaccptr = accumulation[2 * (ply + 1) + (color ^ 1)];
  I16 *oldaccptr = accumulation[2 * ply + (color ^ 1)];
  const I16 *addweightsopp =
      layer1weights(oppksq, color ^ 1, 6 * color + piece2, to);
  const I16 *subweightsopp =
      layer1weights(oppksq, color ^ 1, 6 * color + piece - 2, from);
  ply++;
  if (captured > 0) {
    const I16 *subweights2opp =
        layer1weights(oppksq, color ^ 1, 6 * (color ^ 1) + captured - 2, to);
    totalmaterial -= material[captured - 2];
    addsubsub(oldaccptr, newaccptr, addweightsopp, subweightsopp,
              subweights2opp);
  } else {
    addsub(oldaccptr, newaccptr, addweightsopp, subweightsopp);
  }
  if (piece == 7 && getbucket(to, color) != getbucket(from, color)) {
    int scratchrefreshtime = __builtin_popcountll(Bitboards[0] | Bitboards[1]);
    int cacherefreshtime =
        differencecount(getbucket(to, color), color, Bitboards);
    if (scratchrefreshtime <= cacherefreshtime) {
      refreshfromscratch(to, color, Bitboards);
    } else {
      refreshfromcache(to, color, Bitboards);
    }
  } else {
    newaccptr = accumulation[2 * ply + color];
    oldaccptr = accumulation[2 * (ply - 1) + color];
    const I16 *addweightsus =
        layer1weights(ourksq, color, 6 * color + piece2, to);
    const I16 *subweightsus =
        layer1weights(ourksq, color, 6 * color + piece - 2, from);
    if (captured > 0) {
      const I16 *subweights2us =
          layer1weights(ourksq, color, 6 * (color ^ 1) + captured - 2, to);
      addsubsub(oldaccptr, newaccptr, addweightsus, subweightsus,
                subweights2us);
    } else {
      addsub(oldaccptr, newaccptr, addweightsus, subweightsus);
    }
  }
}
void NNUE::backwardaccumulators(int notation, const U64 *Bitboards) {
  int captured = (notation >> 17) & 7;
  if (captured > 0) {
    totalmaterial += material[captured - 2];
  }
  ply--;
}
int NNUE::evaluate(int color) {
  int bucket = std::min(totalmaterial / bucketdivisor, outputbuckets - 1);
  int eval = weights->finalbias[bucket] * evalQA;
  I16 *stmaccptr = accumulation[2 * ply + color];
  I16 *nstmaccptr = accumulation[2 * ply + (color ^ 1)];
  const int *stmweightsptr = weights->ourlayer2[bucket];
  const int *nstmweightsptr = weights->theirlayer2[bucket];
  for (int i = 0; i < nnuesize; i++) {
    eval += screlu(stmaccptr[i]) * stmweightsptr[i];
    eval += screlu(nstmaccptr[i]) * nstmweightsptr[i];
  }
  eval /= evalQA;
  eval *= evalscale;
  eval /= (evalQA * evalQB);
  return eval;
}
