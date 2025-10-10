/*
  Copyright (c) 2025 Ronald de Man
  This file may be redistributed and/or modified without restrictions.
*/

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jtbprobe.h"
#include "jtbinterface.h"

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#define assume(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#else
#define assume(x) do { } while (0)
#endif

#define INLINE static inline __attribute__((always_inline))

enum {
  WHITE = 0,
  BLACK = 1
};

enum {
  PAWN = 1,
  KNIGHT = 2,
  BISHOP = 3,
  ROOK = 4,
  QUEEN = 5,
  KING = 6
};

/* Magic sliding attack generation */

typedef uint64_t Bitboard;

INLINE int popcount(Bitboard b)
{
  return __builtin_popcountll(b);
}

INLINE int lsb(Bitboard b)
{
  return __builtin_ctzll(b);
}

INLINE int pop_lsb(Bitboard *b)
{
  int s = lsb(*b);
  *b &= *b - 1;
  return s;
}

struct Magic {
  Bitboard *data;
  Bitboard mask;
  uint64_t magic;
};

static Bitboard AttacksTable[88772];
static struct Magic RookMagic[64];

static Bitboard Bit[64];

static Bitboard PawnAttacks[2][64];
static Bitboard KnightAttacks[64];
static Bitboard BishopAttacks[64];
static Bitboard QueenAttacks[64];
static Bitboard KingAttacks[64];

INLINE Bitboard bit(int sq)
{
  return Bit[sq];
}

INLINE Bitboard rook_attacks(int sq, Bitboard occ)
{
  struct Magic *mag = &RookMagic[sq];
  return mag->data[((occ & mag->mask) * mag->magic) >> (64-12)];
}

INLINE Bitboard bishop_attacks(int sq)
{
  return BishopAttacks[sq];
}

INLINE Bitboard queen_attacks(int sq)
{
  return QueenAttacks[sq];
}

INLINE Bitboard knight_attacks(int sq)
{
  return KnightAttacks[sq];
}

INLINE Bitboard king_attacks(int sq)
{
  return KingAttacks[sq];
}

INLINE Bitboard pawn_attacks(int c, int sq)
{
  return PawnAttacks[c][sq];
}

struct MagicInit {
  Bitboard magic;
  int index;
};

// Fixed-shift magics found by Volker Annuss
// from: http://talkchess.com/forum/viewtopic.php?p=727500#727500

static struct MagicInit RookInit[64] = {
  { 0x00280077ffebfffeu,  26304 },
  { 0x2004010201097fffu,  35520 },
  { 0x0010020010053fffu,  38592 },
  { 0x0040040008004002u,   8026 },
  { 0x7fd00441ffffd003u,  22196 },
  { 0x4020008887dffffeu,  80870 },
  { 0x004000888847ffffu,  76747 },
  { 0x006800fbff75fffdu,  30400 },
  { 0x000028010113ffffu,  11115 },
  { 0x0020040201fcffffu,  18205 },
  { 0x007fe80042ffffe8u,  53577 },
  { 0x00001800217fffe8u,  62724 },
  { 0x00001800073fffe8u,  34282 },
  { 0x00001800e05fffe8u,  29196 },
  { 0x00001800602fffe8u,  23806 },
  { 0x000030002fffffa0u,  49481 },
  { 0x00300018010bffffu,   2410 },
  { 0x0003000c0085fffbu,  36498 },
  { 0x0004000802010008u,  24478 },
  { 0x0004002020020004u,  10074 },
  { 0x0001002002002001u,  79315 },
  { 0x0001001000801040u,  51779 },
  { 0x0000004040008001u,  13586 },
  { 0x0000006800cdfff4u,  19323 },
  { 0x0040200010080010u,  70612 },
  { 0x0000080010040010u,  83652 },
  { 0x0004010008020008u,  63110 },
  { 0x0000040020200200u,  34496 },
  { 0x0002008010100100u,  84966 },
  { 0x0000008020010020u,  54341 },
  { 0x0000008020200040u,  60421 },
  { 0x0000820020004020u,  86402 },
  { 0x00fffd1800300030u,  50245 },
  { 0x007fff7fbfd40020u,  76622 },
  { 0x003fffbd00180018u,  84676 },
  { 0x001fffde80180018u,  78757 },
  { 0x000fffe0bfe80018u,  37346 },
  { 0x0001000080202001u,    370 },
  { 0x0003fffbff980180u,  42182 },
  { 0x0001fffdff9000e0u,  45385 },
  { 0x00fffefeebffd800u,  61659 },
  { 0x007ffff7ffc01400u,  12790 },
  { 0x003fffbfe4ffe800u,  16762 },
  { 0x001ffff01fc03000u,      0 },
  { 0x000fffe7f8bfe800u,  38380 },
  { 0x0007ffdfdf3ff808u,  11098 },
  { 0x0003fff85fffa804u,  21803 },
  { 0x0001fffd75ffa802u,  39189 },
  { 0x00ffffd7ffebffd8u,  58628 },
  { 0x007fff75ff7fbfd8u,  44116 },
  { 0x003fff863fbf7fd8u,  78357 },
  { 0x001fffbfdfd7ffd8u,  44481 },
  { 0x000ffff810280028u,  64134 },
  { 0x0007ffd7f7feffd8u,  41759 },
  { 0x0003fffc0c480048u,   1394 },
  { 0x0001ffffafd7ffd8u,  40910 },
  { 0x00ffffe4ffdfa3bau,  66516 },
  { 0x007fffef7ff3d3dau,   3897 },
  { 0x003fffbfdfeff7fau,   3930 },
  { 0x001fffeff7fbfc22u,  72934 },
  { 0x0000020408001001u,  72662 },
  { 0x0007fffeffff77fdu,  56325 },
  { 0x0003ffffbf7dfeecu,  66501 },
  { 0x0001ffff9dffa333u,  14826 } 
};

