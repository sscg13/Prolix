#include "engine.h"

// clang-format off
std::string uciinfostring =
    "id name Prolix \n"
    "id author sscg13 \n"
    "option name UCI_Variant type combo default shatranj var shatranj \n"
    "option name Threads type spin default 1 min 1 max 1 \n"
    "option name Hash type spin default 32 min 1 max 1024 \n"
    "option name UseNNUE type check default true \n"
    "option name NormalizeEval type check default true \n"
    "option name EvalFile type string default <internal> \n"
    "option name UCI_ShowWDL type check default true \n"
    "option name SyzygyPath type string default <empty> \n"
    "uciok\n";
// clang-format on
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
    EUNN->initializennue(Bitboards.Bitboards);
  }
  if (ucicommand.substr(0, 17) == "position startpos") {
    Bitboards.initialize();
    int color = 0;
    int moves[maxmoves];
    std::string mov = "";
    for (int i = 24; i <= ucicommand.length(); i++) {
      if ((ucicommand[i] == ' ') || (i == ucicommand.length())) {
        int len = Bitboards.generatemoves(color, 0, moves);
        int played = -1;
        for (int j = 0; j < len; j++) {
          if (algebraic(moves[j]) == mov) {
            played = j;
          }
        }
        if (played >= 0) {
          Bitboards.makemove(moves[played], false);
          color ^= 1;
        }
        mov = "";
      } else {
        mov += ucicommand[i];
      }
    }
    EUNN->initializennue(Bitboards.Bitboards);
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
    int moves[maxmoves];
    for (int i = reader + 6; i <= ucicommand.length(); i++) {
      if ((ucicommand[i] == ' ') || (i == ucicommand.length())) {
        int len = Bitboards.generatemoves(color, 0, moves);
        int played = -1;
        for (int j = 0; j < len; j++) {
          if (algebraic(moves[j]) == mov) {
            played = j;
          }
        }
        if (played >= 0) {
          Bitboards.makemove(moves[played], false);
          color ^= 1;
        }
        mov = "";
      } else {
        mov += ucicommand[i];
      }
    }
    EUNN->initializennue(Bitboards.Bitboards);
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
        EUNN->readnnuefile(nnuefile);
        EUNN->initializennue(Bitboards.Bitboards);
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
    if (option == "NormalizeEval") {
      std::string value = ucicommand.substr(35, ucicommand.length() - 35);
      if (value == "true") {
        normalizeeval = true;
      } else {
        normalizeeval = false;
      }
    }
    if (option == "UseNNUE") {
      std::string value = ucicommand.substr(29, ucicommand.length() - 29);
      if (value == "true") {
        useNNUE = true;
      } else {
        useNNUE = false;
      }
    }
  }
  if (ucicommand == "eval") {
    int color = Bitboards.position & 1;
    int eval = useNNUE ? EUNN->evaluate(color) : Bitboards.evaluate(color);
    std::cout << "Raw eval: " << eval << "\n";
    std::cout << "Normalized eval: " << normalize(eval) << "\n";
  }
  if (ucicommand.substr(0, 3) == "see") {
    std::string mov = ucicommand.substr(4, ucicommand.length() - 4);
    int color = Bitboards.position & 1;
    int moves[maxmoves];
    int movcount = Bitboards.generatemoves(color, 0, moves);
    int internal = 0;
    for (int i = 0; i < movcount; i++) {
      if (algebraic(moves[i]) == mov) {
        internal = moves[i];
      }
    }
    std::cout << algebraic(internal) << " "
              << Bitboards.see_exceeds(internal, color, 0) << "\n";
  }
  if (ucicommand == "moveorder") {
    int color = Bitboards.position & 1;
    int moves[maxmoves];
    int movcount = Bitboards.generatemoves(color, 0, moves);
    std::cout << "Move scores:\n";
    for (int i = 0; i < movcount; i++) {
      int internal = moves[i];
      std::cout << algebraic(internal) << ": " << Histories->movescore(internal)
                << "\n";
    }
  }
}