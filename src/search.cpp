#include "engine.h"
int lmr_reductions[maxmaxdepth][maxmoves];
std::chrono::time_point<std::chrono::steady_clock> start =
    std::chrono::steady_clock::now();
bool iscapture(int notation) { return ((notation >> 16) & 1); }
void initializelmr() {
  for (int i = 0; i < maxmaxdepth; i++) {
    for (int j = 0; j < maxmoves; j++) {
      lmr_reductions[i][j] =
          (i == 0 || j == 0) ? 0 : floor(788.5 + log(i) * log(j) * 471);
    }
  }
}
void Engine::startup() {
  searchlimits.maxdepth = maxmaxdepth;
  initializett();
  Bitboards.initialize();
  master.syncwith(*this);
}
void Engine::initializett() {
  TT.resize(TTsize);
  for (int i = 0; i < TTsize; i++) {
    TT[i].key = (U64)i + 1ULL;
    TT[i].data = 0;
  }
}
void Searcher::resetauxdata() {
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 64; j++) {
      countermoves[i][j] = 0;
    }
  }
  for (int i = 0; i < maxmaxdepth; i++) {
    killers[i][0] = 0;
    killers[i][1] = 0;
  }
  for (int i = 0; i < maxmaxdepth; i++) {
    for (int j = 0; j < maxmaxdepth + 1; j++) {
      pvtable[i][j] = 0;
    }
  }
  Histories->reset();
}
void Searcher::seedrng() { mt.seed(rd()); }
void Searcher::syncwith(Engine &engine) {
  resetauxdata();
  stopsearch = &(engine.stopsearch);
  TT = &(engine.TT);
  TTsize = &(engine.TTsize);
  EUNN.weights = engine.nnueweights;
  Bitboards = engine.Bitboards;
  searchlimits = engine.searchlimits;
  searchoptions = engine.searchoptions;
  EUNN.initializennue(Bitboards.Bitboards);
}
int Searcher::quiesce(int alpha, int beta, int depth, bool isPV) {
  int index = Bitboards.zobristhash % *TTsize;
  TTentry &ttentry = (*TT)[index];
  int color = Bitboards.position & 1;
  int score =
      searchoptions.useNNUE ? EUNN.evaluate(color) : Bitboards.evaluate(color);
  int bestscore = -SCORE_INF;
  int movcount;
  if (depth > 3) {
    return score;
  }
  bool incheck = Bitboards.checkers(color);
  int moves[maxmoves];
  if (incheck) {
    movcount = Bitboards.generatemoves(color, 0, moves);
    if (movcount == 0) {
      return -SCORE_WIN;
    }
  } else {
    bestscore = score;
    if (alpha < score) {
      alpha = score;
    }
    if (score >= beta) {
      return score;
    }
    movcount = Bitboards.generatemoves(color, 1, moves);
  }
  bool tthit = (ttentry.key == Bitboards.zobristhash);
  if (!isPV && tthit) {
    int score = std::min(std::max(ttentry.score(0), -SCORE_WIN), SCORE_WIN);
    int nodetype = ttentry.nodetype();
    if (nodetype == EXPECTED_PV_NODE) {
      return score;
    }
    if ((nodetype & EXPECTED_CUT_NODE) && (score >= beta)) {
      return score;
    }
    if ((nodetype & EXPECTED_ALL_NODE) && (score <= alpha)) {
      return score;
    }
  }
  for (int i = 0; i < movcount - 1; i++) {
    for (int j = i + 1;
         Histories->movescore(moves[j]) > Histories->movescore(moves[j - 1]) &&
         j > 0;
         j--) {
      std::swap(moves[j], moves[j - 1]);
    }
  }
  for (int i = 0; i < movcount; i++) {
    int mov = moves[i];
    int nextindex = (Bitboards.keyafter(mov) % *TTsize);
    __builtin_prefetch((&(*TT)[nextindex]), 0, 0);
    bool good = (incheck || Bitboards.see_exceeds(mov, color, 0));
    if (good) {
      Bitboards.makemove(mov, 1);
      if (searchoptions.useNNUE) {
        EUNN.forwardaccumulators(mov, Bitboards.Bitboards);
      }
      score = -quiesce(-beta, -alpha, depth + 1, isPV);
      Bitboards.unmakemove(mov);
      if (searchoptions.useNNUE) {
        EUNN.backwardaccumulators(mov, Bitboards.Bitboards);
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
int Searcher::alphabeta(int depth, int ply, int alpha, int beta, bool nmp,
                        int nodetype) {
  pvtable[ply][0] = ply + 1;
  if (Bitboards.repetitions() > 1) {
    return SCORE_DRAW;
  }
  if (Bitboards.halfmovecount() >= 140) {
    return SCORE_DRAW;
  }
  if (Bitboards.twokings()) {
    return SCORE_DRAW;
  }
  int color = Bitboards.position & 1;
  if (Bitboards.bareking(color ^ 1)) {
    return (SCORE_MATE + 1 - ply);
  }
  if (depth <= 0 || ply >= searchlimits.maxdepth) {
    return quiesce(alpha, beta, 0, (nodetype == EXPECTED_PV_NODE));
  }
  int tbwdl = 0;
  bool fewpieces =
      (__builtin_popcountll(Bitboards.Bitboards[0] | Bitboards.Bitboards[1]) <=
       maxtbpieces);
  if (fewpieces && searchoptions.useTB) {
    tbwdl = Bitboards.probetbwdl();
    if (!rootinTB) {
      return tbwdl * (SCORE_TB_WIN - ply);
    }
  }
  int score = -SCORE_INF;
  int bestscore = -SCORE_INF;
  bool improvedalpha = false;
  bool ttnmpgood = false;
  int movcount;
  int index = Bitboards.zobristhash % *TTsize;
  int ttmove = 0;
  int bestmove1 = -1;
  TTentry &ttentry = (*TT)[index];
  int ttdepth = ttentry.depth();
  int ttage = ttentry.age(Bitboards.gamelength);
  bool update = (depth >= ttdepth || ttage != 0);
  bool tthit = (ttentry.key == Bitboards.zobristhash);
  bool incheck = (Bitboards.checkers(color) != 0ULL);
  bool isPV = (nodetype == EXPECTED_PV_NODE);
  int staticeval =
      searchoptions.useNNUE ? EUNN.evaluate(color) : Bitboards.evaluate(color);
  searchstack[ply].eval = staticeval;
  bool improving = false;
  if (ply > 1) {
    improving = (staticeval > searchstack[ply - 2].eval);
  }
  int quiets = 0;
  if (!isPV) {
    alpha = std::max(alpha, -SCORE_MATE + ply);
    beta = std::min(beta, SCORE_MATE - ply - 1);
    if (alpha >= beta) {
      return alpha;
    }
  }
  if (tthit) {
    score = ttentry.score(ply);
    ttmove = ttentry.hashmove();
    int nodetype = ttentry.nodetype();
    if (ttdepth >= depth) {
      if (!isPV && Bitboards.repetitions() == 0) {
        if (nodetype == EXPECTED_PV_NODE) {
          return score;
        }
        if ((nodetype & EXPECTED_CUT_NODE) && (score >= beta)) {
          return score;
        }
        if ((nodetype & EXPECTED_ALL_NODE) && (score <= alpha)) {
          return score;
        }
      }
    } else {
      int margin = std::max(40, 70 * depth - 70 * ttdepth - 70 * improving);
      if (((nodetype & EXPECTED_CUT_NODE) && (score - margin >= beta)) &&
          (abs(beta) < SCORE_MAX_EVAL && !incheck) && (ply > 0) &&
          (margin < 500)) {
        return (score + beta) / 2;
      }
      ttnmpgood = (score >= beta || nodetype == EXPECTED_CUT_NODE);
    }
  }
  if (depth >= 3 && !tthit) {
    depth--;
  }
  int margin = std::max(40, 70 * depth - 70 * improving);
  if (ply > 0 && !tthit) {
    if (staticeval - margin >= beta &&
        (abs(beta) < SCORE_MAX_EVAL && !incheck) && (margin < 500)) {
      return (staticeval + beta) / 2;
    }
  }
  int moves[maxmoves];
  movcount = Bitboards.generatemoves(color, 0, moves);
  if (movcount == 0) {
    return -1 * (SCORE_MATE - ply);
  }
  if ((!incheck && Bitboards.gamephase[color] > 3) && (depth > 1 && nmp) &&
      (staticeval >= beta && !isPV) && (!tthit || ttnmpgood)) {
    Bitboards.makenullmove();
    searchstack[ply].playedmove = 0;
    int childnodetype = EXPECTED_PV_NODE ? EXPECTED_ALL_NODE : 3 - nodetype;
    score = -alphabeta(std::max(0, depth - 2 - (depth + 1) / 3), ply + 1, -beta,
                       1 - beta, false, childnodetype);
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
  int movescore[maxmoves];
  for (int i = 0; i < movcount; i++) {
    int mov = moves[i];
    if (mov == ttmove) {
      movescore[i] = (1 << 20);
    } else {
      movescore[i] = Histories->movescore(mov) +
                     Histories->conthistscore(previousmove, mov);
    }
    if (mov == killers[ply][0]) {
      movescore[i] += 20000;
    }
    /*else if (moves[ply][i] == killers[ply][1]) {
      movescore[ply][i] += 10000;
    }*/
    else if ((mov & 4095) == counter) {
      movescore[i] += 10000;
    }
    /*if (see_exceeds(moves[ply][i], color, 0)) {
        movescore[ply][i]+=15000;
    }*/
    int j = i;
    while (j > 0 && movescore[j] > movescore[j - 1]) {
      std::swap(moves[j], moves[j - 1]);
      std::swap(movescore[j], movescore[j - 1]);
      j--;
    }
  }
  for (int i = 0; i < movcount; i++) {
    bool nullwindow = (i > 0);
    int r = 0;
    bool prune = false;
    int mov = moves[i];
    int nextindex = (Bitboards.keyafter(mov) % *TTsize);
    __builtin_prefetch(&((*TT)[nextindex]), 0, 0);
    if (!iscapture(mov)) {
      quiets++;
      if (i > depth * depth + depth + 4) {
        prune = true;
      }
      if (!isPV && !incheck && depth < 5 && movescore[i] < 0) {
        prune = true;
      }
      r = std::min(1024 * (depth - 1), lmr_reductions[depth][quiets]);
    }
    r -= 1024 * isPV;
    r -= 1024 * improving;
    r += 512 * (nodetype == EXPECTED_CUT_NODE);
    r -= 512 * (mov == killers[ply][0]);
    r -= 512 * (mov == killers[ply][1]);
    r = std::max(0, r);
    if (nullwindow && !incheck && !prune && depth < 6) {
      int threshold = iscapture(mov) ? -30 * depth * depth : -100 * depth;
      prune = !Bitboards.see_exceeds(mov, color, threshold);
    }
    if (fewpieces && searchoptions.useTB) {
      Bitboards.makemove(mov, true);
      if (Bitboards.probetbwdl() != -tbwdl) {
        prune = true;
      }
      Bitboards.unmakemove(mov);
    }
    int e = (movcount == 1);
    if (!(*stopsearch) && !prune) {
      Bitboards.makemove(mov, true);
      searchstack[ply].playedmove = mov;
      if (searchoptions.useNNUE) {
        EUNN.forwardaccumulators(mov, Bitboards.Bitboards);
      }
      r /= 1024;
      if (nullwindow) {
        score = -alphabeta(depth - 1 - r, ply + 1, -alpha - 1, -alpha, true,
                           EXPECTED_CUT_NODE);
        if (score > alpha && r > 0) {
          score = -alphabeta(depth - 1, ply + 1, -alpha - 1, -alpha, true,
                             EXPECTED_CUT_NODE);
        }
        if (score > alpha && score < beta) {
          score = -alphabeta(depth - 1, ply + 1, -beta, -alpha, true,
                             EXPECTED_PV_NODE);
        }
      } else {
        int childnode =
            (nodetype == EXPECTED_PV_NODE) ? EXPECTED_PV_NODE : 3 - nodetype;
        score =
            -alphabeta(depth - 1 + e, ply + 1, -beta, -alpha, true, childnode);
      }
      Bitboards.unmakemove(mov);
      if (searchoptions.useNNUE) {
        EUNN.backwardaccumulators(mov, Bitboards.Bitboards);
      }
      if (score > bestscore) {
        if (score > alpha) {
          if (score >= beta) {
            if (update && !(*stopsearch)) {
              ttentry.update(Bitboards.zobristhash, Bitboards.gamelength, depth,
                             ply, score, EXPECTED_CUT_NODE, mov);
            }
            if (!iscapture(mov) && (killers[ply][0] != mov)) {
              killers[ply][1] = killers[ply][0];
              killers[ply][0] = mov;
            }
            Histories->updatemainhistory(mov, depth * depth);
            if (!iscapture(mov)) {
              Histories->updateconthist(previousmove, mov, depth * depth);
            }
            for (int j = 0; j < i; j++) {
              int mov2 = moves[j];
              Histories->updatemainhistory(mov2, -3 * depth);
              if (!iscapture(mov2)) {
                Histories->updateconthist(previousmove, mov2, -3 * depth);
              }
            }
            if (ply > 0 && nmp && !iscapture(mov)) {
              countermoves[previouspiece - 2][previoussquare] = (mov & 4095);
            }
            return score;
          }
          alpha = score;
          improvedalpha = 1;
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
      if (ismaster && Bitboards.nodecount > searchlimits.hardnodelimit &&
          searchlimits.hardnodelimit > 0) {
        *stopsearch = true;
      }
      if ((Bitboards.nodecount & 1023) == 0) {
        auto now = std::chrono::steady_clock::now();
        auto timetaken =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        if (ismaster && timetaken.count() > searchlimits.hardtimelimit &&
            searchlimits.hardtimelimit > 0) {
          *stopsearch = true;
        }
      }
    }
  }
  int realnodetype = improvedalpha ? EXPECTED_PV_NODE : EXPECTED_ALL_NODE;
  int savedmove = improvedalpha ? moves[bestmove1] : ttmove;
  if (((update || (realnodetype == EXPECTED_PV_NODE)) && !(*stopsearch))) {
    ttentry.update(Bitboards.zobristhash, Bitboards.gamelength, depth, ply,
                   bestscore, realnodetype, savedmove);
  }
  return bestscore;
}
int Searcher::wdlmodel(int eval) {
  int material = Bitboards.material();
  double m = std::max(std::min(material, 64), 4) / 32.0;
  double as[4] = {1.68116882, 4.65282732, -59.57468312, 227.74637225};
  double bs[4] = {-0.87426669, 2.05986232, -1.43046196, 52.66782181};
  double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
  double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];
  return int(0.5 + 1000 / (1 + exp((a - double(eval)) / b)));
}
int Searcher::normalize(int eval) {
  if (abs(eval) >= SCORE_MAX_EVAL) {
    return eval;
  }
  int material = Bitboards.material();
  double m = std::max(std::min(material, 64), 4) / 32.0;
  double as[4] = {1.68116882, 4.65282732, -59.57468312, 227.74637225};
  double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
  return round(100 * eval / a);
}
int Searcher::iterative() {
  Bitboards.nodecount = 0;
  if (ismaster) {
    *stopsearch = false;
  }
  start = std::chrono::steady_clock::now();
  int color = Bitboards.position & 1;
  int score =
      searchoptions.useNNUE ? EUNN.evaluate(color) : Bitboards.evaluate(color);
  int returnedscore = score;
  int depth = 1;
  int bestmove1 = 0;
  int tbscore = 0;
  resetauxdata();
  rootinTB = (__builtin_popcountll(Bitboards.Bitboards[0] |
                                   Bitboards.Bitboards[1]) <= maxtbpieces);
  if (rootinTB && searchoptions.useTB) {
    tbscore = Bitboards.probetbwdl();
  }
  while (!(*stopsearch)) {
    bestmove = -1;
    int delta = 20;
    int alpha = returnedscore - delta;
    int beta = returnedscore + delta;
    bool fail = true;
    while (fail) {
      int score1 = alphabeta(depth, 0, alpha, beta, false, EXPECTED_PV_NODE);
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
    if ((Bitboards.nodecount < searchlimits.hardnodelimit ||
         searchlimits.hardnodelimit <= 0) &&
        (timetaken.count() < searchlimits.hardtimelimit ||
         searchlimits.hardtimelimit <= 0) &&
        depth < searchlimits.maxdepth && bestmove >= 0) {
      returnedscore = score;
      if (rootinTB && searchoptions.useTB && abs(score) < SCORE_WIN) {
        score = SCORE_TB_WIN * tbscore;
      }
      if (ismaster) {
        if (proto == "uci" && !searchoptions.suppressoutput) {
          if (abs(score) <= SCORE_WIN) {
            int printedscore =
                searchoptions.normalizeeval ? normalize(score) : score;
            std::cout << "info depth " << depth << " nodes "
                      << Bitboards.nodecount << " time " << timetaken.count()
                      << " score cp " << printedscore;
            if (searchoptions.showWDL) {
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
              matescore = 1 + (SCORE_MATE - score) / 2;
            } else {
              matescore = (-SCORE_MATE - score) / 2;
            }
            std::cout << "info depth " << depth << " nodes "
                      << Bitboards.nodecount << " time " << timetaken.count()
                      << " score mate " << matescore;
            if (searchoptions.showWDL) {
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
          int printedscore =
              searchoptions.normalizeeval ? normalize(score) : score;
          std::cout << depth << " " << printedscore << " "
                    << timetaken.count() / 10 << " " << Bitboards.nodecount
                    << " ";
          for (int i = 1; i < pvtable[0][0]; i++) {
            std::cout << algebraic(pvtable[0][i]) << " ";
          }
          std::cout << std::endl;
        }
      }
      depth++;
      if (depth == searchlimits.maxdepth && ismaster) {
        *stopsearch = true;
      }
      bestmove1 = bestmove;
    } else if (ismaster) {
      *stopsearch = true;
    }
    if (ismaster && ((timetaken.count() > searchlimits.softtimelimit &&
                      searchlimits.softtimelimit > 0) ||
                     (Bitboards.nodecount > searchlimits.softnodelimit &&
                      searchlimits.softnodelimit > 0))) {
      *stopsearch = true;
    }
  }
  auto now = std::chrono::steady_clock::now();
  auto timetaken =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
  if (ismaster) {
    if (proto == "uci" && !searchoptions.suppressoutput) {
      int nps = 1000 * (Bitboards.nodecount /
                        std::max((uint64_t)1, (uint64_t)timetaken.count()));
      std::cout << "info nodes " << Bitboards.nodecount << " nps " << nps
                << std::endl;
    }
    if (proto == "uci" && !searchoptions.suppressoutput) {
      std::cout << "bestmove " << algebraic(bestmove1) << std::endl;
    }
  }
  if (proto == "xboard") {
    if (ismaster) {
      std::cout << "move " << algebraic(bestmove1) << std::endl;
    }
    Bitboards.makemove(bestmove1, 0);
    if (searchoptions.useNNUE) {
      EUNN.forwardaccumulators(bestmove1, Bitboards.Bitboards);
    }
  }
  bestmove = bestmove1;
  if (!ismaster) {
    delete Histories;
  }
  return returnedscore;
}
void Engine::spawnworker() {
  Searcher worker;
  worker.ismaster = false;
  worker.syncwith(*this);
  int score = worker.iterative();
}