static signed char PawnDelta[2][2][2] = {
  { {  7,  15 }, {  9,  17 } },
  { { -9, -17 }, { -7, -15 } }
};

static signed char KnightDelta[8][2] = {
  { -17, -33 }, { -15, -31 }, { -10, -18 }, { -6, -14 },
  {   6,  14 }, {  10,  18 }, {  15,  31 }, { 17,  33 }
};

static signed char BishopDelta[4][2] = {
  { -18, -34 }, { -14, -30 }, { 14, 30 }, { 18, 34 }
};

static signed char RookDelta[4][2] = {
  { -8, -16 }, { -1, -1 }, { 1, 1 }, { 8, 16 }
};

static signed char QueenDelta[4][2] = {
  { -9, -17 }, { -7, -15 }, { 7, 15 }, { 9, 17 }
};

static signed char KingDelta[8][2] = {
  { -9, -17 }, { -8, -16 }, { -7, -15 }, { -1, -1 },
  {  1,   1 }, {  7,  15 }, {  8,  16 }, {  9, 17 }
};

INLINE bool valid(int sq, signed char delta[2])
{
  int sq88 = sq + (sq & ~7);
  return !((sq88 + delta[1]) & 0x88);
}

static Bitboard calc_attacks(int sq, signed char delta[][2], int num)
{
  Bitboard bb = 0;

  for (int d = 0; d < num; d++)
    if (valid(sq, delta[d]))
      bb |= bit(sq + delta[d][0]);

  return bb;
}

static void init_magics(struct MagicInit *magicInit, struct Magic *magic,
    signed char delta[][2], int shift)
{
  for (int sq = 0; sq < 64; sq++) {
    magic[sq].magic = magicInit[sq].magic;
    magic[sq].data = &AttacksTable[magicInit[sq].index];

    // Calculate mask
    Bitboard mask = 0;
    for (int i = 0; i < 4; i++) {
      if (!valid(sq, delta[i])) continue;
      for (int s = sq + delta[i][0]; valid(s, delta[i]); s += delta[i][0])
        mask |= bit(s);
    }
    magic[sq].mask = mask;

    // Use Carry-Rippler trick
    Bitboard b  = 0;
    do {
      Bitboard attacks = 0;
      for (int j = 0; j < 4; j++)
        for (int s = sq; valid(s, delta[j]); s += delta[j][0]) {
          attacks |= bit(s + delta[j][0]);
          if (b & bit(s + delta[j][0])) break;
        }
      magic[sq].data[(b * magic[sq].magic) >> shift] = attacks;
      b = (b - mask) & mask;
    } while (b);
  }
}

