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
  int movetime;
};
struct Options {
  bool useNNUE = true;
  bool normalizeeval = true;
  bool showWDL = true;
  bool suppressoutput = false;
  bool useTB = false;
};
class Searcher {
  NNUE *EUNN = new NNUE;
  History *Histories = new History;
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
  int quiesce(int alpha, int beta, int color, int depth, bool isPV);
  int alphabeta(int depth, int ply, int alpha, int beta, int color, bool nmp,
                int nodetype);
  int wdlmodel(int eval);
  int normalize(int eval);

public:
  std::vector<TTentry> *TT;
  int *TTsize;
  Board Bitboards;
  Limits searchlimits;
  Options searchoptions;
  std::ofstream dataoutput;
  bool ismaster = true;
  void seedrng();
  void setstopsearch(std::atomic<bool> &stopsearchref);
  void setTT(std::vector<TTentry> &TTref, int &size);
  void loadposition(Board board);
  void loadsearchlimits(Limits limits);
  int iterative(int color);
  void datagenautoplayplain();
  void datagenautoplayviriformat();
  void bookgenautoplay(int lowerbound, int upperbound);
};