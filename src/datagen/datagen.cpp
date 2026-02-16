#include "../engine.h"
#include "viriformat.h"
std::ifstream datainput;
void Searcher::datagenautoplayplain() {
  resetauxdata();
  int seed = mt() % 360;
  Bitboards.parseFEN(get129600FEN(seed, seed));
  std::string game = "";
  std::string result = "";
  int moves[maxmoves];
  for (int i = 0; i < 16; i++) {
    int num_moves = Bitboards.generatemoves(i & 1, 0, moves);
    if (num_moves == 0) {
      return;
    }
    int rand_move = mt() % num_moves;
    Bitboards.makemove(moves[rand_move], 0);
    game += algebraic(moves[rand_move]);
    game += " ";
  }
  if (searchoptions.useNNUE) {
    EUNN.initialize(Bitboards.Bitboards);
  }
  if (Bitboards.generatemoves(0, 0, moves) == 0) {
    return;
  }
  int score = quiesce(-SCORE_INF, SCORE_INF, 0, true);
  if (std::abs(score) > 400) {
    return;
  }
  std::string fens[1024];
  int scores[1024];
  int maxmove = 0;
  bool finished = false;
  int wincount = 0;
  int drawcount = 0;
  while (!finished) {
    int color = Bitboards.position & 1;
    int score = iterative();
    if (((Bitboards.position >> 1) < 40) && (((bestmove >> 16) & 1) == 0) &&
        (Bitboards.checkers(color) == 0ULL) && (abs(score) < SCORE_WIN)) {
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
    if (abs(score) >= 1000) {
      wincount++;
    } else {
      wincount = 0;
    }
    if (abs(score) <= 15 && maxmove >= 40) {
      drawcount++;
    } else {
      drawcount = 0;
    }
    if (wincount >= 5) {
      finished = true;
      if (score * (1 - 2 * color) >= 1000) {
        result = "1.0";
      } else {
        result = "0.0";
      }
    } else if (drawcount >= 8) {
      finished = true;
      result = "0.5";
    } else if (Bitboards.twokings()) {
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
    } else if (Bitboards.generatemoves(color ^ 1, 0, moves) == 0) {
      finished = true;
      if (color == 0) {
        result = "1.0";
      } else {
        result = "0.0";
      }
    } else if (Bitboards.halfmovecount() >= 140) {
      finished = true;
      result = "0.5";
    } else if (maxmove >= 1000) {
      finished = true;
      result = "0.5";
    }
    if (!finished && searchoptions.useNNUE && bestmove > 0) {
      EUNN.initialize(Bitboards.Bitboards);
    }
  }
  for (int i = 0; i < maxmove; i++) {
    dataoutput << fens[i] << " | " << scores[i] << " | " << result << "\n";
  }
}
void Searcher::datagenautoplayviriformat() {
  resetauxdata();
  int seed = mt() % 360;
  Bitboards.parseFEN(get129600FEN(seed, seed));
  int result;
  int moves[maxmoves];
  for (int i = 0; i < 8; i++) {
    int num_moves = Bitboards.generatemoves(i & 1, 0, moves);
    if (num_moves == 0) {
      return;
    }
    int rand_move = mt() % num_moves;
    Bitboards.makemove(moves[rand_move], 0);
  }
  if (Bitboards.generatemoves(0, 0, moves) == 0) {
    return;
  }
  if (searchoptions.useNNUE) {
    EUNN.initialize(Bitboards.Bitboards);
  }
  Viriformat game;
  game.initialize(Bitboards);
  bool finished = false;
  while (!finished) {
    int color = Bitboards.position & 1;
    int score = iterative();
    // Filter criteria: best (next) move non-capture, score not mate score, not
    // in check
    game.push(bestmove, score * (1 - 2 * color));
    if (bestmove == 0) {
      std::cout << "Null best move? mitigating by using proper null move \n";
      Bitboards.makenullmove();
    } else {
      Bitboards.makemove(bestmove, 0);
    }
    if (Bitboards.twokings()) {
      finished = true;
      result = 1;
    } else if (Bitboards.bareking(color)) {
      finished = true;
      if (color == 0) {
        result = 0;
      } else {
        result = 2;
      }
    } else if (Bitboards.repetitions() >= 2) {
      finished = true;
      result = 1;
    } else if (Bitboards.generatemoves(color ^ 1, 0, moves) == 0) {
      finished = true;
      if (color == 0) {
        result = 2;
      } else {
        result = 0;
      }
    } else if (Bitboards.halfmovecount() >= 140) {
      finished = true;
      result = 1;
    }
    if (searchoptions.useNNUE && bestmove > 0) {
      EUNN.initialize(Bitboards.Bitboards);
    }
  }
  game.writewithwdl(dataoutput, result);
}
void Searcher::bookgenautoplay(int lowerbound, int upperbound) {
  resetauxdata();
  int seed = mt() % 360;
  Bitboards.parseFEN(get129600FEN(seed, seed));
  int moves[maxmoves];
  for (int i = 0; i < 8; i++) {
    int num_moves = Bitboards.generatemoves(i & 1, 0, moves);
    if (num_moves == 0) {
      return;
    }
    int rand_move = mt() % num_moves;
    Bitboards.makemove(moves[rand_move], 0);
  }
  if (searchoptions.useNNUE) {
    EUNN.initialize(Bitboards.Bitboards);
  }
  if (Bitboards.generatemoves(0, 0, moves) == 0) {
    return;
  }
  bool finished = false;
  while (!finished) {
    int color = Bitboards.position & 1;
    searchlimits.softnodelimit = color ? 2048 : 8192;
    int score = iterative();
    if ((bestmove > 0) && (((bestmove >> 16) & 1) == 0) &&
        (Bitboards.checkers(color) == 0ULL) && (normalize(abs(score)) <= upperbound) &&
        (normalize(abs(score)) >= lowerbound)) {
      dataoutput << Bitboards.getFEN() << "\n";
      std::cout << "success\n";
      finished = true;
    }
    if (bestmove == 0) {
      std::cout << "Null best move? mitigating by using proper null move \n";
      Bitboards.makenullmove();
    } else {
      Bitboards.makemove(bestmove, 0);
      if (searchoptions.useNNUE) {
        EUNN.initialize(Bitboards.Bitboards);
      }
    }
    if (Bitboards.twokings()) {
      finished = true;
    } else if (Bitboards.bareking(color)) {
      finished = true;
    } else if (Bitboards.repetitions() >= 2) {
      finished = true;
    } else if (Bitboards.generatemoves(color ^ 1, 0, moves) == 0) {
      finished = true;
    } else if (Bitboards.material() < 40) {
      finished = true;
    }
  }
}
void Engine::datagen(int dataformat, int threads, int n,
                     std::string outputfile) {
  if (dataformat == 1) {
    master.dataoutput.open(outputfile, std::ofstream::binary);
  } else {
    master.dataoutput.open(outputfile, std::ofstream::app);
  }
  searchlimits.softnodelimit = 7168;
  searchlimits.hardnodelimit = 65536;
  searchlimits.softtimelimit = 0;
  searchlimits.hardtimelimit = 0;
  searchoptions.suppressoutput = true;
  master.syncwith(*this);
  for (int i = 0; i < n; i++) {
    if (dataformat == 1) {
      master.datagenautoplayviriformat();
    } else {
      master.datagenautoplayplain();
    }
    std::cout << i << "\n";
  }
  master.dataoutput.close();
}
void Engine::bookgen(int lowerbound, int upperbound, int threads, int n,
                     std::string outputfile) {
  master.dataoutput.open(outputfile, std::ofstream::app);
  searchlimits.hardnodelimit = 65536;
  searchlimits.softtimelimit = 0;
  searchlimits.hardtimelimit = 0;
  searchoptions.suppressoutput = true;
  master.syncwith(*this);
  for (int i = 0; i < n; i++) {
    master.bookgenautoplay(lowerbound, upperbound);
    std::cout << i << "\n";
  }
}
void Engine::filter(int lowerbound, int upperbound, int softnodes,
                    int hardnodes, int threads, std::string inputfile,
                    std::string outputfile) {
  searchlimits.softnodelimit = softnodes;
  searchlimits.hardnodelimit = hardnodes;
  searchlimits.softtimelimit = 0;
  searchlimits.hardtimelimit = 0;
  searchoptions.suppressoutput = true;
  std::ifstream epdin;
  epdin.open(inputfile);
  std::ofstream epdout;
  epdout.open(outputfile);
  while (epdin) {
    std::string fen;
    getline(epdin, fen);
    if (fen == "") {
      return;
    }
    Bitboards.parseFEN(fen);
    int color = Bitboards.position & 1;
    master.syncwith(*this);
    int score = master.iterative();
    if (master.normalize(abs(score)) >= lowerbound && master.normalize(abs(score)) <= upperbound) {
      epdout << fen << "\n";
    }
  }
}