#pragma once
#include <cstdint>
using U64 = uint64_t;
struct TTentry {
  U64 key;
  U64 data;
  void update(U64 hash, int gamelength, int depth, int ply, bool ttpv,
              int score, int staticeval, int nodetype, int hashmove);
  int age(int gamelength);
  int hashmove();
  int depth();
  int score(int ply);
  int staticeval();
  int nodetype();
  bool isttPV();
};