void TBitf_init(void)
{
  for (int sq = 0; sq < 64; sq++)
    Bit[sq] = 1ULL << sq;

  for (int sq = 0; sq < 64; sq++) {
    PawnAttacks[WHITE][sq] = calc_attacks(sq, PawnDelta[WHITE], 2);
    PawnAttacks[BLACK][sq] = calc_attacks(sq, PawnDelta[BLACK], 2);
    KnightAttacks[sq]      = calc_attacks(sq, KnightDelta,      8);
    BishopAttacks[sq]      = calc_attacks(sq, BishopDelta,      4);
    QueenAttacks[sq]       = calc_attacks(sq, QueenDelta,       4);
    KingAttacks[sq]        = calc_attacks(sq, KingDelta,        8);
  }

  init_magics(RookInit, RookMagic, RookDelta, 64 - 12);
}

INLINE Bitboard piece_attacks(int pt, int sq, Bitboard occ)
{
  switch (pt & 7) {
  case KNIGHT:
    return knight_attacks(sq);
  case BISHOP:
    return bishop_attacks(sq);
  case ROOK:
    return rook_attacks(sq, occ);
  case QUEEN:
    return queen_attacks(sq);
  case KING:
    return king_attacks(sq);
  }
  assume(0);
}

INLINE Bitboard attacks(int pt, int sq, Bitboard occ)
{
  if ((pt & 7) == PAWN)
    return pawn_attacks(pt >> 3, sq);
  else
    return piece_attacks(pt, sq, occ);
}

typedef struct TB_Position TB_Position;
typedef struct State State;

#define CAPT_FLAG 0x08
#define PROM_FLAG 0x20

// A capture move encodes the indices of the capturing and captured pieces.
INLINE Move make_capture(int i, int j)
{
  return i | CAPT_FLAG | (j << 8);
}

INLINE Move make_prom_capture(int i, int j)
{
  return i | CAPT_FLAG | PROM_FLAG | (j << 8);
}

// A non-capture move encodes the index of the moving piece and the
// destination square.
INLINE Move make_quiet(int i, int sq)
{
  return i | (sq << 8);
}

INLINE Move make_prom(int i, int sq)
{
  return i | PROM_FLAG | (sq << 8);
}

TB_Position *TBitf_alloc_position(void)
{
  TB_Position *pos = (TB_Position *)malloc(sizeof *pos);
  pos->maxIdx = 7;
  pos->maxMoves = 500;
  pos->state = (State *)malloc(pos->maxIdx * sizeof(struct State));
  pos->move = (Move *)malloc(pos->maxMoves * sizeof(Move));

  return pos;
}

void TBitf_free_position(TB_Position *pos)
{
  free(pos->state);
  free(pos->move);
  free(pos);
}

static const char PieceChar[] = " PNBRQK  pnbrqk";

static const uint64_t MatKey[] = {
  0, 0xd8c54b6242cc4658, 0xb84cd5fd6adf1a60, 0x0c8fa4e03da65e01,
  0xa16591be7916b4ac, 0x6e4682f9525cc4c4, 0, 0,
  0, 0x60adc383afac9d1b, 0x97bbdd24afa0b2d1, 0x298cefb2f9bfac89,
  0x6b16a6bc4040b7c2, 0xf7153c5390f198ac, 0, 0
};

uint64_t TB_material_key(TB_Position *pos)
{
  uint64_t key = 0;

  // We can skip the two kings
  for (int i = 2; i < pos->num; i++)
    key += MatKey[pos->pt[i]];

  return key;
}

uint64_t TB_material_key_from_counts(int whiteCounts[8], int blackCounts[8])
{
  uint64_t key = 0;

  for (int i = PAWN; i <= QUEEN; i++)
    key +=  whiteCounts[i] * MatKey[i]
          + blackCounts[i] * MatKey[i + 8];

  return key;
}

void TB_material_string(TB_Position *pos, char str[16])
{
  int cnt[16];

  for (int i = 0; i < 16; i++)
    cnt[i] = 0;
  for (int i = 0; i < pos->num; i++)
    cnt[pos->pt[i]]++;

  int j = 0;
  for (int i = KING; i >= PAWN; i--)
    while (cnt[i]--)
      str[j++] = PieceChar[i];
  str[j++] = 'v';
  for (int i = KING; i >= PAWN; i--)
    while (cnt[8 + i]--)
      str[j++] = PieceChar[i];
  str[j] = 0;
}

void TB_list_squares(TB_Position *pos, const uint8_t *pt, bool flip, int *p)
{
  for (int i = 0; i < pos->num; ) {
    int t = pt[i] ^ (flip << 3);
    for (int j = 0; j < pos->num; j++)
      if (pos->pt[j] == t)
        p[i++] = pos->sq[j] ^ (flip ? 0x38 : 0x00);
  }
}

