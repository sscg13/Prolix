using U64 = uint64_t;
struct TTentry {
  U64 key;
  U64 data;
  void update(U64 hash, int gamelength, int depth, int score, int nodetype, int hashmove);
  int age(int gamelength);
  int hashmove();
  int depth();
  int gamelength();
  int score();
  int nodetype();
};
void TTentry::update(U64 hash, int gamelength, int depth, int score, int nodetype, int hashmove) {
  key = hash;
  data = (U64)((unsigned short int)score);
  data |= (((U64)hashmove) << 16);
  data |= (((U64)nodetype) << 42);
  data |= (((U64)gamelength) << 44);
  data |= (((U64)depth) << 54);
}
int TTentry::age(int gamelength) {
  return std::max((gamelength - ((int)(data >> 44) & 1023)), 0);
}
int TTentry::hashmove() {
  return (int)(data >> 16) & 0x03FFFFFF;
}
int TTentry::depth() {
  return (int)(data >> 54) & 63;
}
int TTentry::score() {
  return (int)(short int)(data & 0x000000000000FFFF);
}
int TTentry::nodetype() {
  return (int)(data >> 42) & 3;
}