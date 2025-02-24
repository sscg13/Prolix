#include "board.h"
#include "external/Fathom/tbprobe.h"
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
U64 KingAttacks[64];
U64 PawnAttacks[2][64];
U64 AlfilAttacks[64];
U64 FerzAttacks[64];
U64 KnightAttacks[64];
U64 RankMask[64];
U64 FileMask[64];
U64 RankAttacks[512];
U64 hashes[8][64];
const U64 colorhash = 0xE344F58E0F3B26E5;
// clang-format off
const int materialm[6] = {78, 77, 144, 415, 657, 20000};
const int materiale[6] = {85, 113, 139, 402, 905, 20000};
const int pstm[6][64] = {
    { 0,   0,   0,   0,   0,   0,   0,   0, 
    -61,  -2,  14,  19,  -9,  -5,  -7, -35,
    -54,  24,  12,  23,   8,   3,  -3, -34, 
    -28, -12,  18,  48,  18,   9,  -2, -35,
      0, -17,  13,  53,  -7,   4, -19, -34, 
    -14,  -4,   4,  -2,   8, -26, -16, -23,
      9,  18,  20,  28,  29,  14,   9,  -2,  
      0,   0,   0,   0,   0,   0,   0,   0},

    {-41, -23,  -7, -17, -19,  -1, -25, -45,
     -38, -36,  -4,  -7,  -5,   1, -32, -37, 
      11, -15,   9, -12, -11,   6, -17,  -0,  
     -17, -12,  13,  26,  23,  18, -13, -18,
     -19,  -2, -14,  24,  23,  -6, -36, -20,
     -25,  -8,  12,  19,  17,  19,  -9, -23,
     -27, -21,   7,  -7,  -8,   2, -28, -42,
     -39, -33, -17, -11, -16, -22, -31, -38},

    {-39, -23, -30, -22,   3, -21, -35, -34,
     -29, -21,  -9,  -1, -19, -11, -21, -33,
     -30, -13,  15,   2,  16,   6,   2, -22,
     -23, -11,   5,  34,   6,  12,  -2, -17,
     -17,  -8,  13,   5,  50,   9,   2, -19,
      -5,  -6,  -5,  11,  -1,  17,   3,  -9,
     -27, -17,  -4,  -1,  -4,  -2, -11, -28,
     -39, -27, -24, -16, -12, -26, -24, -39},

    {-42,  16,  -9, -15,   2, -12,   4, -35,
     -27,  -6,  -1,  16,   8,  -5,  -7, -27,
      11,  14,  25,  26,   1,  18, -13,  -9,
       0,  -2,  20,  37,  22,  16,  12,  -8,
       5,  37,  48,  28,  22,  38,  12, -15,
     -23,   1,  11,  35,  30,  33,   5, -22,
     -32, -32,   0,  15,   3,   4, -18, -18,
     -54, -25, -16,  -1,   4,  -1, -14, -51},

    {-28, -33,  -2,   2,  -2, -12,  -5, -14,
     -45,  -4,  -2, -20,   1,  -5, -16, -12,
     -22, -19, -15,  -2, -13, -16, -14, -28,
     -24, -11,  -6, -10,  -6,  -4, -12,  -4,
      -0,  -8,   5,  -4,  -4,   8,  -9,   7, 
       2,   0,   6,  14,   1,  12,   5,   9,
      17,  17,  25,  20,  25,  29,  21,  19,
      10,  13,  13,  19,  16,  21,   9,  18},

    {-21,  -7,   5,  14, -16,  -5, -11, -19,
       2,  31,  23,   7,  13, -10,  -2,  -2,
     -12,   7,   1,  22,  -1,  13,   3,  -8,
     -19, -16, -14, -17, -21, -13,  -8, -11,
     -18, -13, -17, -33, -25, -22, -15, -17,
     -15, -16, -12, -18, -33, -18, -21, -19,
     -17, -15, -18, -25, -23, -21, -22, -20,
     -17, -20, -19, -26, -22, -16, -18, -14}};
const int pste[6][64] = {
    { 0,   0,   0,   0,   0,   0,   0,   0,
      6, -12,  -1, -24, -38,   8,  -4, -24,
    -25, -13,   4,  12, -17,  -5,   3,  -7, 
    -22, -13,  20,   0,  18, -27, -12,  -4,
    -26,   2,  -5, -14,  10,   4,  -2, -15,
      1,   7,   6,   9,  20, -20,   4, -25,
     18,  22,  22,  21,  42,  10,  16,   7, 
      0,   0,   0,   0,   0,   0,   0,   0},

    {-41, -30,   3, -17, -19, -19, -24, -45,
     -38, -36,  -4,  -7,  -5,   1, -32, -37,
      -1, -15,   9,   2,  12,   6, -17, -29,
     -17, -12,  13,  26,  23,  18, -13, -18,
     -19, -11,  19,  24,  23,   8,  16, -20,
     -25, -8,   12,  19,  17,  19,  -9, -23,
     -29, -21,   7,  -6,  11,   2, -28, -31,
     -39, -33, -17, -11, -16, -22, -31, -38},

    {-41, -24, -24, -22, -16, -21, -35, -34,
     -29, -19,  -9, -13, -19,  -7, -20, -37,
     -32, -14,  19,   7,  25,  12,  -9, -22,
     -23, -14,  10,  20,  28,   3,  -2, -19, 
     -19,  -7,  21,  29,  14,  19,  -0, -20,
      -8,  10,  10,   5,  12,   4,   6, -20,
     -32, -13,  -6,   3, -17,   6, -23, -26,
     -47, -29, -21, -25, -13, -33, -24, -43},

    {-43, -11, -10,  -3,  -3, -14, -18, -37,
     -28, -12,  -8,  -1,   9,   2, -11, -33,
     -20, -10,  17,   5,   9,  24,  -7,  -9,
     -11,  -5,  20,  28,  31,  16,  11, -19,
      -6,   4,  33,  48,  55,  18,  32,   2,
     -24,   2,  11,  32,  29,  30,   7, -18,
     -35,  -7,   0,  17,   9,  11,  -6, -25,
     -49, -26, -17,  -2,   3,   3, -10, -46},

    {-31, -18,  -5,  -2, -13,  -4, -13, -26,
     -27,  -9,  -6,   1,   1,   4, -11,   1,
     -12, -10, -11,  -2, -13,   6,  -0,  10,
      -5,  -5,  -2,  -4,  -7,  -2,   1,  17,
      -1,  -4,  -2,  -3,  -2,   3,   0,   4,
       1,  -3,   2,   2,   2,   7,   6,   9,
       9,   9,  11,   9,   5,   8,  11,  13,
      23,  17,  17,  17,  15,  17,  18,  27},

    {-38, -37, -45, -25, -20, -28, -36, -43,
     -30, -32, -22, -20,  -9, -13, -16, -31,
     -25,  -3,   2,   8,   6,   1,   6, -21,
     -27,  -2,  23,  37,  26,  20,   0,  -9,
      -3,  15,  28,  25,  38,  13,   6, -11,
      -5,   9,  11,  24,   1,  16,  -8, -12,  
     -20,  -9,  -6,  -6,  -3,  -1,  -5, -23,   
     -30, -26, -17, -13, -17, -14, -20, -31}};
