#ifndef JTBPROBE_H
#define JTBPROBE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { TB_WDL, TB_DTM, TB_DTZ };

// Number of detected WDL, DTM and DTZ tables
extern int TB_NumTables[3];

// No need to try to probe if the number of pieces > MaxCardinality
extern int TB_MaxCardinality[3];

// Forward declaration of the TB_Position struct. This struct is not
// declared here but by the code interfacing with the probing code.
struct TB_Position;
typedef struct TB_Position TB_Position;

// Functions implemented by the probing code
void TB_init(char *path);
void TB_free(void);
void TB_release(void);
int TB_probe_wdl(TB_Position *pos, int *success);
int TB_probe_dtz(TB_Position *pos, int *success);
int TB_probe_dtm(TB_Position *pos, int wdl, int *success);

// Functions required by jtbprobe.c

// Return a 64-bit key identifying the combination of material on the board.
uint64_t TB_material_key(TB_Position *pos);

// Return the same key but generated from a list of piece counts.
uint64_t TB_material_key_from_counts(int whiteCounts[8], int blackCounts[8]);

// Produce a text string of the form KQPvKRP, where "KQP" represents the
// white pieces and "KRP" represents the black pieces.
void TB_material_string(TB_Position *pos, char str[16]);

// List the squares (A1=1, H1=8, H8=63) of the pieces on the board in the
// output array p[] in the order indicated by the input array of piece
// types pt[] (WPAWN = 1, BKING = 14). Identical piece types are always
// listed in pt[] consecutively.
// If 'flip' is true, then the function must flip both the color of the
// piece types read from pt[] (^0x08) and the squares stored in p[] (^0x38).
void TB_list_squares(TB_Position *pos, const uint8_t *pt, bool flip, int *p);

// Generate a set of moves which includes all legal captures and store
// them inside or in association with the TbPosition data structure.
// Return the number M of generated moves.
int TB_generate_captures(TB_Position *pos);

// Same for quiet moves.
int TB_generate_quiets(TB_Position *pos, int start);

// Test whether the mth generated move (0 <= m < M) is legal.
bool TB_move_is_legal(TB_Position *pos, int m);

// Test whether the mth move is a legal capture/quiet and do the
// move if it is.
bool TB_do_capture(TB_Position *pos, int m);
bool TB_do_move(TB_Position *pos, int m);

// Undo the last move.
void TB_undo_move(TB_Position *pos, int m);

// Test whether it is white's turn to make a move.
bool TB_white_to_move(TB_Position *pos);

// Test whether the side to move has a bare king and determine the outcome.
bool TB_bare_king(TB_Position *pos, int *v);

// Test whether the side to move is in check.
bool TB_in_check(TB_Position *pos);

// Test whether the mth move is a pawn move.
bool TB_move_is_pawn_move(TB_Position *pos, int m);

// Test whether the position has any legal moves.
bool TB_no_legal_moves(TB_Position *pos);

#ifdef __cplusplus
}
#endif

#endif
