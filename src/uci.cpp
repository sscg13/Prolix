#include "engine.h"
#include "external/probetool/jtbprobe.h"
#include <sstream>

// clang-format off
std::string uciinfostring =
    "id name Prolix \n"
    "id author sscg13 \n"
    "option name UCI_Variant type combo default shatranj var shatranj \n"
    "option name Threads type spin default 1 min 1 max 8 \n"
    "option name Hash type spin default 32 min 1 max 1024 \n"
    "option name UseNNUE type check default true \n"
    "option name NormalizeEval type check default true \n"
    "option name EvalFile type string default <internal> \n"
    "option name UCI_ShowWDL type check default true \n"
    "option name SyzygyPath type string default <empty> \n"
    "uciok\n";
// clang-format on
Limits infinitesearch = {0, 0, 0, 0, maxmaxdepth};
void Engine::uci() {
  std::string ucicommand;
  getline(std::cin, ucicommand);
  std::stringstream tokens(ucicommand);
  std::string token;
  tokens >> token;
  if (token == "uci") {
    std::cout << uciinfostring;
  }
  if (token == "quit") {
    exit(0);
  }
  if (token == "isready") {
    std::cout << "readyok" << std::endl;
  }
  if (token == "ucinewgame") {
    initializett();
    Bitboards.initialize();
  }
  if (token == "position") {
    std::string fen;
    tokens >> token;
    if (token == "startpos") {
      fen = "rnbkqbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBKQBNR w - - 0 1";
    }
    if (token == "fen") {
      for (int i = 0; i < 6; i++) {
        tokens >> token;
        fen += token;
        fen += " ";
      }
    }
    Bitboards.parseFEN(fen);
    int color = Bitboards.position & 1;
    int moves[maxmoves];
    while (tokens >> token) {
      if (token != "moves") {
        int len = Bitboards.generatemoves(color, 0, moves);
        int played = -1;
        for (int j = 0; j < len; j++) {
          if (algebraic(moves[j]) == token) {
            played = j;
          }
        }
        if (played >= 0) {
          Bitboards.makemove(moves[played], false);
          color ^= 1;
        }
      }
    }
  }
  if (token == "setoption") {
    std::string option;
    std::string value;
    tokens >> token;
    tokens >> option;
    tokens >> token;
    tokens >> value;
    if (option == "Threads") {
      threads = std::stoi(value);
    }
    if (option == "Hash") {
      int sum = std::stoi(value);
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
        nnueweights->readnnuefile(nnuefile);
        std::cout << "info string using nnue file " << nnuefile << std::endl;
      }
    }
    if (option == "SyzygyPath") {
      searchoptions.useTB = (value != "<empty>");
      TB_init(value.data());
      std::cout << "info string found " << TB_NumTables[0] << " WDL and "
                << TB_NumTables[2] << " DTZ files \n";
    }
    if (option == "UCI_ShowWDL") {
      if (value == "true") {
        searchoptions.showWDL = true;
      } else {
        searchoptions.showWDL = false;
      }
    }
    if (option == "NormalizeEval") {
      if (value == "true") {
        searchoptions.normalizeeval = true;
      } else {
        searchoptions.normalizeeval = false;
      }
    }
    if (option == "UseNNUE") {
      if (value == "true") {
        searchoptions.useNNUE = true;
      } else {
        searchoptions.useNNUE = false;
      }
    }
  }
  if (token == "go") {
    int wtime = 0;
    int btime = 0;
    int winc = 0;
    int binc = 0;
    searchlimits = infinitesearch;
    while (tokens >> token) {
      if (token == "wtime") {
        tokens >> token;
        wtime = std::stoi(token);
      }
      if (token == "winc") {
        tokens >> token;
        winc = std::stoi(token);
      }
      if (token == "btime") {
        tokens >> token;
        btime = std::stoi(token);
      }
      if (token == "binc") {
        tokens >> token;
        binc = std::stoi(token);
      }
      if (token == "movetime") {
        tokens >> token;
        searchlimits.hardtimelimit = std::stoi(token);
      }
      if (token == "depth") {
        tokens >> token;
        searchlimits.maxdepth = 1 + std::stoi(token);
      }
      if (token == "nodes") {
        tokens >> token;
        searchlimits.hardnodelimit = std::stoi(token);
      }
    }
    int color = Bitboards.position & 1;
    int ourtime = color ? btime : wtime;
    int ourinc = color ? binc : winc;
    if (ourtime > 0) {
      searchlimits.softtimelimit = ourtime / 40 + ourinc / 3;
      searchlimits.hardtimelimit = ourtime / 10 + ourinc;
    }
    master.syncwith(*this);
    if (threads > 1) {
      std::vector<std::thread> workers(threads - 1);
      for (int i = 0; i < threads - 1; i++) {
        workers[i] = std::thread(&Engine::spawnworker, this);
      }
      int score = master.iterative();
      for (auto &thread : workers) {
        if (thread.joinable()) {
          thread.join();
        }
      }
    } else {
      int score = master.iterative();
    }
  }
  if (token == "perft") {
    tokens >> token;
    int depth = std::stoi(token);
    start = std::chrono::steady_clock::now();
    int color = Bitboards.position & 1;
    Bitboards.perft(depth, depth, color);
  }
  if (token == "sperft") {
    tokens >> token;
    int depth = std::stoi(token);
    start = std::chrono::steady_clock::now();
    int color = Bitboards.position & 1;
    Bitboards.perftnobulk(depth, depth, color);
  }
  if (token == "tbwdl") {
    if (__builtin_popcountll(Bitboards.Bitboards[0] | Bitboards.Bitboards[1]) <
        6) {
      int result = Bitboards.probetbwdl();
      std::cout << "WDL Probe result: " << result << "\n";
    } else {
      std::cout << "Error: more than 5 pieces \n";
    }
  }
  /*if (ucicommand == "eval") {
    int color = Bitboards.position & 1;
    int eval = useNNUE ? EUNN.evaluate(color) : Bitboards.evaluate(color);
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
  }*/
}