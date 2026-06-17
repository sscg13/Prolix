#include "../board.h"
#pragma once
struct [[gnu::packed]] PackedBoard {
  U64 occupancy;
  U8 pieces[16];
  U8 stm;
  U8 halfmove;
  U16 fullmove;
  U16 score;
  U8 wdl;
  U8 padding;
  PackedBoard() = default;
  PackedBoard(const Board &Bitboards, int eval);
};

struct [[gnu::packed]] MoveScorePair {
  U16 move;
  U16 score;
  MoveScorePair() = default;
  MoveScorePair(int mov, int eval);
};

class Oranjformat {
  PackedBoard initialpos;
  std::vector<MoveScorePair> moves;

public:
  void initialize(const Board &Bitboards, int eval);
  void push(int move, int score);
  void writewithwdl(std::ofstream &output, int wdl);
  int positioncount(); // meant to be called after writewithwdl
};