const int ferzmobm[5] = {-17, -9, -2, 3, 8};
const int ferzmobe[5] = {-15, -8, 1, 5, 10};
const int alfilmobm[5] = {-17, -11, -5, 4, 11};
const int alfilmobe[5] = {-10, -7, -4, 6, 10};
const int knightmobm[9] = {-35, -21, -12, -5, 0, 5, 12, 21, 35};
const int knightmobe[9] = {-41, -26, -10, 1, 9, 17, 25, 31, 35};
const int rookmobm[15] = {-21, -22, -14, -18, -12, -5, -1, 2, 5, 9, 11, 12, 15, 19, 24};
const int rookmobe[15] = {-40, -32, -22, -15, -9, -5, -1, 3, 7, 13, 20, 23, 28, 32, 37};
const int kingmobe[9] = {-61, -38, -15, -6, 3, 13, 22, 28, 33};
const int phase[6] = {0, 1, 2, 4, 6, 0};
// clang-format on
U64 shift_w(U64 bitboard) { return (bitboard & ~FileA) >> 1; }
U64 shift_n(U64 bitboard) { return bitboard << 8; }
U64 shift_s(U64 bitboard) { return bitboard >> 8; }
U64 shift_e(U64 bitboard) { return (bitboard & ~FileH) << 1; }
void initializeleaperattacks() {
  for (int i = 0; i < 64; i++) {
    U64 square = 1ULL << i;
    PawnAttacks[0][i] = ((square & ~FileA) << 7) | ((square & ~FileH) << 9);
    PawnAttacks[1][i] = ((square & ~FileA) >> 9) | ((square & ~FileH) >> 7);
    U64 horizontal = square | shift_w(square) | shift_e(square);
    U64 full = horizontal | shift_n(horizontal) | shift_s(horizontal);
    KingAttacks[i] = full & ~square;
    U64 knightattack = ((square & ~FileA) << 15);
    knightattack |= ((square & ~FileA) >> 17);
    knightattack |= ((square & ~FileH) << 17);
    knightattack |= ((square & ~FileH) >> 15);
    knightattack |= ((square & ~FileG & ~FileH) << 10);
    knightattack |= ((square & ~FileG & ~FileH) >> 6);
    knightattack |= ((square & ~FileA & ~FileB) << 6);
    knightattack |= ((square & ~FileA & ~FileB) >> 10);
    KnightAttacks[i] = knightattack;
    FerzAttacks[i] = shift_n(shift_w(square) | shift_e(square)) |
                     shift_s(shift_w(square) | shift_e(square));
    U64 alfilattack = ((square & ~FileA & ~FileB) << 14);
    alfilattack |= ((square & ~FileA & ~FileB) >> 18);
    alfilattack |= ((square & ~FileG & ~FileH) << 18);
    alfilattack |= ((square & ~FileG & ~FileH) >> 14);
    AlfilAttacks[i] = alfilattack;
  }
}
void initializemasks() {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      RankMask[8 * i + j] = Ranks[i];
      FileMask[8 * i + j] = Files[j];
    }
  }
}
void initializerankattacks() {
  for (U64 i = 0ULL; i < 0x000000000040; i++) {
    U64 occupied = i << 1;
    for (int j = 0; j < 8; j++) {
      U64 attacks = 0ULL;
      if (j > 0) {
        int k = j - 1;
        while (k >= 0) {
          attacks |= (1ULL << k);
          if ((1ULL << k) & occupied) {
            k = 0;
          }
          k--;
        }
      }
      if (j < 7) {
        int k = j + 1;
        while (k <= 7) {
          attacks |= (1ULL << k);
          if ((1ULL << k) & occupied) {
            k = 7;
          }
          k++;
        }
      }
      RankAttacks[8 * i + j] = attacks;
    }
  }
}
U64 FileAttacks(U64 occupied, int square) {
  U64 forwards = occupied & FileMask[square];
  U64 backwards = __builtin_bswap64(forwards);
  forwards = forwards - 2 * (1ULL << square);
  backwards = backwards - 2 * (1ULL << (56 ^ square));
  backwards = __builtin_bswap64(backwards);
  return (forwards ^ backwards) & FileMask[square];
}
U64 GetRankAttacks(U64 occupied, int square) {
  int row = square & 56;
  int file = square & 7;
  int relevant = (occupied >> (row + 1)) & 63;
  return (RankAttacks[8 * relevant + file] << row);
}
void initializezobrist() {
  std::mt19937_64 mt(20346892);
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 64; j++) {
      hashes[i][j] = mt();
    }
  }
}
std::string algebraic(int notation) {
  std::string convert[64] = {
      "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", "a2", "b2", "c2",
      "d2", "e2", "f2", "g2", "h2", "a3", "b3", "c3", "d3", "e3", "f3",
      "g3", "h3", "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", "a5",
      "b5", "c5", "d5", "e5", "f5", "g5", "h5", "a6", "b6", "c6", "d6",
      "e6", "f6", "g6", "h6", "a7", "b7", "c7", "d7", "e7", "f7", "g7",
      "h7", "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"};
  std::string header = convert[notation & 63] + convert[(notation >> 6) & 63];
  if (notation & (1 << 20)) {
    header = header + "q";
  }
  return header;
}
std::string get8294400FEN(int seed1, int seed2) {
  std::vector<std::string> rank8 = {"r", "n", "b", "k", "q", "b", "n", "r"};
  std::vector<std::string> rank1 = {"R", "N", "B", "K", "Q", "B", "N", "R"};
  std::string FEN = "";
  for (int i = 8; i > 0; i--) {
    FEN += rank8[seed1 % i];
    rank8.erase(rank8.begin() + (seed1 % i));
    seed1 /= i;
  }
  FEN += "/pppppppp/8/8/8/8/PPPPPPPP/";
  for (int i = 8; i > 0; i--) {
    FEN += rank1[seed2 % i];
    rank1.erase(rank1.begin() + (seed2 % i));
    seed2 /= i;
  }
  FEN += " w - - 0 1";
  return FEN;
}
std::string backrow(int seed, bool black) {
  int triangle1[5] = {-12, 0, 1, 2, 4};
  int orders[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  char rank[9] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};
  int location = 2 + 4 * (seed % 2);
  rank[location] = black ? 'b' : 'B';
  orders[location] = 0;
  for (int i = location + 1; i < 8; i++) {
    orders[i] = std::max(orders[i] - 1, 0);
  }
  seed /= 2;
  location = 1 + 4 * (seed % 2);
  rank[location] = black ? 'b' : 'B';
  orders[location] = 0;
  for (int i = location + 1; i < 8; i++) {
    orders[i] = std::max(orders[i] - 1, 0);
  }
  seed /= 2;
  int aux = -1;
  for (int i = 0; i < 8; i += 2) {
    if (orders[i] > 0) {
      aux++;
      if (aux == seed % 3) {
        location = i;
      }
    }
  }
  rank[location] = black ? 'q' : 'Q';
  orders[location] = 0;
  for (int i = location + 1; i < 8; i++) {
    orders[i] = std::max(orders[i] - 1, 0);
  }
  seed /= 3;
  aux = -1;
  for (int i = 0; i < 8; i++) {
    if (orders[i] > 0) {
      aux++;
      if (aux == seed % 5) {
        location = i;
      }
    }
  }
  rank[location] = black ? 'k' : 'K';
  orders[location] = 0;
  for (int i = location + 1; i < 8; i++) {
    orders[i] = std::max(orders[i] - 1, 0);
  }
  seed /= 5;
  for (int i = 0; i < 7; i++) {
    for (int j = i + 1; j < 8; j++) {
      if (triangle1[orders[i]] + triangle1[orders[j]] == seed + 1) {
        rank[i] = black ? 'n' : 'N';
        rank[j] = black ? 'n' : 'N';
        orders[i] = 0;
        orders[j] = 0;
      }
    }
  }
  for (int i = 0; i < 8; i++) {
    if (orders[i] > 0) {
      rank[i] = black ? 'r' : 'R';
    }
  }
  std::string output = rank;
  return output;
}
std::string get129600FEN(int seed1, int seed2) {
  std::string FEN = backrow(seed1, 1);
  FEN += "/pppppppp/8/8/8/8/PPPPPPPP/";
  FEN += backrow(seed2, 0);
  FEN += " w - - 0 1";
  return FEN;
}