bool TB_white_to_move(TB_Position *pos)
{
  return pos->stm == WHITE;
}

bool TB_bare_king(TB_Position *pos, int *v)
{
  if (pos->cnt[pos->stm])
    return false;

  // Still a draw if the bare king can capture the opponent's last piece.
  *v = pos->num == 3 && (   king_attacks(pos->sq[pos->stm])
                         & ~king_attacks(pos->sq[pos->stm ^ 1])
                         &  bit(pos->sq[2])) ? 0 : -2;

  return true;
}

bool TB_move_is_legal(TB_Position *pos, int m)
{
  if (!TB_do_move(pos, m))
    return false;
  TB_undo_move(pos, m);
  return true;
}

bool TB_no_legal_moves(TB_Position *pos)
{
  int num = TB_generate_captures(pos);
  num = TB_generate_quiets(pos, num);
  for (int m = 0; m < num; m++)
    if (TB_move_is_legal(pos, m))
      return false;
  return true;
}

bool TB_move_is_pawn_move(TB_Position *pos, int m)
{
  Move move = pos->move[pos->state[pos->idx].firstMove + m];
  return (pos->pt[move & 7] & 7) == PAWN;
}

INLINE int piece_idx(TB_Position *pos, int sq)
{
  for (int i = 0; i < pos->num; i++)
    if (pos->sq[i] == sq)
      return i;
  assume(false); // signal to the compiler that this path is unreachable
  return 0;
}

INLINE bool rank18(int sq)
{
  return (unsigned)(sq - 8) >= 48;
}

int TB_generate_captures(TB_Position *pos)
{
  int m = pos->state[pos->idx].firstMove;
  Bitboard b;

  if (m + 150 >= pos->maxMoves) {
    pos->maxMoves += 500;
    pos->move = (Move *)realloc(pos->move, pos->maxMoves * sizeof(Move));
  }

  // generate regular captures and captures with promotion
  for (int i = 0; i < pos->num; i++) {
    if ((pos->pt[i] >> 3) != pos->stm)
      continue;
    if ((pos->pt[i] & 7) == PAWN) {
      b = pawn_attacks(pos->pt[i] >> 3, pos->sq[i]) & pos->occ;
      while (b) {
        int sq = pop_lsb(&b);
        int j = piece_idx(pos, sq);
        if (!((pos->pt[i] ^ pos->pt[j]) & 8))
          continue;
        if (!rank18(sq))
          pos->move[m++] = make_capture(i, j);
        else
          pos->move[m++] = make_prom_capture(i, j);
      }
    }
    else {
      b = piece_attacks(pos->pt[i] & 7, pos->sq[i], pos->occ) & pos->occ;
      while (b) {
        int sq = pop_lsb(&b);
        int j = piece_idx(pos, sq);
        if ((pos->pt[i] ^ pos->pt[j]) & 0x08)
          pos->move[m++] = make_capture(i, j);
      }
    }
  }

  pos->state[pos->idx + 1].firstMove = m;
  return m - pos->state[pos->idx].firstMove;
}

int TB_generate_quiets(TB_Position *pos, int start)
{
  int m = pos->state[pos->idx].firstMove + start;
  Bitboard b;

  if (m + 150 >= pos->maxMoves) {
    pos->maxMoves += 500;
    pos->move = (Move *)realloc(pos->move, pos->maxMoves * sizeof(Move));
  }

  for (int i = 0; i < pos->num; i++) {
    if ((pos->pt[i] >> 3) != pos->stm)
      continue;
    if ((pos->pt[i] & 7) == PAWN) { // pawn moves
      int sq = pos->sq[i];
      int fwd = pos->stm == WHITE ? 8 : -8;
      if (!(bit(sq + fwd) & pos->occ)) {
        if (!rank18(sq + fwd))
          pos->move[m++] = make_quiet(i, sq + fwd);
        else
          pos->move[m++] = make_prom(i, sq + fwd);
      }
    } else { // non-pawn moves
      b = piece_attacks(pos->pt[i] & 7, pos->sq[i], pos->occ) & ~pos->occ;
      while (b) {
        int sq = pop_lsb(&b);
        pos->move[m++] = make_quiet(i, sq);
      }
    }
  }

  pos->state[pos->idx + 1].firstMove = m;
  return m - pos->state[pos->idx].firstMove;
}

