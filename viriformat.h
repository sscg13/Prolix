#include "board.h"
#pragma once
struct Marlinboard {
  U64 occupancy;
  U64 pieces[2];
  U64 data;
  // 8 bit epsquare
  // 8 bit halfmoveclock
  // 16 bit fullmove
  // 16 bit eval (signed)
  // 8 bit wdl
  // 8 bit extra
  void pack(const Board &Bitboards, int score);
};
class Viriformat {
  Marlinboard initialpos;
  std::vector<unsigned int> moves;

public:
  void initialize(const Board &Bitboards);
  void push(int move, int score);
  void writewithwdl(std::ofstream &output, int wdl);
};
