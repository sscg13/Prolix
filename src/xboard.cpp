#include "engine.h"

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
    EUNN->initializennue(Bitboards.Bitboards);
    gosent = false;
  }
  if (xcommand.substr(0, 8) == "setboard") {
    std::string fen = xcommand.substr(9, xcommand.length() - 9);
    Bitboards.parseFEN(fen);
    EUNN->initializennue(Bitboards.Bitboards);
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
    int moves[maxmoves];
    int len = Bitboards.generatemoves(color, 0, moves);
    int played = -1;
    for (int j = 0; j < len; j++) {
      if (algebraic(moves[j]) == xcommand) {
        played = j;
      }
    }
    if (played >= 0) {
      Bitboards.makemove(moves[played], false);
      if (useNNUE) {
        EUNN->initializennue(Bitboards.Bitboards);
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