INLINE bool king_attacked(TB_Position *pos, int stm)
{
  Bitboard b = bit(pos->sq[stm]); // square of king
  for (int i = 0; i < pos->num; i++)
    if (   (pos->pt[i] >> 3) == (stm ^ 1)
        && (attacks(pos->pt[i], pos->sq[i], pos->occ) & b))
      return true;
  return false;
} 

INLINE bool opp_king_attacked(TB_Position *pos)
{
  return king_attacked(pos, pos->stm ^ 1);
}

bool TB_in_check(TB_Position *pos)
{
  return king_attacked(pos, pos->stm);
}

bool TB_do_move(TB_Position *pos, int m)
{
  if (pos->idx + 2 >= pos->maxIdx) {
    pos->maxIdx += 100;
    pos->state = (State *)realloc(pos->state, pos->maxIdx * sizeof(struct State));
  }

  Move move = pos->move[pos->state[pos->idx].firstMove + m];

  int i = move & 7, j = move >> 8;
  pos->state[pos->idx++].fromSquare = pos->sq[i];
  pos->occ ^= bit(pos->sq[i]);
  if (move & PROM_FLAG)
    pos->pt[i] += QUEEN - PAWN;
  if (move & CAPT_FLAG) {
    pos->sq[i] = pos->sq[j];
    pos->pt[pos->num--] = pos->pt[j];
    pos->cnt[pos->stm ^ 1]--;
    pos->sq[j] = pos->sq[pos->num];
    pos->pt[j] = pos->pt[pos->num];
  } else {
    pos->sq[i] = j;
    pos->occ ^= bit(j);
  }
  pos->stm ^= 1;

  if (opp_king_attacked(pos)) {
    TB_undo_move(pos, m);
    return false;
  }

  return true;
}

void TB_undo_move(TB_Position *pos, int m)
{
  Move move = pos->move[pos->state[--pos->idx].firstMove + m];

  int i = move & 7, j = move >> 8;
  if (move & CAPT_FLAG) {
    pos->pt[pos->num] = pos->pt[j];
    pos->sq[pos->num] = pos->sq[j];
    pos->pt[j] = pos->pt[++pos->num];
    pos->sq[j] = pos->sq[i];
    pos->cnt[pos->stm]++;
  } else {
    pos->occ ^= bit(j);
  }
  if (move & PROM_FLAG)
    pos->pt[i] -= QUEEN - PAWN;
  pos->sq[i] = pos->state[pos->idx].fromSquare;
  pos->occ ^= bit(pos->sq[i]);
  pos->stm ^= 1;
}

void TBitf_set_from_fen(TB_Position *pos, const char *fen, int *cnt70)
{
  int f, r;
  char c;

  pos->pt[0] = KING;
  pos->pt[1] = KING + 8;
  pos->sq[0] = pos->sq[1] = 0xff;
  pos->num = 2;
  pos->idx = 0;
  pos->state[0] = (State){ 0 };
  *cnt70 = 0;

  for (f = 0, r = 7; (c = *fen) && !isspace(c); fen++) {
    if (c == '/' && r > 0) {
      r--;
      f = 0;
      continue;
    }
    if (f == 8)
      goto illegal_fen;
    if (isdigit(c) && f + (c - '0') <= 8)
      f += c - '0';
    else if (c == 'K' && pos->sq[0] == 0xff)
      pos->sq[0] = (r << 3) | f++;
    else if (c == 'k' && pos->sq[1] == 0xff)
      pos->sq[1] = (r << 3) | f++;
    else if (c == 'K' || c == 'k')
      goto illegal_fen;
    else if (strchr(PieceChar, c)) {
      if (pos->num == 7)
        goto too_many_pieces;
      pos->pt[pos->num] = strchr(PieceChar, c) - PieceChar;
      pos->sq[pos->num++] = (r << 3) | f++;
    }
    else
      goto illegal_fen;
  }
  if (!isspace(c) || r > 0 || pos->sq[0] == 0xff || pos->sq[1] == 0xff)
    goto illegal_fen;

  // side to move
  while (isspace(c = *fen++)) ;
  if (c == 'w')
    pos->stm = WHITE;
  else if (c == 'b')
    pos->stm = BLACK;
  else
    goto illegal_fen;

  // castling rights
  while (isspace(c = *fen++)) ;
  if (!c)
    goto finish;
  if (strchr("kqKQ", c)) {
    fprintf(stderr, "Castling rights not allowed.\n");
    exit(EXIT_FAILURE);
  }
  if (c != '-')
    goto illegal_fen;

  // ep rights
  while (isspace(c = *fen++)) ;
  if (!c)
    goto finish;
  if (c != '-')
    goto illegal_fen;

  // Halfmove clock
  while (isspace(c = *fen++)) ;
  if (!c)
    goto finish;
  while (isdigit(c)) {
    *cnt70 = *cnt70 * 10 + (c - '0');
    c = *fen++;
  }

  // We ignore the fullmove number.

finish:
  pos->occ = 0;
  for (int i = 0; i < pos->num; i++)
    pos->occ |= bit(pos->sq[i]);

  pos->cnt[WHITE] = pos->cnt[BLACK] = 0;
  for (int i = 2; i < pos->num; i++)
    pos->cnt[pos->pt[i] >> 3]++;

  if (!opp_king_attacked(pos))
    return;

illegal_fen:
  fprintf(stderr, "Illegal fen.\n");
  exit(EXIT_FAILURE);

too_many_pieces:
  fprintf(stderr, "Too many pieces.\n");
  exit(EXIT_FAILURE);
}

