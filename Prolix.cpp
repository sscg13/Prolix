#include "board.cpp"
#include "history.cpp"
#include "nnue.cpp"
#include "tt.cpp"
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <time.h>
std::string proto = "uci";
//clang-format off
std::string uciinfostring =
    "id name Prolix \nid author sscg13 \n"
    "option name UCI_Variant type combo default shatranj var shatranj \n"
    "option name Threads type spin default 1 min 1 max 1 \n"
    "option name Hash type spin default 32 min 1 max 1024 \n"
    "option name Use NNUE type check default true \n"
    "option name EvalFile type string default <internal> \n"
    "option name UCI_ShowWDL type check default true \n"
    "option name SyzygyPath type string default <empty> \nuciok\n";
//clang-format on
const int maxmaxdepth = 32;
const int maxtbpieces = 5;
int lmr_reductions[maxmaxdepth][256];
auto start = std::chrono::steady_clock::now();
std::ifstream datainput;
std::string inputfile;
struct abinfo {
  int playedmove;
  int eval;
  int excludedmove;
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
void Engine::initializett() {
  TT.resize(TTsize);
  for (int i = 0; i < TTsize; i++) {
    TT[i].key = (U64)i + 1ULL;
    TT[i].data = 0;
  }
}
void Engine::resetauxdata() {
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 64; j++) {
      countermoves[i][j] = 0;
    }
  }
  for (int i = 0; i < 32; i++) {
    killers[i][0] = 0;
    killers[i][1] = 0;
  }
  for (int i = 0; i < maxmaxdepth; i++) {
    for (int j = 0; j < maxmaxdepth + 1; j++) {
      pvtable[i][j] = 0;
    }
  }
  Histories.reset();
}
void Engine::startup() {
  initializett();
  resetauxdata();
  Bitboards.initialize();
  EUNN.loaddefaultnet();
  EUNN.initializennue(Bitboards.Bitboards);
  mt.seed(rd());
}
void initializelmr() {
  for (int i = 0; i < maxmaxdepth; i++) {
    for (int j = 0; j < 256; j++) {
      lmr_reductions[i][j] =
          (i == 0 || j == 0) ? 0 : floor(0.77 + log(i) * log(j) * 0.46);
    }
  }
}
int Engine::quiesce(int alpha, int beta, int color, int depth) {
  int score = useNNUE ? EUNN.evaluate(color) : Bitboards.evaluate(color);
  int bestscore = -30000;
  int movcount;
  if (depth > 3) {
    return score;
  }
  bool incheck = Bitboards.checkers(color);
  if (incheck) {
    movcount = Bitboards.generatemoves(color, 0, maxdepth + depth);
    if (movcount == 0) {
      return -27000;
    }
  } else {
    bestscore = score;
    if (alpha < score) {
      alpha = score;
    }
    if (score >= beta) {
      return score;
    }
    movcount = Bitboards.generatemoves(color, 1, maxdepth + depth);
  }

  if (depth < 1) {
    for (int i = 0; i < movcount - 1; i++) {
      for (int j = i + 1;
           Histories.movescore(Bitboards.moves[maxdepth + depth][j]) >
               Histories.movescore(Bitboards.moves[maxdepth + depth][j - 1]) &&
           j > 0;
           j--) {
        std::swap(Bitboards.moves[maxdepth + depth][j],
                  Bitboards.moves[maxdepth + depth][j - 1]);
      }
    }
  }
  for (int i = 0; i < movcount; i++) {
    int mov = Bitboards.moves[maxdepth + depth][i];
    bool good = (incheck || Bitboards.see_exceeds(mov, color, 0));
    if (good) {
      Bitboards.makemove(mov, 1);
      if (useNNUE) {
        EUNN.forwardaccumulators(mov);
      }
      score = -quiesce(-beta, -alpha, color ^ 1, depth + 1);
      Bitboards.unmakemove(mov);
      if (useNNUE) {
        EUNN.backwardaccumulators(mov);
      }
      if (score >= beta) {
        return score;
      }
      if (score > alpha) {
        alpha = score;
      }
      if (score > bestscore) {
        bestscore = score;
      }
    }
  }
  return bestscore;
}
int Engine::alphabeta(int depth, int ply, int alpha, int beta, int color,
                      bool nmp) {
  pvtable[ply][0] = ply + 1;
  if (Bitboards.repetitions() > 1) {
    return 0;
  }
  if (Bitboards.halfmovecount() >= 140) {
    return 0;
  }
  if (Bitboards.twokings()) {
    return 0;
  }
  if (Bitboards.bareking(color ^ 1)) {
    return (28001 - ply);
  }
  if (depth <= 0 || ply >= maxdepth) {
    return quiesce(alpha, beta, color, 0);
  }
  int tbwdl = 0;
  bool fewpieces =
      (__builtin_popcountll(Bitboards.Bitboards[0] | Bitboards.Bitboards[1]) <=
       maxtbpieces);
  if (fewpieces && useTB) {
    tbwdl = Bitboards.probetbwdl();
    if (!rootinTB) {
      return tbwdl * (26000 - ply);
    }
  }
  int score = -30000;
  int bestscore = -30000;
  int allnode = 0;
  int movcount;
  int index = Bitboards.zobristhash % TTsize;
  int ttmove = 0;
  int bestmove1 = -1;
  int ttdepth = TT[index].depth();
  int ttage = TT[index].age(Bitboards.gamelength);
  int nodetype = 0;
  bool update = (depth >= (ttdepth - ttage / 3));
  bool incheck = (Bitboards.checkers(color) != 0ULL);
  bool isPV = (beta - alpha > 1);
  int staticeval = useNNUE ? EUNN.evaluate(color) : Bitboards.evaluate(color);
  int excluded = searchstack[ply].excludedmove;
  searchstack[ply].eval = staticeval;
  bool improving = false;
  if (ply > 1) {
    improving = (staticeval > searchstack[ply - 2].eval);
  }
  int quiets = 0;
  if (TT[index].key == Bitboards.zobristhash && excluded == 0) {
    score = TT[index].score();
    ttmove = TT[index].hashmove();
    nodetype = TT[index].nodetype();
    if (ttdepth >= depth) {
      if (!isPV && Bitboards.repetitions() == 0) {
        if (nodetype == 3) {
          return score;
        }
        if ((nodetype & 1) && (score >= beta)) {
          return score;
        }
        if ((nodetype & 2) && (score <= alpha)) {
          return score;
        }
      }
    } else {
      int margin = std::max(40, 70 * (depth - ttdepth - improving));
      if (((nodetype & 1) && (score - margin >= beta)) &&
          (abs(beta) < 20000 && !incheck) && (ply > 0)) {
        return (score + beta) / 2;
      }
    }
  }
  int margin = std::max(40, 70 * (depth - improving));
  if (ply > 0 && score == -30000 && excluded == 0) {
    if (staticeval - margin >= beta && (abs(beta) < 20000 && !incheck)) {
      return (staticeval + beta) / 2;
    }
  }
  movcount = Bitboards.generatemoves(color, 0, ply);
  if (movcount == 0) {
    return -1 * (28000 - ply);
  }
  if ((!incheck && Bitboards.gamephase[color] > 3) && (depth > 1 && nmp) &&
      (staticeval >= beta && !isPV) && excluded == 0) {
    Bitboards.makenullmove();
    searchstack[ply].playedmove = 0;
    score = -alphabeta(std::max(0, depth - 2 - (depth + 1) / 3), ply + 1, -beta,
                       1 - beta, color ^ 1, false);
    Bitboards.unmakenullmove();
    if (score >= beta) {
      return beta;
    }
  }
  /*if ((depth < 3) && (staticeval + 200*depth < alpha) && !isPV) {
      int qsearchscore = quiesce(alpha, beta, color, 0);
      if (qsearchscore <= alpha) {
          return alpha;
      }
  }*/
  int counter = 0;
  int previousmove = 0;
  int previouspiece = 0;
  int previoussquare = 0;
  if (ply > 0 && nmp) {
    previousmove = searchstack[ply - 1].playedmove;
    previouspiece = (previousmove >> 13) & 7;
    previoussquare = (previousmove >> 6) & 63;
    counter = countermoves[previouspiece - 2][previoussquare];
  }
  int movescore[256];
  for (int i = 0; i < movcount; i++) {
    int mov = Bitboards.moves[ply][i];
    if (mov == ttmove) {
      movescore[i] = (1 << 20);
    } else {
      movescore[i] = Histories.movescore(mov);
    }
    if (mov == killers[ply][0]) {
      movescore[i] += 20000;
    }
    /*else if (moves[ply][i] == killers[ply][1]) {
      movescore[ply][i] += 10000;
    }*/
    /*else if ((mov & 4095) == counter) {
      movescore[i] += 10000;
    }*/
    /*if (see_exceeds(moves[ply][i], color, 0)) {
        movescore[ply][i]+=15000;
    }*/
    int j = i;
    while (j > 0 && movescore[j] > movescore[j - 1]) {
      std::swap(Bitboards.moves[ply][j], Bitboards.moves[ply][j - 1]);
      std::swap(movescore[j], movescore[j - 1]);
      j--;
    }
  }
  for (int i = 0; i < movcount; i++) {
    bool nullwindow = (i > 0);
    int r = 0;
    bool prune = false;
    int mov = Bitboards.moves[ply][i];
    if (mov == excluded) {
      continue;
    }
    if (!iscapture(mov)) {
      quiets++;
      /*if (quiets > 7*depth) {
        prune = true;
      }*/
      r = std::min(depth - 1, lmr_reductions[depth][quiets]);
    }
    r = std::max(0, r - isPV - improving);
    if (fewpieces && useTB) {
      Bitboards.makemove(mov, true);
      if (Bitboards.probetbwdl() != -tbwdl) {
        prune = true;
      }
      Bitboards.unmakemove(mov);
    }
    int e = 0;
    if ((ply > 0 && depth >= 8) && (mov == ttmove && excluded == 0) && (ttdepth >= depth - 4) && (nodetype & 1)) {
      int singularbeta = TT[index].score() - 2 * depth;
      int singulardepth = (depth - 1) / 2;
      searchstack[ply].excludedmove = mov;
      score = alphabeta(singulardepth, ply, singularbeta - 1, singularbeta, color, nmp);
      searchstack[ply].excludedmove = 0;
      if (score < singularbeta) {
        e = 1;
      }
    }
    if (!stopsearch && !prune) {
      Bitboards.makemove(mov, true);
      searchstack[ply].playedmove = mov;
      int newdepth = depth - 1 + e;
      if (useNNUE) {
        EUNN.forwardaccumulators(mov);
      }
      if (nullwindow) {
        score = -alphabeta(newdepth - r, ply + 1, -alpha - 1, -alpha,
                           color ^ 1, true);
        if (score > alpha && r > 0) {
          score = -alphabeta(newdepth, ply + 1, -alpha - 1, -alpha, color ^ 1,
                             true);
        }
        if (score > alpha && score < beta) {
          score =
              -alphabeta(newdepth, ply + 1, -beta, -alpha, color ^ 1, true);
        }
      } else {
        score = -alphabeta(newdepth, ply + 1, -beta, -alpha, color ^ 1, true);
      }
      Bitboards.unmakemove(mov);
      if (useNNUE) {
        EUNN.backwardaccumulators(mov);
      }
      if (score > bestscore) {
        if (score > alpha) {
          if (score >= beta) {
            if (update && !stopsearch && excluded == 0) {
              TT[index].update(Bitboards.zobristhash, Bitboards.gamelength,
                               depth, score, 1, mov);
            }
            if (!iscapture(mov) && (killers[ply][0] != mov)) {
              killers[ply][1] = killers[ply][0];
              killers[ply][0] = mov;
            }
            if (iscapture(mov)) {
              Histories.updatenoisyhistory(mov, depth * depth * depth);
            } else {
              Histories.updatequiethistory(mov, depth * depth);
            }
            for (int j = 0; j < i; j++) {
              int mov2 = Bitboards.moves[ply][j];
              if (iscapture(mov2)) {
                Histories.updatenoisyhistory(mov2, -3 * depth);
              } else {
                Histories.updatequiethistory(mov2, -3 * depth);
              }
            }
            if (ply > 0 && nmp && !iscapture(mov)) {
              countermoves[previouspiece - 2][previoussquare] = (mov & 4095);
            }
            return score;
          }
          alpha = score;
          allnode = 1;
        }
        if (ply == 0) {
          bestmove = mov;
        }
        pvtable[ply][ply + 1] = mov;
        pvtable[ply][0] = pvtable[ply + 1][0] ? pvtable[ply + 1][0] : ply + 2;
        for (int j = ply + 2; j < pvtable[ply][0]; j++) {
          pvtable[ply][j] = pvtable[ply + 1][j];
        }
        bestmove1 = i;
        bestscore = score;
      }
      if (Bitboards.nodecount > hardnodelimit && hardnodelimit > 0) {
        stopsearch = true;
      }
      if ((Bitboards.nodecount & 1023) == 0) {
        auto now = std::chrono::steady_clock::now();
        auto timetaken =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (timetaken.count() > hardtimelimit && hardtimelimit > 0) {
          stopsearch = true;
        }
      }
    }
  }
  if (((update || allnode) && !stopsearch && excluded == 0)) {
    TT[index].update(Bitboards.zobristhash, Bitboards.gamelength, depth,
                     bestscore, 2 + allnode, Bitboards.moves[ply][bestmove1]);
  }
  return bestscore;
}
int Engine::wdlmodel(int eval) {
  int material = Bitboards.material();
  double m = std::max(std::min(material, 64), 4) / 32.0;
  double as[4] = {12.86611189, -1.56947052, -105.75177291, 247.30758159};
  double bs[4] = {-7.31901285, 36.79299424, -14.98330140, 64.14426025};
  double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
  double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];
  return int(0.5 + 1000 / (1 + exp((a - double(eval)) / b)));
}
int Engine::normalize(int eval) {
  if (abs(eval) >= 25000) {
    return eval;
  }
  int material = Bitboards.material();
  double m = std::max(std::min(material, 64), 4) / 32.0;
  double as[4] = {12.86611189, -1.56947052, -105.75177291, 247.30758159};
  double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
  return round(100 * eval / a);
}
int Engine::iterative(int color) {
  Bitboards.nodecount = 0;
  stopsearch = false;
  start = std::chrono::steady_clock::now();
  int score = useNNUE ? EUNN.evaluate(color) : Bitboards.evaluate(color);
  int returnedscore = score;
  int depth = 1;
  int bestmove1 = 0;
  int tbscore = 0;
  resetauxdata();
  rootinTB = (__builtin_popcountll(Bitboards.Bitboards[0] |
                                   Bitboards.Bitboards[1]) <= maxtbpieces);
  if (rootinTB && useTB) {
    tbscore = Bitboards.probetbwdl();
  }
  while (!stopsearch) {
    bestmove = -1;
    int delta = 30;
    int alpha = returnedscore - delta;
    int beta = returnedscore + delta;
    bool fail = true;
    while (fail) {
      int score1 = alphabeta(depth, 0, alpha, beta, color, false);
      if (score1 >= beta) {
        beta += delta;
        delta += delta;
      } else if (score1 <= alpha) {
        alpha -= delta;
        delta += delta;
      } else {
        score = score1;
        fail = false;
      }
    }
    auto now = std::chrono::steady_clock::now();
    auto timetaken =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    if ((Bitboards.nodecount < hardnodelimit || hardnodelimit <= 0) &&
        (timetaken.count() < hardtimelimit || hardtimelimit <= 0) &&
        depth < maxdepth && bestmove >= 0) {
      returnedscore = score;
      if (rootinTB && useTB && abs(score) < 27000) {
        score = 26000 * tbscore;
      }
      if (proto == "uci" && !suppressoutput) {
        if (abs(score) <= 27000) {
          int normalscore = normalize(score);
          std::cout << "info depth " << depth << " nodes "
                    << Bitboards.nodecount << " time " << timetaken.count()
                    << " score cp " << normalscore;
          if (showWDL) {
            int winrate = wdlmodel(score);
            int lossrate = wdlmodel(-score);
            int drawrate = 1000 - winrate - lossrate;
            std::cout << " wdl " << winrate << " " << drawrate << " "
                      << lossrate;
          }
          std::cout << " pv ";
          for (int i = 1; i < pvtable[0][0]; i++) {
            std::cout << algebraic(pvtable[0][i]) << " ";
          }
          std::cout << std::endl;
        } else {
          int matescore;
          if (score > 0) {
            matescore = 1 + (28000 - score) / 2;
          } else {
            matescore = (-28000 - score) / 2;
          }
          std::cout << "info depth " << depth << " nodes "
                    << Bitboards.nodecount << " time " << timetaken.count()
                    << " score mate " << matescore;
          if (showWDL) {
            int winrate = 1000 * (matescore > 0);
            int lossrate = 1000 * (matescore < 0);
            std::cout << " wdl " << winrate << " 0 " << lossrate;
          }
          std::cout << " pv ";
          for (int i = 1; i < pvtable[0][0]; i++) {
            std::cout << algebraic(pvtable[0][i]) << " ";
          }
          std::cout << std::endl;
        }
      }
      if (proto == "xboard") {
        std::cout << depth << " " << score << " " << timetaken.count() / 10
                  << " " << Bitboards.nodecount << " ";
        for (int i = 1; i < pvtable[0][0]; i++) {
          std::cout << algebraic(pvtable[0][i]) << " ";
        }
        std::cout << std::endl;
      }
      depth++;
      if (depth == maxdepth) {
        stopsearch = true;
      }
      bestmove1 = bestmove;
    } else {
      stopsearch = true;
    }
    if ((timetaken.count() > softtimelimit && softtimelimit > 0) ||
        (Bitboards.nodecount > softnodelimit && softnodelimit > 0)) {
      stopsearch = true;
    }
  }
  auto now = std::chrono::steady_clock::now();
  auto timetaken =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
  if (proto == "uci" && !suppressoutput) {
    int nps = 1000 * (Bitboards.nodecount / std::max(1LL, timetaken.count()));
    std::cout << "info nodes " << Bitboards.nodecount << " nps " << nps
              << std::endl;
  }
  if (proto == "uci" && !suppressoutput) {
    std::cout << "bestmove " << algebraic(bestmove1) << std::endl;
  }
  if (proto == "xboard") {
    std::cout << "move " << algebraic(bestmove1) << std::endl;
    Bitboards.makemove(bestmove1, 0);
    if (useNNUE) {
      EUNN.forwardaccumulators(bestmove1);
    }
  }
  bestmove = bestmove1;
  return returnedscore;
}
void Engine::datagenautoplay() {
  suppressoutput = true;
  initializett();
  resetauxdata();
  int seed = mt() % 360;
  Bitboards.parseFEN(get129600FEN(seed, seed));
  std::string game = "";
  std::string result = "";
  for (int i = 0; i < 8; i++) {
    int num_moves = Bitboards.generatemoves(i & 1, 0, 0);
    if (num_moves == 0) {
      suppressoutput = false;
      initializett();
      resetauxdata();
      seed = mt() % 360;
      Bitboards.parseFEN(get129600FEN(seed, seed));
      return;
    }
    int rand_move = mt() % num_moves;
    Bitboards.makemove(Bitboards.moves[0][rand_move], 0);
    game += algebraic(Bitboards.moves[0][rand_move]);
    game += " ";
  }
  if (useNNUE) {
    EUNN.initializennue(Bitboards.Bitboards);
  }
  if (Bitboards.generatemoves(0, 0, 0) == 0) {
    suppressoutput = false;
    initializett();
    resetauxdata();
    Bitboards.initialize();
    return;
  }
  std::string fens[1024];
  int scores[1024];
  int maxmove = 0;
  bool finished = false;
  while (!finished) {
    int color = Bitboards.position & 1;
    int score = iterative(color);
    if ((bestmove > 0) && (((bestmove >> 16) & 1) == 0) &&
        (Bitboards.checkers(color) == 0ULL) && (abs(score) < 27000)) {
      fens[maxmove] = Bitboards.getFEN();
      scores[maxmove] = score * (1 - 2 * color);
      maxmove++;
    }
    if (bestmove == 0) {
      std::cout << "Null best move? mitigating by using proper null move \n";
      Bitboards.makenullmove();
    } else {
      Bitboards.makemove(bestmove, 0);
    }
    if (Bitboards.twokings()) {
      finished = true;
      result = "0.5";
    } else if (Bitboards.bareking(color)) {
      finished = true;
      if (color == 0) {
        result = "0.0";
      } else {
        result = "1.0";
      }
    } else if (Bitboards.repetitions() >= 2) {
      finished = true;
      result = "0.5";
    } else if (Bitboards.generatemoves(color ^ 1, 0, 0) == 0) {
      finished = true;
      if (color == 0) {
        result = "1.0";
      } else {
        result = "0.0";
      }
    } else if (Bitboards.halfmovecount() >= 140) {
      finished = true;
      result = "0.5";
    }
    if (useNNUE && bestmove > 0) {
      EUNN.forwardaccumulators(bestmove);
    }
  }
  for (int i = 0; i < maxmove; i++) {
    dataoutput << fens[i] << " | " << scores[i] << " | " << result << "\n";
  }
  suppressoutput = false;
  initializett();
  resetauxdata();
  Bitboards.initialize();
}
void Engine::bookgenautoplay(int lowerbound, int upperbound) {
  suppressoutput = true;
  initializett();
  resetauxdata();
  int seed = mt() % 40320;
  Bitboards.parseFEN(get8294400FEN(seed, seed));
  for (int i = 0; i < 8; i++) {
    int num_moves = Bitboards.generatemoves(i & 1, 0, 0);
    if (num_moves == 0) {
      suppressoutput = false;
      initializett();
      resetauxdata();
      seed = mt() % 40320;
      Bitboards.parseFEN(get8294400FEN(seed, seed));
      return;
    }
    int rand_move = mt() % num_moves;
    Bitboards.makemove(Bitboards.moves[0][rand_move], 0);
  }
  if (useNNUE) {
    EUNN.initializennue(Bitboards.Bitboards);
  }
  if (Bitboards.generatemoves(0, 0, 0) == 0) {
    suppressoutput = false;
    initializett();
    resetauxdata();
    Bitboards.initialize();
    return;
  }
  bool finished = false;
  while (!finished) {
    int color = Bitboards.position & 1;
    softnodelimit = color ? 4096 : 16384;
    int score = iterative(color);
    if ((bestmove > 0) && (((bestmove >> 16) & 1) == 0) &&
        (Bitboards.checkers(color) == 0ULL) && (abs(score) <= upperbound) &&
        (abs(score) >= lowerbound)) {
      dataoutput << Bitboards.getFEN() << "\n";
      std::cout << "success\n";
      finished = true;
    }
    if (bestmove == 0) {
      std::cout << "Null best move? mitigating by using proper null move \n";
      Bitboards.makenullmove();
    } else {
      Bitboards.makemove(bestmove, 0);
      if (useNNUE) {
        EUNN.forwardaccumulators(bestmove);
      }
    }
    if (Bitboards.twokings()) {
      finished = true;
    } else if (Bitboards.bareking(color)) {
      finished = true;
    } else if (Bitboards.repetitions() >= 2) {
      finished = true;
    } else if (Bitboards.generatemoves(color ^ 1, 0, 0) == 0) {
      finished = true;
    } else if (Bitboards.material() < 32) {
      finished = true;
    }
  }
  suppressoutput = false;
  initializett();
  resetauxdata();
  Bitboards.initialize();
}
void Engine::datagen(int n, std::string outputfile) {
  dataoutput.open(outputfile, std::ofstream::app);
  softnodelimit = 16384;
  hardnodelimit = 65536;
  softtimelimit = 0;
  hardtimelimit = 0;
  for (int i = 0; i < n; i++) {
    datagenautoplay();
    std::cout << i << "\n";
  }
  dataoutput.close();
}
void Engine::bookgen(int lowerbound, int upperbound, int n,
                     std::string outputfile) {
  dataoutput.open(outputfile, std::ofstream::app);
  hardnodelimit = 65536;
  softtimelimit = 0;
  hardtimelimit = 0;
  for (int i = 0; i < n; i++) {
    bookgenautoplay(lowerbound, upperbound);
    std::cout << i << "\n";
  }
}
void Engine::bench() {
  std::string benchfens[14] = {
      "r5r1/1k6/1pqb4/1Bppn1p1/P1n1p2p/P1N1P2P/2KQ1p2/1RBR2N1 w - - 0 45",
      "8/1R6/4q3/3Nk1p1/2P3p1/3PK3/8/8 w - - 2 83",
      "8/8/8/1KQQQ3/2P3qP/5k2/7b/8 b - - 20 76",
      "2r1r3/p1pk1ppp/bpnpp2b/8/3P4/BPQ1PN1P/P1P1KPP1/R6R b - - 1 14",
      "3kq3/3p4/3p1p2/6pK/1R1Q4/1P1B1r2/8/8 w - - 2 44",
      "1nbkq3/1rpppr1p/3b1p2/p1PP1Pp1/1p6/PP1NP1PB/3Q3n/RNBKR3 w - - 0 20",
      "r4br1/8/p2k2qp/7n/1R1N4/3BB1P1/P2PPQ1P/3K4 w - - 3 32",
      "8/8/8/8/3Qk1n1/2K1P3/8/8 b - - 46 162",
      "rnbkqbnr/ppppp1p1/5p1p/8/8/3P2P1/PPP1PP1P/RNBKQBNR w - - 0 1",
      "5b1r/8/1p1pq1p1/p1k3P1/5RP1/P1PB4/4KQ2/8 w - - 1 44",
      "2r1qr2/8/1pkp2pb/p2pn1N1/3R2PP/3BP1Q1/P1P1R3/2K5 b - - 6 30",
      "1r2q3/R4pn1/1p1pkn2/3p1p2/1PpP2p1/N1P1K1P1/3Q3P/2B1R3 b - - 5 31",
      "8/1Q6/3Q4/3p1p2/2pkq2R/5q2/5K2/8 w - - 2 116",
      "8/4k3/4R3/2PK4/1P3Nn1/P2PPn2/5r2/8 b - - 2 58"};
  suppressoutput = true;
  maxdepth = 14;
  auto commence = std::chrono::steady_clock::now();
  int nodes = 0;
  softnodelimit = 0;
  hardnodelimit = 0;
  softtimelimit = 0;
  hardtimelimit = 0;
  for (int i = 0; i < 14; i++) {
    startup();
    Bitboards.parseFEN(benchfens[i]);
    EUNN.initializennue(Bitboards.Bitboards);
    int color = Bitboards.position & 1;
    iterative(color);
    nodes += Bitboards.nodecount;
  }
  auto conclude = std::chrono::steady_clock::now();
  int timetaken =
      std::chrono::duration_cast<std::chrono::milliseconds>(conclude - commence)
          .count();
  int nps = 1000 * (nodes / timetaken);
  std::cout << nodes << " nodes " << nps << " nps\n";
}
void Engine::uci() {
  std::string ucicommand;
  getline(std::cin, ucicommand);
  if (ucicommand == "uci") {
    std::cout << uciinfostring;
  }
  if (ucicommand == "quit") {
    exit(0);
  }
  if (ucicommand == "isready") {
    std::cout << "readyok" << std::endl;
  }
  if (ucicommand == "ucinewgame") {
    initializett();
    Bitboards.initialize();
    EUNN.initializennue(Bitboards.Bitboards);
  }
  if (ucicommand.substr(0, 17) == "position startpos") {
    Bitboards.initialize();
    int color = 0;
    std::string mov = "";
    for (int i = 24; i <= ucicommand.length(); i++) {
      if ((ucicommand[i] == ' ') || (i == ucicommand.length())) {
        int len = Bitboards.generatemoves(color, 0, 0);
        int played = -1;
        for (int j = 0; j < len; j++) {
          if (algebraic(Bitboards.moves[0][j]) == mov) {
            played = j;
          }
        }
        if (played >= 0) {
          Bitboards.makemove(Bitboards.moves[0][played], false);
          color ^= 1;
        }
        mov = "";
      } else {
        mov += ucicommand[i];
      }
    }
    EUNN.initializennue(Bitboards.Bitboards);
  }
  if (ucicommand.substr(0, 12) == "position fen") {
    int reader = 13;
    while (ucicommand[reader] != 'm' && reader < ucicommand.length()) {
      reader++;
    }
    std::string fen = ucicommand.substr(13, reader - 12);
    Bitboards.parseFEN(fen);
    int color = Bitboards.position & 1;
    std::string mov = "";
    for (int i = reader + 6; i <= ucicommand.length(); i++) {
      if ((ucicommand[i] == ' ') || (i == ucicommand.length())) {
        int len = Bitboards.generatemoves(color, 0, 0);
        int played = -1;
        for (int j = 0; j < len; j++) {
          if (algebraic(Bitboards.moves[0][j]) == mov) {
            played = j;
          }
        }
        if (played >= 0) {
          Bitboards.makemove(Bitboards.moves[0][played], false);
          color ^= 1;
        }
        mov = "";
      } else {
        mov += ucicommand[i];
      }
    }
    EUNN.initializennue(Bitboards.Bitboards);
  }
  if (ucicommand.substr(0, 8) == "go wtime") {
    int wtime;
    int btime;
    int winc = 0;
    int binc = 0;
    int sum;
    int add;
    int reader = 8;
    while (ucicommand[reader] != 'b') {
      reader++;
    }
    reader--;
    while (ucicommand[reader] == ' ') {
      reader--;
    }
    sum = 0;
    add = 1;
    while (ucicommand[reader] != ' ') {
      sum += ((int)(ucicommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    if (sum < 100) {
      sum = 100;
    }
    wtime = sum;
    while (ucicommand[reader] != 'w' && reader < ucicommand.length()) {
      reader++;
    }
    reader--;
    while (ucicommand[reader] == ' ') {
      reader--;
    }
    sum = 0;
    add = 1;
    while (ucicommand[reader] != ' ') {
      sum += ((int)(ucicommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    if (sum < 100) {
      sum = 100;
    }
    btime = sum;
    while (ucicommand[reader] != 'b' && reader < ucicommand.length()) {
      reader++;
    }
    reader--;
    while (ucicommand[reader] == ' ') {
      reader--;
    }
    if (reader < ucicommand.length() - 1) {
      sum = 0;
      add = 1;
      while (ucicommand[reader] != ' ') {
        sum += ((int)(ucicommand[reader] - 48)) * add;
        add *= 10;
        reader--;
      }
      winc = sum;
      reader = ucicommand.length() - 1;
      while (ucicommand[reader] == ' ') {
        reader--;
      }
      sum = 0;
      add = 1;
      while (ucicommand[reader] != ' ') {
        sum += ((int)(ucicommand[reader] - 48)) * add;
        add *= 10;
        reader--;
      }
      binc = sum;
    }
    int color = Bitboards.position & 1;
    softnodelimit = 0;
    hardnodelimit = 0;
    if (color == 0) {
      softtimelimit = wtime / 40 + winc / 3;
      hardtimelimit = wtime / 10 + winc;
    } else {
      softtimelimit = btime / 40 + binc / 3;
      hardtimelimit = btime / 10 + binc;
    }
    int score = iterative(color);
  }
  if (ucicommand.substr(0, 11) == "go movetime") {
    int sum = 0;
    int add = 1;
    int reader = ucicommand.length() - 1;
    while (ucicommand[reader] != ' ') {
      sum += ((int)(ucicommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    int color = Bitboards.position & 1;
    softnodelimit = 0;
    hardnodelimit = 0;
    softtimelimit = sum;
    hardtimelimit = sum;
    int score = iterative(color);
  }
  if (ucicommand.substr(0, 8) == "go nodes") {
    int sum = 0;
    int add = 1;
    int reader = ucicommand.length() - 1;
    while (ucicommand[reader] != ' ') {
      sum += ((int)(ucicommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    int color = Bitboards.position & 1;
    softnodelimit = sum;
    hardnodelimit = sum;
    softtimelimit = 0;
    hardtimelimit = 0;
    int score = iterative(color);
  }
  if (ucicommand.substr(0, 11) == "go infinite") {
    int color = Bitboards.position & 1;
    softnodelimit = 0;
    hardnodelimit = 0;
    softtimelimit = 0;
    hardtimelimit = 0;
    int score = iterative(color);
  }
  if (ucicommand.substr(0, 8) == "go depth") {
    int sum = 0;
    int add = 1;
    int reader = ucicommand.length() - 1;
    while (ucicommand[reader] != ' ') {
      sum += ((int)(ucicommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    int color = Bitboards.position & 1;
    softnodelimit = 0;
    hardnodelimit = 0;
    softtimelimit = 0;
    hardtimelimit = 0;
    maxdepth = sum + 1;
    int score = iterative(color);
    maxdepth = maxmaxdepth;
  }
  if (ucicommand.substr(0, 8) == "go perft") {
    start = std::chrono::steady_clock::now();
    int color = Bitboards.position & 1;
    int sum = 0;
    int add = 1;
    int reader = ucicommand.length() - 1;
    while (ucicommand[reader] != ' ') {
      sum += ((int)(ucicommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    Bitboards.perft(sum, sum, color);
  }
  if (ucicommand.substr(0, 9) == "go sperft") {
    start = std::chrono::steady_clock::now();
    int color = Bitboards.position & 1;
    int sum = 0;
    int add = 1;
    int reader = ucicommand.length() - 1;
    while (ucicommand[reader] != ' ') {
      sum += ((int)(ucicommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    Bitboards.perftnobulk(sum, sum, color);
  }
  if (ucicommand.substr(0, 9) == "set input") {
    inputfile = ucicommand.substr(10, ucicommand.length() - 10);
  }
  if (ucicommand.substr(0, 14) == "setoption name") {
    int reader = 15;
    std::string option = "";
    while (ucicommand[reader] != ' ') {
      option += ucicommand[reader];
      reader++;
    }
    if (option == "Hash") {
      reader = ucicommand.length() - 1;
      int sum = 0;
      int add = 1;
      while (ucicommand[reader] != ' ') {
        sum += ((int)(ucicommand[reader] - 48)) * add;
        add *= 10;
        reader--;
      }
      if (sum <= 1024) {
        int oldTTsize = TTsize;
        TTsize = 65536 * sum;
        TT.resize(TTsize);
        TT.shrink_to_fit();
      }
    }
    if (option == "EvalFile") {
      std::string nnuefile = ucicommand.substr(30, ucicommand.length() - 30);
      if (nnuefile != "<internal>") {
        EUNN.readnnuefile(nnuefile);
        EUNN.initializennue(Bitboards.Bitboards);
        std::cout << "info string using nnue file " << nnuefile << std::endl;
      }
    }
    if (option == "SyzygyPath") {
      std::string tbpath = ucicommand.substr(32, ucicommand.length() - 32);
      useTB = (tbpath != "<empty>");
      if (!tb_init(tbpath.c_str())) {
        std::cout << "info error initializing Syzygy TBs" << std::endl;
      } else if (useTB) {
        std::cout << "info string successful init Syzygy TBs" << std::endl;
      }
    }
    if (option == "UCI_ShowWDL") {
      std::string value = ucicommand.substr(33, ucicommand.length() - 33);
      if (value == "true") {
        showWDL = true;
      } else {
        showWDL = false;
      }
    }
    if (option == "Use") {
      std::string value = ucicommand.substr(30, ucicommand.length() - 30);
      if (value == "true") {
        useNNUE = true;
      } else {
        useNNUE = false;
      }
    }
  }
  if (ucicommand.substr(0, 3) == "see") {
    std::string mov = ucicommand.substr(4, ucicommand.length() - 4);
    int color = Bitboards.position & 1;
    int movcount = Bitboards.generatemoves(color, 0, 0);
    int internal = 0;
    for (int i = 0; i < movcount; i++) {
      if (algebraic(Bitboards.moves[0][i]) == mov) {
        internal = Bitboards.moves[0][i];
      }
    }
    std::cout << algebraic(internal) << " "
              << Bitboards.see_exceeds(internal, color, 0) << "\n";
  }
  if (ucicommand == "moveorder") {
    int color = Bitboards.position & 1;
    int movcount = Bitboards.generatemoves(color, 0, 0);
    std::cout << "Move scores:\n";
    for (int i = 0; i < movcount; i++) {
      int internal = Bitboards.moves[0][i];
      std::cout << algebraic(internal) << ": " << Histories.movescore(internal)
                << "\n";
    }
  }
}
void Engine::xboard() {
  std::string xcommand;
  getline(std::cin, xcommand);
  if (xcommand.substr(0, 8) == "protover") {
    std::cout << "feature ping=1 setboard=1 analyze=0 sigint=0 sigterm=0 "
                 "myname=\"Prolix\" variants=\"shatranj\"\nfeature done=1"
              << std::endl;
  }
  if (xcommand == "new") {
    initializett();
    Bitboards.initialize();
    EUNN.initializennue(Bitboards.Bitboards);
    gosent = false;
  }
  if (xcommand.substr(0, 8) == "setboard") {
    std::string fen = xcommand.substr(9, xcommand.length() - 9);
    Bitboards.parseFEN(fen);
    EUNN.initializennue(Bitboards.Bitboards);
  }
  if (xcommand.substr(0, 4) == "time") {
    int reader = 5;
    while ('0' <= xcommand[reader] && xcommand[reader] <= '9') {
      reader++;
    }
    reader--;
    int sum = 0;
    int add = 10;
    while (xcommand[reader] != ' ') {
      sum += ((int)(xcommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    softtimelimit = sum / 48;
    hardtimelimit = sum / 16;
  }
  if (xcommand.substr(0, 7) == "level 0") {
    int reader = 8;
    int sum1 = 0;
    int sum2 = 0;
    int add = 60000;
    while ((xcommand[reader] != ' ') && (xcommand[reader] != ':')) {
      reader++;
    }
    int save = reader;
    reader--;
    while (xcommand[reader] != ' ') {
      sum1 += ((int)(xcommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    add = 10000;
    reader = save + 1;
    if (xcommand[save] == ':') {
      while (xcommand[reader] != ' ') {
        sum1 += ((int)(xcommand[reader] - 48)) * add;
        add /= 10;
        reader++;
      }
    }
    add = 1000;
    bool incenti = false;
    reader = xcommand.length() - 1;
    while (xcommand[reader] != ' ') {
      if (xcommand[reader] >= '0') {
        sum2 += ((int)xcommand[reader] - 48) * add;
        add *= 10;
      }
      if (xcommand[reader] == '.') {
        incenti = true;
      }
      reader--;
    }
    if (incenti) {
      sum2 /= 100;
    }
    softtimelimit = sum1 / 40 + sum2 / 3;
    hardtimelimit = sum1 / 10 + sum2;
  }
  if (xcommand.substr(0, 4) == "ping") {
    int sum = 0;
    int add = 1;
    int reader = xcommand.length() - 1;
    while (xcommand[reader] != ' ') {
      sum += ((int)(xcommand[reader] - 48)) * add;
      add *= 10;
      reader--;
    }
    std::cout << "pong " << sum << std::endl;
  }
  if ((xcommand.length() == 4) || (xcommand.length() == 5)) {
    int color = Bitboards.position & 1;
    int len = Bitboards.generatemoves(color, 0, 0);
    int played = -1;
    for (int j = 0; j < len; j++) {
      if (algebraic(Bitboards.moves[0][j]) == xcommand) {
        played = j;
      }
    }
    if (played >= 0) {
      Bitboards.makemove(Bitboards.moves[0][played], false);
      if (useNNUE) {
        EUNN.forwardaccumulators(Bitboards.moves[0][played]);
      }
      if (gosent) {
        int color = Bitboards.position & 1;
        softnodelimit = 0;
        hardnodelimit = 0;
        int score = iterative(color);
      }
    }
  }
  if (xcommand == "go") {
    int color = Bitboards.position & 1;
    softnodelimit = 0;
    hardnodelimit = 0;
    int score = iterative(color);
    gosent = true;
  }
}
int main(int argc, char *argv[]) {
  initializeleaperattacks();
  initializemasks();
  initializerankattacks();
  initializezobrist();
  initializelmr();
  if (argc > 1) {
    if (std::string(argv[1]) == "bench") {
      Engine Prolix;
      Prolix.startup();
      Prolix.bench();
      return 0;
    }
    if (std::string(argv[1]) == "datagen") {
      if (argc < 5) {
        std::cerr << "Proper usage: ./(exe) datagen threads games outputfile";
        return 0;
      }
      int threads = atoi(argv[2]);
      int games = atoi(argv[3]);
      std::cout << "Generating NNUE data with " << threads << " threads x "
                << games << " games:\n";
      std::vector<std::thread> datagenerators(threads);
      std::vector<Engine> Engines(threads);
      for (int i = 0; i < threads; i++) {
        std::string outputfile =
            std::string(argv[4]) + std::to_string(i) + ".txt";
        Engines[i].startup();
        datagenerators[i] =
            std::thread(&Engine::datagen, &Engines[i], games, outputfile);
      }
      for (auto &thread : datagenerators) {
        thread.join();
      }
      return 0;
    }
    if (std::string(argv[1]) == "bookgen") {
      if (argc < 7) {
        std::cerr << "Proper usage: ./(exe) bookgen threads games outputfile "
                     "low high";
        return 0;
      }
      int threads = atoi(argv[2]);
      int games = atoi(argv[3]);
      int lowerbound = atoi(argv[5]);
      int upperbound = atoi(argv[6]);
      std::cout << "Generating book with lower bound " << lowerbound
                << " and upper bound " << upperbound << "\n";
      std::vector<std::thread> datagenerators(threads);
      std::vector<Engine> Engines(threads);
      for (int i = 0; i < threads; i++) {
        std::string outputfile =
            std::string(argv[4]) + std::to_string(i) + ".txt";
        Engines[i].startup();
        datagenerators[i] =
            std::thread(&Engine::bookgen, &Engines[i], lowerbound, upperbound,
                        games, outputfile);
      }
      for (auto &thread : datagenerators) {
        thread.join();
      }
      return 0;
    }
  }
  Engine Prolix;
  Prolix.startup();
  getline(std::cin, proto);
  if (proto == "uci") {
    std::cout << uciinfostring;
    while (true) {
      Prolix.uci();
    }
  }
  if (proto == "xboard") {
    while (true) {
      Prolix.xboard();
    }
  }
  return 0;
}
