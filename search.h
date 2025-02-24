#include "board.h"
#include "external/Fathom/tbprobe.h"
#include "nnue.h"
#include "history.h"
#include "tt.h"
#include <chrono>
#include <time.h>
#pragma once
extern std::string proto;
const int maxmaxdepth = 32;
const int maxtbpieces = 5;
extern int lmr_reductions[maxmaxdepth][256];
extern std::chrono::time_point<std::chrono::steady_clock> start;
extern std::string inputfile;
struct abinfo {
  int playedmove;
  int eval;
};
class Engine {
  Board Bitboards;
  int TTsize = 2097152;
  std::vector<TTentry> TT;
  bool useNNUE = true;
  bool showWDL = true;
  NNUE EUNN;
  History Histories;
  int killers[32][2];
  int countermoves[6][64];
  bool gosent = false;
  bool stopsearch = false;
  bool suppressoutput = false;
  bool rootinTB = false;
  bool useTB = false;
  int maxdepth = 32;
  abinfo searchstack[64];
  int pvtable[maxmaxdepth + 1][maxmaxdepth + 1];
  int bestmove = 0;
  int movetime = 0;
  int softnodelimit = 0;
  int hardnodelimit = 0;
  int softtimelimit = 0;
  int hardtimelimit = 0;
  std::random_device rd;
  std::mt19937 mt;
  std::ofstream dataoutput;
  void initializett();
  void resetauxdata();
  int quiesce(int alpha, int beta, int color, int depth);
  int alphabeta(int depth, int ply, int alpha, int beta, int color, bool nmp);
  int wdlmodel(int eval);
  int normalize(int eval);
  int iterative(int color);
  void datagenautoplay();
  void bookgenautoplay(int lowerbound, int upperbound);

public:
  void startup();
  void bench();
  void datagen(int n, std::string outputfile);
  void bookgen(int lowerbound, int upperbound, int n, std::string outputfile);
  void uci();
  void xboard();
};

bool iscapture(int notation);
void initializelmr();