// Convert the mth move into algebraic notation
void TBitf_move_to_string(TB_Position *pos, int m, char *str)
{
  Move move = pos->move[pos->state[pos->idx].firstMove + m];

  int i = move & 7, j = move >> 8;
  int from = pos->sq[i];
  int to = (move & CAPT_FLAG) ? pos->sq[j] : j;
  if ((pos->pt[i] & 7) == PAWN) {
    if (move & CAPT_FLAG)
      *str++ = 'a' + (from & 7);
  } else {
    *str++ = PieceChar[pos->pt[i] & 7];
    bool sameTo = false, sameFile = false, sameRank = false;
    int num = pos->state[pos->idx + 1].firstMove - pos->state[pos->idx].firstMove;
    for (int m1 = 0; m1 < num; m1++) {
      int move1 = pos->move[pos->state[pos->idx].firstMove + m1];
      if (   (move1 & 7) != i
          && (move1 >> 8) == j
          && pos->pt[move1 & 7] == pos->pt[i]
          && TB_do_move(pos, m1))
      {
        TB_undo_move(pos, m1);
        sameTo = true;
        int from1 = pos->sq[move1 & 7];
        if ((from1 & 7) == (from & 7))
          sameFile = true;
        if ((from1 >> 3) == (from >> 3))
          sameRank = true;
      }
    }
    if (sameTo) {
      if (!sameFile || sameRank)
        *str++ = 'a' + (from & 7);
      if (sameFile)
        *str++ = '1' + (from >> 3);
    }
  }
  if (move & CAPT_FLAG)
    *str++ = 'x';
  *str++ = 'a' + (to & 7);
  *str++ = '1' + (to >> 3);
  if (move & PROM_FLAG)
    *str++ = 'Q';
  if (TB_do_move(pos, m)) {
    if (TB_in_check(pos))
      *str++ = TB_no_legal_moves(pos) ? '#' : '+';
    TB_undo_move(pos, m);
  }
  *str = 0;
}

// Convert the mth move to uci notation
void TBitf_move_to_string_uci(TB_Position *pos, int m, char *str)
{
  Move move = pos->move[pos->state[pos->idx].firstMove + m];

  int i = move & 7, j = move >> 8;
  int from = pos->sq[i];
  int to = (move & CAPT_FLAG) ? pos->sq[j] : j;
  *str++ = 'a' + (from & 7);
  *str++ = '1' + (from >> 3);
  *str++ = 'a' + (to & 7);
  *str++ = '1' + (to >> 3);
  if (move & PROM_FLAG)
    *str++ = 'q';
  *str = 0;
}

void TBitf_print_pos(TB_Position *pos)
{
  int board[64];

  for (int i = 0; i < 64; i++)
    board[i] = 0;
  for (int i = 0; i < pos->num; i++)
    board[pos->sq[i]] = pos->pt[i];

  printf("\n +---+---+---+---+---+---+---+---+\n");

  for (int r = 7; r >= 0; r--) {
    for (int f = 0; f <= 7; f++)
      printf(" | %c", PieceChar[board[8 * r + f]]);

    printf(" | %d\n +---+---+---+---+---+---+---+---+\n", r + 1);
  }

  printf("   a   b   c   d   e   f   g   h\n\n");
}
