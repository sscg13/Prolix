#include "datagen.cpp"
int lmr_reductions[maxmaxdepth][256];
std::chrono::time_point<std::chrono::steady_clock> start =
    std::chrono::steady_clock::now();
bool iscapture(int notation) { return ((notation >> 16) & 1); }
void initializelmr() {
  for (int i = 0; i < maxmaxdepth; i++) {
    for (int j = 0; j < 256; j++) {
      lmr_reductions[i][j] =
          (i == 0 || j == 0) ? 0 : floor(0.77 + log(i) * log(j) * 0.46);
    }
  }
}
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
int Engine::quiesce(int alpha, int beta, int color, int depth) {
  int score = useNNUE ? EUNN.evaluate(color) : Bitboards.evaluate(color);
  int bestscore = -30000;
  int movcount;
  if (depth > 3) {
    return score;
  }
  bool incheck = Bitboards.checkers(color);
  if (incheck) {
    movcount = Bitboards.generatemoves(color, 0, maxmaxdepth + depth);
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
    movcount = Bitboards.generatemoves(color, 1, maxmaxdepth + depth);
  }

  if (depth < 1) {
    for (int i = 0; i < movcount - 1; i++) {
      for (int j = i + 1;
           Histories.movescore(Bitboards.moves[maxmaxdepth + depth][j]) >
               Histories.movescore(Bitboards.moves[maxmaxdepth + depth][j - 1]) &&
           j > 0;
           j--) {
        std::swap(Bitboards.moves[maxmaxdepth + depth][j],
                  Bitboards.moves[maxmaxdepth + depth][j - 1]);
      }
    }
  }
  for (int i = 0; i < movcount; i++) {
    int mov = Bitboards.moves[maxmaxdepth + depth][i];
    bool good = (incheck || Bitboards.see_exceeds(mov, color, 0));
    if (good) {
      Bitboards.makemove(mov, 1);
      if (useNNUE) {
        EUNN.forwardaccumulators(mov, Bitboards.Bitboards);
      }
      score = -quiesce(-beta, -alpha, color ^ 1, depth + 1);
      Bitboards.unmakemove(mov);
      if (useNNUE) {
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
  bool update = (depth >= (ttdepth - ttage / 2));
  bool tthit = (TT[index].key == Bitboards.zobristhash);
  bool incheck = (Bitboards.checkers(color) != 0ULL);
  bool isPV = (beta - alpha > 1);
  int staticeval = useNNUE ? EUNN.evaluate(color) : Bitboards.evaluate(color);
  searchstack[ply].eval = staticeval;
  bool improving = false;
  if (ply > 1) {
    improving = (staticeval > searchstack[ply - 2].eval);
  }
  int quiets = 0;
  if (tthit) {
    score = TT[index].score();
    ttmove = TT[index].hashmove();
    int nodetype = TT[index].nodetype();
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
  if (depth >= 3 && !tthit) {
    depth--;
  }
  int margin = std::max(40, 70 * (depth - improving));
  if (ply > 0 && score == -30000) {
    if (staticeval - margin >= beta && (abs(beta) < 20000 && !incheck)) {
      return (staticeval + beta) / 2;
    }
  }
  movcount = Bitboards.generatemoves(color, 0, ply);
  if (movcount == 0) {
    return -1 * (28000 - ply);
  }
  if ((!incheck && Bitboards.gamephase[color] > 3) && (depth > 1 && nmp) &&
      (staticeval >= beta && !isPV)) {
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
    if (!iscapture(mov)) {
      quiets++;
      if (quiets > depth * depth + depth + 1) {
        prune = true;
      }
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
    int e = (movcount == 1);
    if (!stopsearch && !prune) {
      Bitboards.makemove(mov, true);
      searchstack[ply].playedmove = mov;
      if (useNNUE) {
        EUNN.forwardaccumulators(mov, Bitboards.Bitboards);
      }
      if (nullwindow) {
        score = -alphabeta(depth - 1 - r, ply + 1, -alpha - 1, -alpha,
                           color ^ 1, true);
        if (score > alpha && r > 0) {
          score = -alphabeta(depth - 1, ply + 1, -alpha - 1, -alpha, color ^ 1,
                             true);
        }
        if (score > alpha && score < beta) {
          score =
              -alphabeta(depth - 1, ply + 1, -beta, -alpha, color ^ 1, true);
        }
      } else {
        score = -alphabeta(depth - 1, ply + 1, -beta, -alpha, color ^ 1, true);
      }
      Bitboards.unmakemove(mov);
      if (useNNUE) {
        EUNN.backwardaccumulators(mov, Bitboards.Bitboards);
      }
      if (score > bestscore) {
        if (score > alpha) {
          if (score >= beta) {
            if (update && !stopsearch) {
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
  if (((update || allnode) && !stopsearch)) {
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
      EUNN.forwardaccumulators(bestmove1, Bitboards.Bitboards);
    }
  }
  bestmove = bestmove1;
  return returnedscore;
}