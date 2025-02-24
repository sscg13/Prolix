#include "search.h"
#include "viriformat.h"
std::ifstream datainput;
void Engine::datagenautoplay() {
    suppressoutput = true;
    initializett();
    resetauxdata();
    int seed = mt() % 360;
    Bitboards.parseFEN(get129600FEN(seed, seed));
    int result;
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
    }
    if (Bitboards.generatemoves(0, 0, 0) == 0) {
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
      //Filter criteria: best (next) move non-capture, score not mate score, not in check
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
      } else if (Bitboards.generatemoves(color ^ 1, 0, 0) == 0) {
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
        EUNN.forwardaccumulators(bestmove, Bitboards.Bitboards);
      }
    }
    game.writewithwdl(dataoutput, result);
    suppressoutput = false;
    initializett();
    resetauxdata();
    Bitboards.initialize();
  }
  
void Engine::bookgenautoplay(int lowerbound, int upperbound) {
    suppressoutput = true;
    initializett();
    resetauxdata();
    int seed = mt() % 360;
    Bitboards.parseFEN(get129600FEN(seed, seed));
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
          EUNN.forwardaccumulators(bestmove, Bitboards.Bitboards);
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
    dataoutput.open(outputfile, std::ofstream::binary);
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