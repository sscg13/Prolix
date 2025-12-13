#include "board.h"
#include "consts.h"
#include "eval/nnue.h"
#include "history.h"
#include "search.h"
#include "tt.h"
#include <chrono>
#include <thread>
#include <time.h>
#pragma once
extern std::string proto;
extern int quiet_reductions[maxmaxdepth][maxmoves];
extern std::chrono::time_point<std::chrono::steady_clock> start;
extern std::string inputfile;
class Engine {
  int TTsize = 2097152;
  std::vector<TTentry> TT;
  Board Bitboards;
  NNUEWeights *nnueweights = new NNUEWeights;
  bool gosent = false;
  std::atomic<bool> stopsearch = ATOMIC_VAR_INIT(false);
  abinfo searchstack[maxmaxdepth + 32];
  int pvtable[maxmaxdepth + 1][maxmaxdepth + 1];
  int bestmove = 0;
  Limits searchlimits;
  Options searchoptions;
  std::random_device rd;
  std::mt19937 mt;
  std::ofstream dataoutput;
  int threads = 1;
  Searcher master;
  void initializett();

public:
  friend class Searcher;
  void startup();
  void bench();
  void spawnworker();
  void evalscale(std::string inputfile);
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