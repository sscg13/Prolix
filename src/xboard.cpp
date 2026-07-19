#include "engine.h"
#include <sstream>

namespace {
int parseleveltime(const std::string &value) {
  const size_t colon = value.find(':');
  if (colon == std::string::npos) {
    return std::stoi(value) * 60000;
  }
  const int minutes = std::stoi(value.substr(0, colon));
  const int seconds = std::stoi(value.substr(colon + 1));
  return (minutes * 60 + seconds) * 1000;
}
}

void Engine::xboard() {
  std::string xcommand;
  getline(std::cin, xcommand);
  std::stringstream tokens(xcommand);
  std::string token;
  tokens >> token;

  if (token == "quit") {
    exit(0);
  }
  if (token == "protover") {
    std::cout << "feature ping=1 setboard=1 analyze=0 sigint=0 sigterm=0 "
                 "colors=0 draw=0 smp=1 memory=1\n"
                 "feature option=\"EvalLevel -spin 8 0 8\"\n"
                 "feature myname=\"Prolix\" variants=\"shatranj\"\n"
                 "feature done=1"
              << std::endl;
    return;
  }

  if (token == "new") {
    initializett();
    Bitboards.parseFEN(startposFEN);
    searchlimits.maxdepth = maxmaxdepth;
    gosent = false;
    return;
  }
  if (token == "variant") {
    tokens >> token;
    if (token != "shatranj") {
      std::cout << "Error (unknown variant): " << token << std::endl;
    }
    return;
  }
  if (token == "setboard") {
    std::string fen;
    std::getline(tokens, fen);
    const size_t first = fen.find_first_not_of(" \t");
    if (first == std::string::npos) {
      fen.clear();
    } else {
      fen.erase(0, first);
    }
    Bitboards.parseFEN(fen);
    return;
  }
  if (token == "sd") {
    int depth;
    if (tokens >> depth) {
      searchlimits.maxdepth = std::max(1, std::min(depth + 1, maxmaxdepth));
    }
    return;
  }
  if (token == "st") {
    int seconds;
    if (tokens >> seconds) {
      searchlimits.softtimelimit = 0;
      searchlimits.hardtimelimit = std::max(0, seconds * 1000);
    }
    return;
  }
  if (token == "cores") {
    int count;
    if (tokens >> count) {
      setthreadcount(count);
    }
    return;
  }
  if (token == "memory") {
    int megabytes;
    if (tokens >> megabytes) {
      sethashsize(megabytes);
    }
    return;
  }
  if (token == "time") {
    int centiseconds;
    if (tokens >> centiseconds) {
      const int milliseconds = centiseconds * 10;
      searchlimits.softtimelimit = milliseconds / 48;
      searchlimits.hardtimelimit = milliseconds / 16;
    }
    return;
  }
  if (token == "level") {
    int movesPerSession;
    std::string baseTime;
    int increment;
    if (tokens >> movesPerSession >> baseTime >> increment) {
      if (movesPerSession == 0) {
        const int baseMilliseconds = parseleveltime(baseTime);
        const int incrementMilliseconds = increment * 1000;
        searchlimits.softtimelimit =
            baseMilliseconds / 40 + incrementMilliseconds / 3;
        searchlimits.hardtimelimit =
            baseMilliseconds / 10 + incrementMilliseconds;
      }
    }
    return;
  }
  if (token == "otim") {
    // Prolix only uses its own remaining time; otim is intentionally ignored.
    return;
  }
  if (token == "post") {
    searchoptions.minimal = false;
    return;
  }
  if (token == "nopost") {
    searchoptions.minimal = true;
    return;
  }
  if (token == "option") {
    std::string option;
    if (tokens >> option) {
      const size_t equals = option.find('=');
      std::string name = option;
      std::string value;
      if (equals == std::string::npos) {
        tokens >> value;
      } else {
        name = option.substr(0, equals);
        value = option.substr(equals + 1);
      }
      if (!value.empty()) {
        const int parsed = std::stoi(value);
        if (name == "EvalLevel") {
          setevallevel(parsed);
        }
      }
    }
    return;
  }
  if (token == "ping") {
    int pingNumber;
    if (tokens >> pingNumber) {
      std::cout << "pong " << pingNumber << std::endl;
    }
    return;
  }
  if (token == "go") {
    searchlimits.softnodelimit = 0;
    searchlimits.hardnodelimit = 0;
    runsearch(true);
    gosent = true;
    return;
  }
  if (token == "usermove") {
    tokens >> token;
  }
  if ((token.length() == 4) || (token.length() == 5)) {
    int color = Bitboards.position & 1;
    int moves[maxmoves];
    int len = Bitboards.generatemoves(color, 0, moves);
    int played = -1;
    for (int j = 0; j < len; j++) {
      if (algebraic(moves[j]) == token) {
        played = j;
      }
    }
    if (played >= 0) {
      Bitboards.makemove(moves[played], false);
      if (gosent) {
        searchlimits.softnodelimit = 0;
        searchlimits.hardnodelimit = 0;
        runsearch(true);
      }
    }
  }
}
