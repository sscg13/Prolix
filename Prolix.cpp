#include "board.h"
#include "engine.h"
#include "external/Fathom/tbprobe.h"
#include "nnue.h"
#include <thread>
extern std::string uciinfostring;
std::string proto = "uci";
std::string inputfile;
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
        std::cerr << "Proper usage: ./(exe) datagen <viriformat|plain> threads "
                     "games outputfile";
        return 0;
      }
      int dataformat = (std::string(argv[2]) == "viriformat");
      std::string extension = dataformat ? ".vf" : ".txt";
      int threads = atoi(argv[3]);
      int games = atoi(argv[4]);
      std::cout << "Generating NNUE data with " << threads << " threads x "
                << games << " games:\n";
      std::vector<std::thread> datagenerators(threads);
      std::vector<Engine> Engines(threads);
      for (int i = 0; i < threads; i++) {
        std::string outputfile =
            std::string(argv[5]) + std::to_string(i) + extension;
        Engines[i].startup();
        datagenerators[i] = std::thread(&Engine::datagen, &Engines[i],
                                        dataformat, games, outputfile);
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
    if (std::string(argv[1]) == "filter") {
      if (argc < 9) {
        std::cerr << "Proper usage: ./(exe) filter threads softnodelimit "
                     "hardnodelimit inputfile outputfile low high";
        return 0;
      }
      int threads = atoi(argv[2]);
      std::string inputfile = std::string(argv[5]);
      std::cout << "Writing to temp files\n";
      std::vector<std::string> tempnames(threads);
      std::vector<std::ofstream> tempfiles(threads);
      for (int i = 0; i < threads; i++) {
        tempnames[i] = "temp" + std::to_string(i) + ".epd";
        tempfiles[i].open(tempnames[i]);
      }
      int poscount = 0;
      std::ifstream epdinput;
      epdinput.open(inputfile);
      while (epdinput) {
        std::string fen;
        getline(epdinput, fen);
        tempfiles[poscount % threads] << fen << "\n";
        poscount++;
      }
      for (int i = 0; i < threads; i++) {
        tempfiles[i].close();
      }
      int softnodes = atoi(argv[3]);
      int hardnodes = atoi(argv[4]);
      int lowerbound = atoi(argv[7]);
      int upperbound = atoi(argv[8]);
      std::cout << "Filtering with lower bound " << lowerbound
                << " and upper bound " << upperbound << "\n";
      std::vector<std::thread> workers(threads);
      std::vector<Engine> Engines(threads);
      for (int i = 0; i < threads; i++) {
        std::string outputfile =
            std::string(argv[6]) + std::to_string(i) + ".epd";
        Engines[i].startup();
        workers[i] =
            std::thread(&Engine::filter, &Engines[i], lowerbound, upperbound,
                        softnodes, hardnodes, tempnames[i], outputfile);
      }
      for (auto &thread : workers) {
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
