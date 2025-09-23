#include "board.h"
#include "consts.h"
#include "eval/nnue.h"
#include "external/Fathom/tbprobe.h"
#include "history.h"
#include "search.h"
#include "tt.h"
#include <chrono>
#include <time.h>
#pragma once
extern std::string proto;
const int maxtbpieces = 5;
extern int lmr_reductions[maxmaxdepth][maxmoves];
extern std::chrono::time_point<std::chrono::steady_clock> start;
extern std::string inputfile;
class Engine {
  int TTsize = 2097152;
  std::vector<TTentry> TT;
  Board Bitboards;
  bool useNNUE = true;
  bool normalizeeval = true;
  bool showWDL = true;
  bool gosent = false;
  std::atomic<bool> stopsearch = ATOMIC_VAR_INIT(false);
  bool useTB = false;
  abinfo searchstack[maxmaxdepth + 32];
  int pvtable[maxmaxdepth + 1][maxmaxdepth + 1];
  int bestmove = 0;
  Limits searchlimits;
  std::random_device rd;
  std::mt19937 mt;
  std::ofstream dataoutput;
  Searcher master;
  void initializett();

public:
  void startup();
  void bench();
  void datagen(int dataformat, int threads, int n, std::string outputfile);
  void bookgen(int lowerbound, int upperbound, int threads, int n,
               std::string outputfile);
  void filter(int lowerbound, int upperbound, int softnodes, int hardnodes,
              int threads, std::string inputfile, std::string outputfile);
  void uci();
  void xboard();
};

bool iscapture(int notation);
void initializelmr();