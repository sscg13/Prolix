#ifndef JTBINTERFACE_H
#define JTBINTERFACE_H

#include <stdbool.h>

#include "jtbprobe.h"

#ifdef __cplusplus
extern "C" {
#endif

// Board and move representation

struct State {
  int firstMove;
  uint8_t fromSquare;
};

typedef uint16_t Move;

struct TB_Position {
  uint64_t occ;
  uint8_t sq[8];
  uint8_t pt[8];
  int num;
  int cnt[2];
  int stm;
  int idx;
  int maxIdx;
  int maxMoves;
  struct State *state;
  Move *move;
};

typedef struct TB_Position TB_Position;

// Initialize data structures
void TBitf_init(void);

// Allocate a TB_Position instance
TB_Position *TBitf_alloc_position(void);

// Free a TB_Position instance
void TBitf_free_position(TB_Position *pos);

// Initialize a TB_Position instance from a fen
void TBitf_set_from_fen(TB_Position *pos, const char *fen, int *cnt70);

// Convert a move to algebraic notation
void TBitf_move_to_string(TB_Position *pos, int m, char *str);

// Convert a move to uci notation
void TBitf_move_to_string_uci(TB_Position *pos, int m, char *str);

// Print an ASCII representation of the position to stdout
void TBitf_print_pos(TB_Position *pos);

#ifdef __cplusplus
}
#endif

#endif
