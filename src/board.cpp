#include "board.h"
#include "consts.h"
#include "eval/hce.h"
#include "external/probetool/jtbprobe.h"
#include "external/probetool/jtbinterface.h"
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
  for (U64 i = 0ULL; i < 64; i++) {
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
  zobrist[0] = zobristhash = scratchzobrist();
  if (tbpos == nullptr) {
    tbpos = TBitf_alloc_position();
  }
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
U64 Board::keyafter(int notation) {
  int from = notation & 63;
  int to = (notation >> 6) & 63;
  int color = (notation >> 12) & 1;
  int piece = (notation >> 13) & 7;
  int captured = (notation >> 17) & 7;
  int promoted = (notation >> 21) & 3;
  int piece2 = (promoted > 0) ? piece + 2 : piece;
  U64 change = (colorhash ^ hashes[color][from] ^ hashes[color][to] ^
                hashes[piece][from] ^ hashes[piece2][to]);
  if (captured) {
    change ^= (hashes[color ^ 1][to] ^ hashes[captured][to]);
  }
  return zobristhash ^ change;
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
    gamephase[color] -= phase[0];
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
    gamephase[color] += phase[0];
  }
}
int Board::generatemoves(int color, bool capturesonly, int *movelist) {
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
    movelist[movecount] = notation;
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
      movelist[movecount] = notation;
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
        movelist[movecount] = notation | (3 << 20);
        movecount++;
      } else {
        movelist[movecount] = notation;
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
          movelist[movecount] = notation | (3 << 20);
          movecount++;
        } else {
          movelist[movecount] = notation;
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
      movelist[movecount] = notation;
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
        movelist[movecount] = notation;
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
      movelist[movecount] = notation;
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
        movelist[movecount] = notation;
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
      movelist[movecount] = notation;
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
        movelist[movecount] = notation;
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
      movelist[movecount] = notation;
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
        movelist[movecount] = notation;
        movecount++;
        ourmoves ^= (1ULL << movesquare);
      }
    }
    ourrooks ^= (1ULL << rooksquare);
  }
  return movecount;
}
U64 Board::perft(int depth, int initialdepth, int color) {
  int moves[maxmoves];
  int movcount = generatemoves(color, 0, moves);
  U64 ans = 0;
  if (depth > 1) {
    for (int i = 0; i < movcount; i++) {
      makemove(moves[i], true);
      if (depth == initialdepth) {
        std::cout << algebraic(moves[i]);
        std::cout << ": ";
      }
      ans += perft(depth - 1, initialdepth, color ^ 1);
      unmakemove(moves[i]);
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
  int moves[maxmoves];
  int movcount = generatemoves(color, 0, moves);
  U64 ans = 0;
  for (int i = 0; i < movcount; i++) {
    makemove(moves[i], true);
    if (depth == initialdepth) {
      std::cout << algebraic(moves[i]);
      std::cout << ": ";
    }
    if (depth > 1) {
      ans += perftnobulk(depth - 1, initialdepth, color ^ 1);
    } else {
      ans++;
    }
    unmakemove(moves[i]);
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
        gamephase[0] += phase[0];
      }
      if (hm == 'A' || hm == 'B') {
        Bitboards[3] |= (1ULL << order[progress]);
        evalm[0] += materialm[1];
        evale[0] += materiale[1];
        evalm[0] += pstm[1][order[progress]];
        evale[0] += pste[1][order[progress]];
        gamephase[0] += phase[1];
      }
      if (hm == 'F' || hm == 'Q') {
        Bitboards[4] |= (1ULL << order[progress]);
        evalm[0] += materialm[2];
        evale[0] += materiale[2];
        evalm[0] += pstm[2][order[progress]];
        evale[0] += pste[2][order[progress]];
        gamephase[0] += phase[2];
      }
      if (hm == 'N') {
        Bitboards[5] |= (1ULL << order[progress]);
        evalm[0] += materialm[3];
        evale[0] += materiale[3];
        evalm[0] += pstm[3][order[progress]];
        evale[0] += pste[3][order[progress]];
        gamephase[0] += phase[3];
      }
      if (hm == 'R') {
        Bitboards[6] |= (1ULL << order[progress]);
        evalm[0] += materialm[4];
        evale[0] += materiale[4];
        evalm[0] += pstm[4][order[progress]];
        evale[0] += pste[4][order[progress]];
        gamephase[0] += phase[4];
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
        gamephase[1] += phase[0];
      }
      if (hm == 'a' || hm == 'b') {
        Bitboards[3] |= (1ULL << order[progress]);
        evalm[1] += materialm[1];
        evale[1] += materiale[1];
        evalm[1] += pstm[1][56 ^ order[progress]];
        evale[1] += pste[1][56 ^ order[progress]];
        gamephase[1] += phase[1];
      }
      if (hm == 'f' || hm == 'q') {
        Bitboards[4] |= (1ULL << order[progress]);
        evalm[1] += materialm[2];
        evale[1] += materiale[2];
        evalm[1] += pstm[2][56 ^ order[progress]];
        evale[1] += pste[2][56 ^ order[progress]];
        gamephase[1] += phase[2];
      }
      if (hm == 'n') {
        Bitboards[5] |= (1ULL << order[progress]);
        evalm[1] += materialm[3];
        evale[1] += materiale[3];
        evalm[1] += pstm[3][56 ^ order[progress]];
        evale[1] += pste[3][56 ^ order[progress]];
        gamephase[1] += phase[3];
      }
      if (hm == 'r') {
        Bitboards[6] |= (1ULL << order[progress]);
        evalm[1] += materialm[4];
        evale[1] += materiale[4];
        evalm[1] += pstm[4][56 ^ order[progress]];
        evale[1] += pste[4][56 ^ order[progress]];
        gamephase[1] += phase[4];
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
  int success = 0;
  /*int rule70 = 0;
  TBitf_set_from_fen(tbpos, getFEN().c_str(), &rule70);
  int result = TB_probe_wdl(tbpos, &success);
  std::cout << "Raw result (fen): " << result << "\n";*/
  uint8_t convert[8] = {0, 0, 1, 3, 5, 2, 4, 6};
  tbpos->pt[0] = 6;
  tbpos->pt[1] = 14;
  tbpos->idx = 0;
  tbpos->stm = position & 1;
  tbpos->occ = Bitboards[0] | Bitboards[1];
  tbpos->state[0] = (State) { 0 };
  tbpos->sq[0] = __builtin_ctzll(Bitboards[0] & Bitboards[7]);
  tbpos->sq[1] = __builtin_ctzll(Bitboards[1] & Bitboards[7]);
  U64 occ = __builtin_bswap64((Bitboards[0] | Bitboards[1]) & ~Bitboards[7]);
  tbpos->cnt[0] = __builtin_popcountll(Bitboards[0]) - 1;
  tbpos->cnt[1] = __builtin_popcountll(Bitboards[1]) - 1;
  tbpos->num = 2;
  while (occ) {
    int sq = 56 ^ __builtin_ctzll(occ);
    tbpos->sq[tbpos->num] = sq;
    int piece = 0;
    for (int c = 0; c < 2; c++) {
      if (Bitboards[c] & (1ULL << sq)) {
        piece += 8*c;
        break;
      }
    }
    for (int p = 2; p < 7; p++) {
      if (Bitboards[p] & (1ULL << sq)) {
        piece += convert[p];
        break;
      }
    }
    tbpos->pt[tbpos->num++] = piece;
    occ &= (occ - 1);
  }
  int result = TB_probe_wdl(tbpos, &success);
  //std::cout << "Raw result (Bitboard): " << result << "\n";
  
  return success ? (result > 0) - (result < 0) : -3;
}