U64 Board::scratchzobrist() {
  U64 scratch = 0ULL;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 64; j++) {
      if (Bitboards[i] & (1ULL << j)) {
        scratch ^= hashes[i][j];
      }
    }
  }
  if (position & 1) {
    scratch ^= colorhash;
  }
  return scratch;
}
void Board::initialize() {
  Bitboards[0] = Rank1 | Rank2;
  Bitboards[1] = Rank7 | Rank8;
  Bitboards[2] = Rank2 | Rank7;
  Bitboards[3] = (Rank1 | Rank8) & (FileC | FileF);
  Bitboards[4] = (Rank1 | Rank8) & FileE;
  Bitboards[5] = (Rank1 | Rank8) & (FileB | FileG);
  Bitboards[6] = (Rank1 | Rank8) & (FileA | FileH);
  Bitboards[7] = (Rank1 | Rank8) & FileD;
  position = 0;
  history[0] = position;
  int startmatm =
      (8 * materialm[0] + 2 * (materialm[1] + materialm[3] + materialm[4]) +
       materialm[2]);
  int startmate =
      (8 * materiale[0] + 2 * (materiale[1] + materiale[3] + materiale[4]) +
       materiale[2]);
  int startpstm = 0;
  int startpste = 0;
  for (int i = 0; i < 16; i++) {
    startpstm += pstm[startpiece[i]][i];
    startpste += pste[startpiece[i]][i];
  }
  evalm[0] = startmatm + startpstm;
  evalm[1] = startmatm + startpstm;
  evale[0] = startmate + startpste;
  evale[1] = startmate + startpste;
  gamephase[0] = 24;
  gamephase[1] = 24;
  gamelength = 0;
  zobrist[0] = scratchzobrist();
}
int Board::repetitions() {
  int repeats = 0;
  for (int i = gamelength - 2; i >= 0; i -= 2) {
    if (zobrist[i] == zobrist[gamelength]) {
      repeats++;
      if (i >= root) {
        repeats++;
      }
    }
  }
  return repeats;
}
int Board::halfmovecount() { return (position >> 1); }
bool Board::twokings() { return (Bitboards[0] | Bitboards[1]) == Bitboards[7]; }
bool Board::bareking(int color) {
  return (Bitboards[color] & Bitboards[7]) == Bitboards[color];
}
int Board::material() {
  return __builtin_popcountll(Bitboards[2]) +
         __builtin_popcountll(Bitboards[3]) +
         2 * __builtin_popcountll(Bitboards[4]) +
         4 * __builtin_popcountll(Bitboards[5]) +
         6 * __builtin_popcountll(Bitboards[6]);
}
U64 Board::checkers(int color) {
  int kingsquare = __builtin_ctzll(Bitboards[color] & Bitboards[7]);
  int opposite = color ^ 1;
  U64 attacks = 0ULL;
  U64 occupied = Bitboards[0] | Bitboards[1];
  attacks |= (KnightAttacks[kingsquare] & Bitboards[5]);
  attacks |= (PawnAttacks[color][kingsquare] & Bitboards[2]);
  attacks |= (AlfilAttacks[kingsquare] & Bitboards[3]);
  attacks |= (FerzAttacks[kingsquare] & Bitboards[4]);
  attacks |= (GetRankAttacks(occupied, kingsquare) & Bitboards[6]);
  attacks |= (FileAttacks(occupied, kingsquare) & Bitboards[6]);
  attacks &= Bitboards[opposite];
  return attacks;
}
void Board::makenullmove() {
  gamelength++;
  int halfmove = (position >> 1) & 127;
  zobristhash ^= colorhash;
  position ^= (halfmove << 1);
  halfmove++;
  position ^= (halfmove << 1);
  position ^= 1;
  zobrist[gamelength] = zobristhash;
  history[gamelength] = position;
}
void Board::unmakenullmove() {
  gamelength--;
  position = history[gamelength];
  zobristhash = zobrist[gamelength];
}
void Board::makemove(int notation, bool reversible) {
  // 6 bits from square, 6 bits to square, 1 bit color, 3 bits piece moved, 1
  // bit capture, 3 bits piece captured, 1 bit promotion, 2 bits promoted piece,
  // 1 bit castling, 1 bit double pawn push, 1 bit en passant, 26 bits total

  // 1 bit color, 7 bits halfmove, 6 bits ep, 4 bits castling
  gamelength++;
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  Bitboards[color] ^= ((1ULL << from) | (1ULL << to));
  Bitboards[piece] ^= ((1ULL << from) | (1ULL << to));
  evalm[color] += pstm[piece - 2][(56 * color) ^ to];
  evalm[color] -= pstm[piece - 2][(56 * color) ^ from];
  evale[color] += pste[piece - 2][(56 * color) ^ to];
  evale[color] -= pste[piece - 2][(56 * color) ^ from];
  zobristhash ^= (hashes[color][from] ^ hashes[color][to]);
  zobristhash ^= (hashes[piece][from] ^ hashes[piece][to]);
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 21) & 3;
  int halfmove = (position >> 1);
  position ^= (halfmove << 1);
  halfmove++;
  position &= 0x0003C0FF;
  if (piece == 2) {
    halfmove = 0;
    if (!reversible) {
      gamelength = 0;
    }
  }
  if (notation & (1 << 16)) {
    Bitboards[color ^ 1] ^= (1ULL << to);
    Bitboards[captured] ^= (1ULL << to);
    zobristhash ^= (hashes[color ^ 1][to] ^ hashes[captured][to]);
    evalm[color ^ 1] -= materialm[captured - 2];
    evale[color ^ 1] -= materiale[captured - 2];
    evalm[color ^ 1] -= pstm[captured - 2][(56 * (color ^ 1)) ^ to];
    evale[color ^ 1] -= pste[captured - 2][(56 * (color ^ 1)) ^ to];
    gamephase[color ^ 1] -= phase[captured - 2];
    halfmove = 0;
    if (!reversible) {
      gamelength = 0;
    }
  }
  if (notation & (1 << 20)) {
    Bitboards[2] ^= (1ULL << to);
    Bitboards[promoted + 3] ^= (1ULL << to);
    zobristhash ^= (hashes[2][to] ^ hashes[promoted + 3][to]);
    evalm[color] -= (materialm[0] + pstm[0][(56 * color) ^ from]);
    evalm[color] +=
        (materialm[promoted + 1] + pstm[promoted + 1][(56 * color) ^ from]);
    evale[color] -= (materiale[0] + pste[0][(56 * color) ^ from]);
    evale[color] +=
        (materiale[promoted + 1] + pste[promoted + 1][(56 * color) ^ to]);
    gamephase[color] += phase[promoted + 1];
  } else if (notation & (1 << 24)) {
    position ^= ((from + to) << 7);
  }
  if (!reversible) {
    root = gamelength;
  }
  position ^= 1;
  position ^= (halfmove << 1);
  zobristhash ^= colorhash;
  history[gamelength] = position;
  zobrist[gamelength] = zobristhash;
  nodecount++;
}
void Board::unmakemove(int notation) {
  gamelength--;
  position = history[gamelength];
  zobristhash = zobrist[gamelength];
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  Bitboards[color] ^= ((1ULL << from) | (1ULL << to));
  Bitboards[piece] ^= ((1ULL << from) | (1ULL << to));
  evalm[color] += pstm[piece - 2][(56 * color) ^ from];
  evalm[color] -= pstm[piece - 2][(56 * color) ^ to];
  evale[color] += pste[piece - 2][(56 * color) ^ from];
  evale[color] -= pste[piece - 2][(56 * color) ^ to];
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 21) & 3;
  if (notation & (1 << 16)) {
    Bitboards[color ^ 1] ^= (1ULL << to);
    Bitboards[captured] ^= (1ULL << to);
    evalm[color ^ 1] += materialm[captured - 2];
    evale[color ^ 1] += materiale[captured - 2];
    evalm[color ^ 1] += pstm[captured - 2][(56 * (color ^ 1)) ^ to];
    evale[color ^ 1] += pste[captured - 2][(56 * (color ^ 1)) ^ to];
    gamephase[color ^ 1] += phase[captured - 2];
  }
  if (notation & (1 << 20)) {
    Bitboards[2] ^= (1ULL << to);
    Bitboards[promoted + 3] ^= (1ULL << to);
    evalm[color] += (materialm[0] + pstm[0][(56 * color) ^ from]);
    evalm[color] -=
        (materialm[promoted + 1] + pstm[promoted + 1][(56 * color) ^ from]);
    evale[color] += (materiale[0] + pste[0][(56 * color) ^ from]);
    evale[color] -=
        (materiale[promoted + 1] + pste[promoted + 1][(56 * color) ^ to]);
    gamephase[color] -= phase[promoted + 1];
  }
}
int Board::generatemoves(int color, bool capturesonly, int depth) {
  int movecount = 0;
  mobilitym[color] = 0;
  mobilitye[color] = 0;
  int kingsquare = __builtin_ctzll(Bitboards[color] & Bitboards[7]);
  int pinrank = kingsquare & 56;
  int pinfile = kingsquare & 7;
  int opposite = color ^ 1;
  U64 opponentattacks = 0ULL;
  U64 pinnedpieces = 0ULL;
  U64 checkmask = 0ULL;
  U64 preoccupied = Bitboards[0] | Bitboards[1];
  U64 kingRank = GetRankAttacks(preoccupied, kingsquare);
  U64 kingFile = FileAttacks(preoccupied, kingsquare);
  U64 occupied = preoccupied ^ (1ULL << kingsquare);
  U64 opponentpawns = Bitboards[opposite] & Bitboards[2];
  U64 opponentalfils = Bitboards[opposite] & Bitboards[3];
  U64 opponentferzes = Bitboards[opposite] & Bitboards[4];
  U64 opponentknights = Bitboards[opposite] & Bitboards[5];
  U64 opponentrooks = Bitboards[opposite] & Bitboards[6];
  int pawncount = __builtin_popcountll(opponentpawns);
  int alfilcount = __builtin_popcountll(opponentalfils);
  int ferzcount = __builtin_popcountll(opponentferzes);
  int knightcount = __builtin_popcountll(opponentknights);
  int rookcount = __builtin_popcountll(opponentrooks);
  U64 ourcaptures = 0ULL;
  U64 ourmoves = 0ULL;
  U64 ourmask = 0ULL;
  for (int i = 0; i < pawncount; i++) {
    int pawnsquare = __builtin_ctzll(opponentpawns);
    opponentattacks |= PawnAttacks[opposite][pawnsquare];
    opponentpawns ^= (1ULL << pawnsquare);
  }
  U64 opponentpawnattacks = opponentattacks;
  for (int i = 0; i < alfilcount; i++) {
    int alfilsquare = __builtin_ctzll(opponentalfils);
    opponentattacks |= AlfilAttacks[alfilsquare];
    opponentalfils ^= (1ULL << alfilsquare);
  }
  U64 opponentalfilattacks = opponentattacks;
  for (int i = 0; i < ferzcount; i++) {
    int ferzsquare = __builtin_ctzll(opponentferzes);
    opponentattacks |= FerzAttacks[ferzsquare];
    opponentferzes ^= (1ULL << ferzsquare);
  }
  U64 opponentferzattacks = opponentattacks;
  for (int i = 0; i < knightcount; i++) {
    int knightsquare = __builtin_ctzll(opponentknights);
    opponentattacks |= KnightAttacks[knightsquare];
    opponentknights ^= (1ULL << knightsquare);
  }
  U64 opponentknightattacks = opponentattacks;
  for (int i = 0; i < rookcount; i++) {
    int rooksquare = __builtin_ctzll(opponentrooks);
    U64 r = GetRankAttacks(occupied, rooksquare);
    U64 file = FileAttacks(occupied, rooksquare);
    if (!(r & (1ULL << kingsquare))) {
      pinnedpieces |= (r & kingRank);
    } else {
      checkmask |= (GetRankAttacks(preoccupied, rooksquare) & kingRank);
    }
    if (!(file & (1ULL << kingsquare))) {
      pinnedpieces |= (file & kingFile);
    } else {
      checkmask |= (FileAttacks(preoccupied, rooksquare) & kingFile);
    }
    opponentattacks |= (r | file);
    opponentrooks ^= (1ULL << rooksquare);
  }
  int opponentking = __builtin_ctzll(Bitboards[opposite] & Bitboards[7]);
  opponentattacks |= KingAttacks[opponentking];
  ourcaptures =
      KingAttacks[kingsquare] & ((~opponentattacks) & Bitboards[opposite]);
  mobilitye[color] += kingmobe[__builtin_popcountll(KingAttacks[kingsquare] &
                                                    (~opponentattacks))];
  int capturenumber = __builtin_popcountll(ourcaptures);
  int movenumber;
  for (int i = 0; i < capturenumber; i++) {
    int capturesquare = __builtin_ctzll(ourcaptures);
    int notation = kingsquare | (capturesquare << 6);
    notation |= (color << 12);
    notation |= (7 << 13);
    int captured = 0;
    for (int j = 2; j < 7; j++) {
      if ((1ULL << capturesquare) & (Bitboards[opposite] & Bitboards[j])) {
        captured = j;
      }
    }
    notation |= (1 << 16);
    notation |= (captured << 17);
    moves[depth][movecount] = notation;
    movecount++;
    ourcaptures ^= (1ULL << capturesquare);
  }
  if (!capturesonly) {
    ourmoves = KingAttacks[kingsquare] & ((~opponentattacks) & (~preoccupied));
    movenumber = __builtin_popcountll(ourmoves);
    for (int i = 0; i < movenumber; i++) {
      int movesquare = __builtin_ctzll(ourmoves);
      int notation = kingsquare | (movesquare << 6);
      notation |= (color << 12);
      notation |= (7 << 13);
      moves[depth][movecount] = notation;
      movecount++;
      ourmoves ^= (1ULL << movesquare);
    }
  }
  U64 checks = checkers(color);
  if (__builtin_popcountll(checks) > 1) {
    return movecount;
  } else if (checks) {
    checkmask |= checks;
  } else {
    checkmask = ~(0ULL);
  }
  U64 ourpawns = Bitboards[color] & Bitboards[2];
  U64 ouralfils = Bitboards[color] & Bitboards[3];
  U64 ourferzes = Bitboards[color] & Bitboards[4];
  U64 ourknights = Bitboards[color] & Bitboards[5];
  U64 ourrooks = Bitboards[color] & Bitboards[6];
  pawncount = __builtin_popcountll(ourpawns);
  alfilcount = __builtin_popcountll(ouralfils);
  ferzcount = __builtin_popcountll(ourferzes);
  knightcount = __builtin_popcountll(ourknights);
  rookcount = __builtin_popcountll(ourrooks);
  for (int i = 0; i < pawncount; i++) {
    int pawnsquare = __builtin_ctzll(ourpawns);
    if ((pinnedpieces & (1ULL << pawnsquare)) &&
        ((pawnsquare & 56) == pinrank)) {
      ourpawns ^= (1ULL << pawnsquare);
      continue;
    } else if ((pinnedpieces & (1ULL << pawnsquare)) && capturesonly) {
      ourpawns ^= (1ULL << pawnsquare);
      continue;
    }
    int capturenumber = 0;
    if ((pinnedpieces & (1ULL << pawnsquare)) == 0ULL) {
      ourcaptures = PawnAttacks[color][pawnsquare] & Bitboards[opposite];
      ourcaptures &= checkmask;
      capturenumber = __builtin_popcountll(ourcaptures);
    }
    for (int j = 0; j < capturenumber; j++) {
      int capturesquare = __builtin_ctzll(ourcaptures);
      int notation = pawnsquare | (capturesquare << 6);
      notation |= (color << 12);
      notation |= (2 << 13);
      int captured = 0;
      for (int j = 2; j < 7; j++) {
        if ((1ULL << capturesquare) & (Bitboards[opposite] & Bitboards[j])) {
          captured = j;
        }
      }
      notation |= (1 << 16);
      notation |= (captured << 17);
      if (((color == 0) && (capturesquare & 56) == 56) ||
          ((color == 1) && (capturesquare & 56) == 0)) {
        moves[depth][movecount] = notation | (3 << 20);
        movecount++;
      } else {
        moves[depth][movecount] = notation;
        movecount++;
      }
      ourcaptures ^= (1ULL << capturesquare);
    }
    if (!capturesonly) {
      ourmoves = (1ULL << (pawnsquare + 8 * (1 - 2 * color))) & (~preoccupied);
      ourmoves &= checkmask;
      int movenumber = __builtin_popcountll(ourmoves);
      for (int j = 0; j < movenumber; j++) {
        int movesquare = __builtin_ctzll(ourmoves);
        int notation = pawnsquare | (movesquare << 6);
        notation |= (color << 12);
        notation |= (2 << 13);
        if (((color == 0) && (movesquare & 56) == 56) ||
            ((color == 1) && (movesquare & 56) == 0)) {
          moves[depth][movecount] = notation | (3 << 20);
          movecount++;
        } else {
          moves[depth][movecount] = notation;
          movecount++;
        }
        ourmoves ^= (1ULL << movesquare);
      }
    }
    ourpawns ^= (1ULL << pawnsquare);
  }
  for (int i = 0; i < alfilcount; i++) {
    int alfilsquare = __builtin_ctzll(ouralfils);
    if (pinnedpieces & (1ULL << alfilsquare)) {
      ouralfils ^= (1ULL << alfilsquare);
      continue;
    }
    ourmask = AlfilAttacks[alfilsquare];
    mobilitym[color] += alfilmobm[__builtin_popcountll(
        ourmask & (~opponentpawnattacks) & (~Bitboards[color]))];
    mobilitye[color] += alfilmobe[__builtin_popcountll(
        ourmask & (~opponentpawnattacks) & (~Bitboards[color]))];
    ourmask &= checkmask;
    ourcaptures = ourmask & Bitboards[opposite];
    int capturenumber = __builtin_popcountll(ourcaptures);
    for (int j = 0; j < capturenumber; j++) {
      int capturesquare = __builtin_ctzll(ourcaptures);
      int notation = alfilsquare | (capturesquare << 6);
      notation |= (color << 12);
      notation |= (3 << 13);
      int captured = 0;
      for (int j = 2; j < 7; j++) {
        if ((1ULL << capturesquare) & (Bitboards[opposite] & Bitboards[j])) {
          captured = j;
        }
      }
      notation |= (1 << 16);
      notation |= (captured << 17);
      moves[depth][movecount] = notation;
      movecount++;
      ourcaptures ^= (1ULL << capturesquare);
    }
    if (!capturesonly) {
      ourmoves = ourmask & (~preoccupied);
      int movenumber = __builtin_popcountll(ourmoves);
      for (int j = 0; j < movenumber; j++) {
        int movesquare = __builtin_ctzll(ourmoves);
        int notation = alfilsquare | (movesquare << 6);
        notation |= (color << 12);
        notation |= (3 << 13);
        moves[depth][movecount] = notation;
        movecount++;
        ourmoves ^= (1ULL << movesquare);
      }
    }
    ouralfils ^= (1ULL << alfilsquare);
  }
  for (int i = 0; i < ferzcount; i++) {
    int ferzsquare = __builtin_ctzll(ourferzes);
    if (pinnedpieces & (1ULL << ferzsquare)) {
      ourferzes ^= (1ULL << ferzsquare);
      continue;
    }
    ourmask = FerzAttacks[ferzsquare];
    mobilitym[color] += ferzmobm[__builtin_popcountll(
        ourmask & (~opponentalfilattacks) & (~Bitboards[color]))];
    mobilitye[color] += ferzmobe[__builtin_popcountll(
        ourmask & (~opponentalfilattacks) & (~Bitboards[color]))];
    ourmask &= checkmask;
    ourcaptures = ourmask & Bitboards[opposite];
    int capturenumber = __builtin_popcountll(ourcaptures);
    for (int j = 0; j < capturenumber; j++) {
      int capturesquare = __builtin_ctzll(ourcaptures);
      int notation = ferzsquare | (capturesquare << 6);
      notation |= (color << 12);
      notation |= (4 << 13);
      int captured = 0;
      for (int j = 2; j < 7; j++) {
        if ((1ULL << capturesquare) & (Bitboards[opposite] & Bitboards[j])) {
          captured = j;
        }
      }
      notation |= (1 << 16);
      notation |= (captured << 17);
      moves[depth][movecount] = notation;
      movecount++;
      ourcaptures ^= (1ULL << capturesquare);
    }
    if (!capturesonly) {
      ourmoves = ourmask & (~preoccupied);
      int movenumber = __builtin_popcountll(ourmoves);
      for (int j = 0; j < movenumber; j++) {
        int movesquare = __builtin_ctzll(ourmoves);
        int notation = ferzsquare | (movesquare << 6);
        notation |= (color << 12);
        notation |= (4 << 13);
        moves[depth][movecount] = notation;
        movecount++;
        ourmoves ^= (1ULL << movesquare);
      }
    }
    ourferzes ^= (1ULL << ferzsquare);
  }
  for (int i = 0; i < knightcount; i++) {
    int knightsquare = __builtin_ctzll(ourknights);
    if (pinnedpieces & (1ULL << knightsquare)) {
      ourknights ^= (1ULL << knightsquare);
      continue;
    }
    ourmask = KnightAttacks[knightsquare];
    mobilitym[color] += knightmobm[__builtin_popcountll(
        ourmask & (~opponentferzattacks) & (~Bitboards[color]))];
    mobilitye[color] += knightmobe[__builtin_popcountll(
        ourmask & (~opponentferzattacks) & (~Bitboards[color]))];
    ourmask &= checkmask;
    ourcaptures = ourmask & Bitboards[opposite];
    int capturenumber = __builtin_popcountll(ourcaptures);
    for (int j = 0; j < capturenumber; j++) {
      int capturesquare = __builtin_ctzll(ourcaptures);
      int notation = knightsquare | (capturesquare << 6);
      notation |= (color << 12);
      notation |= (5 << 13);
      int captured = 0;
      for (int j = 2; j < 7; j++) {
        if ((1ULL << capturesquare) & (Bitboards[opposite] & Bitboards[j])) {
          captured = j;
        }
      }
      notation |= (1 << 16);
      notation |= (captured << 17);
      moves[depth][movecount] = notation;
      movecount++;
      ourcaptures ^= (1ULL << capturesquare);
    }
    if (!capturesonly) {
      ourmoves = ourmask & (~preoccupied);
      int movenumber = __builtin_popcountll(ourmoves);
      for (int j = 0; j < movenumber; j++) {
        int movesquare = __builtin_ctzll(ourmoves);
        int notation = knightsquare | (movesquare << 6);
        notation |= (color << 12);
        notation |= (5 << 13);
        moves[depth][movecount] = notation;
        movecount++;
        ourmoves ^= (1ULL << movesquare);
      }
    }
    ourknights ^= (1ULL << knightsquare);
  }
  for (int i = 0; i < rookcount; i++) {
    int rooksquare = __builtin_ctzll(ourrooks);
    ourmask = (GetRankAttacks(preoccupied, rooksquare) |
               FileAttacks(preoccupied, rooksquare));
    U64 pinmask = ~(0ULL);
    if (pinnedpieces & (1ULL << rooksquare)) {
      int rookrank = rooksquare & 56;
      if (rookrank == pinrank) {
        pinmask = GetRankAttacks(preoccupied, rooksquare);
      } else {
        pinmask = FileAttacks(preoccupied, rooksquare);
      }
    }
    mobilitym[color] += rookmobm[__builtin_popcountll(
        ourmask & (~opponentknightattacks) & (~Bitboards[color]))];
    mobilitye[color] += rookmobe[__builtin_popcountll(
        ourmask & (~opponentknightattacks) & (~Bitboards[color]))];
    ourmask &= (pinmask & checkmask);
    ourcaptures = ourmask & Bitboards[opposite];
    int capturenumber = __builtin_popcountll(ourcaptures);
    for (int j = 0; j < capturenumber; j++) {
      int capturesquare = __builtin_ctzll(ourcaptures);
      int notation = rooksquare | (capturesquare << 6);
      notation |= (color << 12);
      notation |= (6 << 13);
      int captured = 0;
      for (int j = 2; j < 7; j++) {
        if ((1ULL << capturesquare) & (Bitboards[opposite] & Bitboards[j])) {
          captured = j;
        }
      }
      notation |= (1 << 16);
      notation |= (captured << 17);
      moves[depth][movecount] = notation;
      movecount++;
      ourcaptures ^= (1ULL << capturesquare);
    }
    if (!capturesonly) {
      ourmoves = ourmask & (~preoccupied);
      int movenumber = __builtin_popcountll(ourmoves);
      for (int j = 0; j < movenumber; j++) {
        int movesquare = __builtin_ctzll(ourmoves);
        int notation = rooksquare | (movesquare << 6);
        notation |= (color << 12);
        notation |= (6 << 13);
        moves[depth][movecount] = notation;
        movecount++;
        ourmoves ^= (1ULL << movesquare);
      }
    }
    ourrooks ^= (1ULL << rooksquare);
  }
  return movecount;
}
U64 Board::perft(int depth, int initialdepth, int color) {
  int movcount = generatemoves(color, 0, depth);
  U64 ans = 0;
  if (depth > 1) {
    for (int i = 0; i < movcount; i++) {
      makemove(moves[depth][i], true);
      if (depth == initialdepth) {
        std::cout << algebraic(moves[depth][i]);
        std::cout << ": ";
      }
      ans += perft(depth - 1, initialdepth, color ^ 1);
      unmakemove(moves[depth][i]);
    }
    if (depth == initialdepth - 1) {
      std::cout << ans << " ";
    }
    if (depth == initialdepth) {
      std::cout << "\n" << ans << "\n";
    }
    return ans;
  } else {
    if (initialdepth == 2) {
      std::cout << movcount << " ";
    }
    return movcount;
  }
}
U64 Board::perftnobulk(int depth, int initialdepth, int color) {
  int movcount = generatemoves(color, 0, depth);
  U64 ans = 0;
  for (int i = 0; i < movcount; i++) {
    makemove(moves[depth][i], true);
    if (depth == initialdepth) {
      std::cout << algebraic(moves[depth][i]);
      std::cout << ": ";
    }
    if (depth > 1) {
      ans += perftnobulk(depth - 1, initialdepth, color ^ 1);
    } else {
      ans++;
    }
    unmakemove(moves[depth][i]);
  }
  if (depth == initialdepth - 1) {
    std::cout << ans << " ";
  }
  if (depth == initialdepth) {
    std::cout << "\n" << ans << "\n";
  }
  return ans;
}
void Board::parseFEN(std::string FEN) {
  gamelength = 0;
  root = 0;
  gamephase[0] = 0;
  gamephase[1] = 0;
  evalm[0] = 0;
  evalm[1] = 0;
  evale[0] = 0;
  evale[1] = 0;
  int order[64] = {56, 57, 58, 59, 60, 61, 62, 63, 48, 49, 50, 51, 52,
                   53, 54, 55, 40, 41, 42, 43, 44, 45, 46, 47, 32, 33,
                   34, 35, 36, 37, 38, 39, 24, 25, 26, 27, 28, 29, 30,
                   31, 16, 17, 18, 19, 20, 21, 22, 23, 8,  9,  10, 11,
                   12, 13, 14, 15, 0,  1,  2,  3,  4,  5,  6,  7};
  char file[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
  int progress = 0;
  for (int i = 0; i < 8; i++) {
    Bitboards[i] = 0ULL;
  }
  int tracker = 0;
  int castling = 0;
  int color = 0;
  while (FEN[tracker] != ' ') {
    char hm = FEN[tracker];
    if ('0' <= hm && hm <= '9') {
      int repeat = (int)hm - 48;
      progress += repeat;
    }
    if ('A' <= hm && hm <= 'Z') {
      Bitboards[0] |= (1ULL << order[progress]);
      if (hm == 'P') {
        Bitboards[2] |= (1ULL << order[progress]);
        evalm[0] += materialm[0];
        evale[0] += materiale[0];
        evalm[0] += pstm[0][order[progress]];
        evale[0] += pste[0][order[progress]];
      }
      if (hm == 'A' || hm == 'B') {
        Bitboards[3] |= (1ULL << order[progress]);
        evalm[0] += materialm[1];
        evale[0] += materiale[1];
        evalm[0] += pstm[1][order[progress]];
        evale[0] += pste[1][order[progress]];
        gamephase[0] += 1;
      }
      if (hm == 'F' || hm == 'Q') {
        Bitboards[4] |= (1ULL << order[progress]);
        evalm[0] += materialm[2];
        evale[0] += materiale[2];
        evalm[0] += pstm[2][order[progress]];
        evale[0] += pste[2][order[progress]];
        gamephase[0] += 2;
      }
      if (hm == 'N') {
        Bitboards[5] |= (1ULL << order[progress]);
        evalm[0] += materialm[3];
        evale[0] += materiale[3];
        evalm[0] += pstm[3][order[progress]];
        evale[0] += pste[3][order[progress]];
        gamephase[0] += 4;
      }
      if (hm == 'R') {
        Bitboards[6] |= (1ULL << order[progress]);
        evalm[0] += materialm[4];
        evale[0] += materiale[4];
        evalm[0] += pstm[4][order[progress]];
        evale[0] += pste[4][order[progress]];
        gamephase[0] += 6;
      }
      if (hm == 'K') {
        Bitboards[7] |= (1ULL << order[progress]);
        evalm[0] += pstm[5][order[progress]];
        evale[0] += pste[5][order[progress]];
      }
      progress++;
    }
    if ('a' <= hm && hm <= 'z') {
      Bitboards[1] |= (1ULL << order[progress]);
      if (hm == 'p') {
        Bitboards[2] |= (1ULL << order[progress]);
        evalm[1] += materialm[0];
        evale[1] += materiale[0];
        evalm[1] += pstm[0][56 ^ order[progress]];
        evale[1] += pste[0][56 ^ order[progress]];
      }
      if (hm == 'a' || hm == 'b') {
        Bitboards[3] |= (1ULL << order[progress]);
        evalm[1] += materialm[1];
        evale[1] += materiale[1];
        evalm[1] += pstm[1][56 ^ order[progress]];
        evale[1] += pste[1][56 ^ order[progress]];
        gamephase[1] += 1;
      }
      if (hm == 'f' || hm == 'q') {
        Bitboards[4] |= (1ULL << order[progress]);
        evalm[1] += materialm[2];
        evale[1] += materiale[2];
        evalm[1] += pstm[2][56 ^ order[progress]];
        evale[1] += pste[2][56 ^ order[progress]];
        gamephase[1] += 2;
      }
      if (hm == 'n') {
        Bitboards[5] |= (1ULL << order[progress]);
        evalm[1] += materialm[3];
        evale[1] += materiale[3];
        evalm[1] += pstm[3][56 ^ order[progress]];
        evale[1] += pste[3][56 ^ order[progress]];
        gamephase[1] += 4;
      }
      if (hm == 'r') {
        Bitboards[6] |= (1ULL << order[progress]);
        evalm[1] += materialm[4];
        evale[1] += materiale[4];
        evalm[1] += pstm[4][56 ^ order[progress]];
        evale[1] += pste[4][56 ^ order[progress]];
        gamephase[1] += 6;
      }
      if (hm == 'k') {
        Bitboards[7] |= (1ULL << order[progress]);
        evalm[1] += pstm[5][56 ^ order[progress]];
        evale[1] += pste[5][56 ^ order[progress]];
      }
      progress++;
    }
    tracker++;
  }
  while (FEN[tracker] == ' ') {
    tracker++;
  }
  if (FEN[tracker] == 'b') {
    color = 1;
  }
  position = color;
  tracker += 6;
  int halfmove = (int)(FEN[tracker]) - 48;
  tracker++;
  if (FEN[tracker] != ' ') {
    halfmove = 10 * halfmove + (int)(FEN[tracker]) - 48;
  }
  position |= (halfmove << 1);
  zobristhash = scratchzobrist();
  zobrist[0] = zobristhash;
  history[0] = position;
}
std::string Board::getFEN() {
  int order[64] = {56, 57, 58, 59, 60, 61, 62, 63, 48, 49, 50, 51, 52,
                   53, 54, 55, 40, 41, 42, 43, 44, 45, 46, 47, 32, 33,
                   34, 35, 36, 37, 38, 39, 24, 25, 26, 27, 28, 29, 30,
                   31, 16, 17, 18, 19, 20, 21, 22, 23, 8,  9,  10, 11,
                   12, 13, 14, 15, 0,  1,  2,  3,  4,  5,  6,  7};
  std::string FEN = "";
  int empt = 0;
  char convert[2][6] = {{'P', 'B', 'Q', 'N', 'R', 'K'},
                        {'p', 'b', 'q', 'n', 'r', 'k'}};
  int color;
  int piece;
  for (int i = 0; i < 64; i++) {
    color = -1;
    for (int j = 0; j < 2; j++) {
      if (Bitboards[j] & (1ULL << order[i])) {
        color = j;
      }
    }
    if (color >= 0) {
      if (empt > 0) {
        FEN = FEN + (char)(empt + 48);
        empt = 0;
      }
      for (int j = 0; j < 6; j++) {
        if (Bitboards[j + 2] & (1ULL << order[i])) {
          piece = j;
        }
      }
      FEN = FEN + (convert[color][piece]);
    } else {
      empt++;
      if ((i & 7) == 7) {
        FEN = FEN + (char)(empt + 48);
        empt = 0;
      }
    }
    if (((i & 7) == 7) && (i < 63)) {
      FEN = FEN + '/';
    }
  }
  FEN = FEN + ' ';
  if (position & 1) {
    FEN = FEN + "b - - ";
  } else {
    FEN = FEN + "w - - ";
  }
  int halfmove = position >> 1;
  std::string bruh = "";
  do {
    bruh = bruh + (char)(halfmove % 10 + 48);
    halfmove /= 10;
  } while (halfmove > 0);
  reverse(bruh.begin(), bruh.end());
  FEN = FEN + bruh + " 1";
  return FEN;
}
int Board::evaluate(int color) {
  int midphase = std::min(48, gamephase[0] + gamephase[1]);
  int endphase = 48 - midphase;
  int mideval =
      evalm[color] + mobilitym[color] - evalm[color ^ 1] - mobilitym[color ^ 1];
  int endeval =
      evale[color] + mobilitye[color] - evale[color ^ 1] - mobilitye[color ^ 1];
  int progress = 200 - (position >> 1);
  int base = (mideval * midphase + endeval * endphase) / 48 + 10;
  return (base * progress) / 200;
}
bool Board::see_exceeds(int mov, int color, int threshold) {
  int see_values[6] = {100, 100, 170, 370, 640, 20000};
  int target = (mov >> 6) & 63;
  int victim = (mov >> 17) & 7;
  int attacker = (mov >> 13) & 7;
  int value = (victim > 0) ? see_values[victim - 2] - threshold : -threshold;
  if (value < 0) {
    return false;
  }
  if (value - see_values[attacker - 2] >= 0) {
    return true;
  }
  int pieces[2][6] = {{0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}};
  U64 occupied = Bitboards[0] | Bitboards[1];
  occupied ^= (1ULL << (mov & 63));
  U64 us = Bitboards[color];
  U64 enemy = Bitboards[color ^ 1];
  U64 alfils = AlfilAttacks[target] & Bitboards[3];
  U64 ferzes = FerzAttacks[target] & Bitboards[4];
  U64 knights = KnightAttacks[target] & Bitboards[5];
  U64 kings = KingAttacks[target] & Bitboards[7];
  occupied ^= (enemy & Bitboards[6]);
  pieces[0][0] =
      __builtin_popcountll((PawnAttacks[color][target] & Bitboards[2] & enemy));
  pieces[0][1] = __builtin_popcountll(alfils & enemy);
  pieces[0][2] = __builtin_popcountll(ferzes & enemy);
  pieces[0][3] = __builtin_popcountll(knights & enemy);
  pieces[0][4] = __builtin_popcountll(
      (FileAttacks(occupied, target) | GetRankAttacks(occupied, target)) &
      Bitboards[6] & enemy);
  pieces[0][5] = __builtin_popcountll(kings & enemy);
  occupied ^= (Bitboards[6]);
  pieces[1][0] = __builtin_popcountll(
      (PawnAttacks[color ^ 1][target] & Bitboards[2] & us));
  pieces[1][1] = __builtin_popcountll(alfils & us);
  pieces[1][2] = __builtin_popcountll(ferzes & us);
  pieces[1][3] = __builtin_popcountll(knights & us);
  pieces[1][4] = __builtin_popcountll(
      (FileAttacks(occupied, target) | GetRankAttacks(occupied, target)) &
      Bitboards[6] & us);
  pieces[1][5] = __builtin_popcountll(kings & us);
  if (attacker > 2) {
    pieces[1][attacker - 2]--;
  }
  int next[2] = {0, 0};
  int previous[2] = {0, attacker - 2};
  int i = 0;
  while (true) {
    while (pieces[i][next[i]] == 0 && next[i] < 6) {
      next[i]++;
    }
    if (next[i] > 5) {
      return (value >= 0);
    }
    value += (2 * i - 1) * see_values[previous[i ^ 1]];
    if ((2 * i - 1) * (value + (1 - 2 * i) * see_values[next[i]]) >= 1 - i) {
      return i;
    }
    previous[i] = next[i];
    pieces[i][next[i]]--;
    i ^= 1;
  }
}
int Board::probetbwdl() {
  auto wdl =
      tb_probe_wdl(Bitboards[0], Bitboards[1], Bitboards[7], Bitboards[4],
                   Bitboards[6], Bitboards[3], Bitboards[5], Bitboards[2], 0U,
                   0U, 0U, ((position & 1) == 0));
  return (wdl > 2) - (wdl < 2);
}
