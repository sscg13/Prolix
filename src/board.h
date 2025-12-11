#include "consts.h"
#include "zobrist.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#pragma once

struct TB_Position;

class Board {
  int evalm[2] = {0, 0};
  int evale[2] = {0, 0};
  int mobilitym[2] = {0, 0};
  int mobilitye[2] = {0, 0};
  U64 zobrist[256];
  int history[256];
  int root = 0;
  Zobrist hashes;
  TB_Position *tbpos = nullptr;
  int startpiece[16] = {4, 3, 1, 5, 2, 1, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0};

public:
  U64 Bitboards[8];
  int pieces[64];
  int gamelength = 0;
  int position = 0;
  int nodecount = 0;
  int gamephase[2] = {0, 0};
  U64 zobristhash = 0ULL;
  void get_tbpos_pointer();
  void free_tbpos_pointer();
  U64 scratchzobrist();
  void initialize();
  int repetitions();
  int halfmovecount();
  bool twokings();
  bool bareking(int color);
  int material();
  U64 checkers(int color);
  void makenullmove();
  void unmakenullmove();
  U64 keyafter(int notation);
  void makemove(int notation, bool reversible);
  void unmakemove(int notation);
  int generatemoves(int color, bool capturesonly, int *movelist);
  U64 perft(int depth, int initialdepth, int color);
  U64 perftnobulk(int depth, int initialdepth, int color);
  void parseFEN(std::string FEN);
  std::string getFEN();
  int evaluate(int color);
  bool see_exceeds(int mov, int color, int threshold);
  int probetbwdl();
};

void initializeleaperattacks();
void initializemasks();
void initializerankattacks();
void initializezobrist();
std::string algebraic(int notation);
std::string get8294400FEN(int seed1, int seed2);
std::string get129600FEN(int seed1, int seed2);