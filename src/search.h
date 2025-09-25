#include "board.h"
#include "consts.h"
#include "eval/nnue.h"
#include "external/Fathom/tbprobe.h"
#include "history.h"
#include "tt.h"
#include <atomic>
#include <chrono>
#include <time.h>
#pragma once
class Engine;
struct abinfo {
  int playedmove;
  int eval;
};
struct Limits {
  int softnodelimit;
  int hardnodelimit;
  int softtimelimit;
  int hardtimelimit;
  int maxdepth;
};
struct Options {
  bool useNNUE = true;
  bool normalizeeval = true;
  bool showWDL = true;
  bool suppressoutput = false;
  bool useTB = false;
};
class Searcher {
  NNUE EUNN;
  History *Histories = new History;
  std::vector<TTentry> *TT;
  int *TTsize;
  int *signal;
  int killers[maxmaxdepth][2];
  int countermoves[6][64];
  std::atomic<bool> *stopsearch;
  bool rootinTB = false;
  abinfo searchstack[maxmaxdepth + 32];
  int pvtable[maxmaxdepth + 1][maxmaxdepth + 1];
  int bestmove = 0;
  std::random_device rd;
  std::mt19937 mt;
  void resetauxdata();
  int quiesce(int alpha, int beta, int depth, bool isPV);
  int alphabeta(int depth, int ply, int alpha, int beta, bool nmp,
                int nodetype);
  int wdlmodel(int eval);
  int normalize(int eval);

public:
  Board Bitboards;
  Limits searchlimits;
  Options searchoptions;
  std::ofstream dataoutput;
  bool ismaster = true;
  void seedrng();
  void syncwith(Engine &engine);
  int iterative();
  void datagenautoplayplain();
  void datagenautoplayviriformat();
  void bookgenautoplay(int lowerbound, int upperbound);
};