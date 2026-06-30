#include "oranjformat.h"

PackedBoard::PackedBoard(const Board &Bitboards, int eval)
    : occupancy(Bitboards.Bitboards[0] | Bitboards.Bitboards[1]), pieces{},
      stm((Bitboards.position & 1) << 7), halfmove(Bitboards.position >> 1),
      fullmove(0), score((I16)eval), wdl(0), padding(0) {
  U64 occupied = occupancy;
  int i = 0;
  while (occupied) {
    int square = __builtin_ctzll(occupied);
    pieces[i / 2] |= (((U8)Bitboards.pieces[square]) << (4 * (i & 1)));
    occupied &= (occupied - 1);
    i++;
  }
}

MoveScorePair::MoveScorePair(int mov, int eval)
    : move(mov & 4095), score((I16)eval) {
  if (mov & (1 << 20)) {
    move |= 0x1000;
  }
}

void Oranjformat::initialize(const Board &Bitboards, int eval) {
  initialpos = PackedBoard(Bitboards, eval);
  moves.clear();
}

void Oranjformat::push(int move, int score) { moves.emplace_back(move, score); }

void Oranjformat::writewithwdl(std::ofstream &output, int wdl) {
  initialpos.wdl = wdl;
  moves.emplace_back(0, 0);
  output.write(reinterpret_cast<const char *>(&initialpos), 32);
  output.write(reinterpret_cast<const char *>(moves.data()), 4 * moves.size());
}

int Oranjformat::positioncount() { return moves.size() - 1; }