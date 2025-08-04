#include "../engine.h"
#include "viriformat.h"
std::ifstream datainput;
void Engine::datagenautoplayplain() {
  suppressoutput = true;
  initializett();
  resetauxdata();
  int seed = mt() % 360;
  Bitboards.parseFEN(get129600FEN(seed, seed));
  std::string game = "";
  std::string result = "";
  int moves[maxmoves];
  for (int i = 0; i < 8; i++) {
    int num_moves = Bitboards.generatemoves(i & 1, 0, moves);
    if (num_moves == 0) {
      return;
    }
    int rand_move = mt() % num_moves;
    Bitboards.makemove(moves[rand_move], 0);
    game += algebraic(moves[rand_move]);
    game += " ";
  }
  if (useNNUE) {
    EUNN.initializennue(Bitboards.Bitboards);
  }
  if (Bitboards.generatemoves(0, 0, moves) == 0) {
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
    if (useNNUE && bestmove > 0) {
      EUNN.initializennue(Bitboards.Bitboards);
    }
  }
  for (int i = 0; i < maxmove; i++) {
    dataoutput << fens[i] << " | " << scores[i] << " | " << result << "\n";
  }
}
void Engine::datagenautoplayviriformat() {
  suppressoutput = true;
  initializett();
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
  if (useNNUE) {
    EUNN.initializennue(Bitboards.Bitboards);
  }
  Viriformat game;
  game.initialize(Bitboards);
  bool finished = false;
  while (!finished) {
    int color = Bitboards.position & 1;
    int score = iterative(color);
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
        result = 1;
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
    if (useNNUE && bestmove > 0) {
      EUNN.initializennue(Bitboards.Bitboards);
    }
  }
  game.writewithwdl(dataoutput, result);
}
void Engine::bookgenautoplay(int lowerbound, int upperbound) {
  suppressoutput = true;
  initializett();
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
  if (useNNUE) {
    EUNN.initializennue(Bitboards.Bitboards);
  }
  if (Bitboards.generatemoves(0, 0, moves) == 0) {
    return;
  }
  bool finished = false;
  while (!finished) {
    int color = Bitboards.position & 1;
    softnodelimit = color ? 2048 : 10240;
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
        EUNN.initializennue(Bitboards.Bitboards);
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
    } else if (Bitboards.material() < 32) {
      finished = true;
    }
  }
}
void Engine::datagen(int dataformat, int n, std::string outputfile) {
  if (dataformat == 1) {
    dataoutput.open(outputfile, std::ofstream::binary);
  } else {
    dataoutput.open(outputfile, std::ofstream::app);
  }
  softnodelimit = 10240;
  hardnodelimit = 65536;
  softtimelimit = 0;
  hardtimelimit = 0;
  for (int i = 0; i < n; i++) {
    if (dataformat == 1) {
      datagenautoplayviriformat();
    } else {
      datagenautoplayplain();
    }
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
void Engine::filter(int lowerbound, int upperbound, int softnodes,
                    int hardnodes, std::string inputfile,
                    std::string outputfile) {
  softnodelimit = softnodes;
  hardnodelimit = hardnodes;
  softtimelimit = 0;
  hardtimelimit = 0;
  suppressoutput = true;
  std::ifstream epdin;
  epdin.open(inputfile);
  std::ofstream epdout;
  epdout.open(outputfile);
  TTsize = 65536;
  TT.resize(TTsize);
  TT.shrink_to_fit();
  while (epdin) {
    std::string fen;
    getline(epdin, fen);
    if (fen == "") {
      return;
    }
    Bitboards.parseFEN(fen);
    EUNN.initializennue(Bitboards.Bitboards);
    int color = Bitboards.position & 1;
    int score = iterative(color);
    if (abs(score) >= lowerbound && abs(score) <= upperbound) {
      epdout << fen << "\n";
    }
    initializett();
  }
}