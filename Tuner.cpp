#include <fstream>
#include <iostream>
#include <cmath>
using U64 = uint64_t;
using namespace std;

double materialm[6] = {78, 77, 144, 415, 657, 20000};
double materiale[6] = {85, 113, 139, 402, 905, 20000};
double materialmgrad[6] = {0,0,0,0,0,0};
double materialegrad[6] = {0,0,0,0,0,0};
double pstm[6][64] = {{0,0,0,0,0,0,0,0,-61,-2,14,19,-9,-5,-7,-35,-54,24,12,23,8,3,-3,-34,-28,-12,18,48,18,9,-2,-35,0,-17,13,53,-7,4,-19,-34,-14,-4,4,-2,8,-26,-16,-23,9,18,20,28,29,14,9,-2,0,0,0,0,0,0,0,0},
{-41,-23,-7,-17,-19,-1,-25,-45,-38,-36,-4,-7,-5,1,-32,-37,11,-15,9,-12,-11,6,-17,-0,-17,-12,13,26,23,18,-13,-18,-19,-2,-14,24,23,-6,-36,-20,-25,-8,12,19,17,19,-9,-23,-27,-21,7,-7,-8,2,-28,-42,-39,-33,-17,-11,-16,-22,-31,-38},
{-39,-23,-30,-22,3,-21,-35,-34,-29,-21,-9,-1,-19,-11,-21,-33,-30,-13,15,2,16,6,2,-22,-23,-11,5,34,6,12,-2,-17,-17,-8,13,5,50,9,2,-19,-5,-6,-5,11,-1,17,3,-9,-27,-17,-4,-1,-4,-2,-11,-28,-39,-27,-24,-16,-12,-26,-24,-39},
{-42,16,-9,-15,2,-12,4,-35,-27,-6,-1,16,8,-5,-7,-27,11,14,25,26,1,18,-13,-9,0,-2,20,37,22,16,12,-8,5,37,48,28,22,38,12,-15,-23,1,11,35,30,33,5,-22,-32,-32,0,15,3,4,-18,-18,-54,-25,-16,-1,4,-1,-14,-51},
{-28,-33,-2,2,-2,-12,-5,-14,-45,-4,-2,-20,1,-5,-16,-12,-22,-19,-15,-2,-13,-16,-14,-28,-24,-11,-6,-10,-6,-4,-12,-4,-0,-8,5,-4,-4,8,-9,7,2,0,6,14,1,12,5,9,17,17,25,20,25,29,21,19,10,13,13,19,16,21,9,18},
{-21,-7,5,14,-16,-5,-11,-19,2,31,23,7,13,-10,-2,-2,-12,7,1,22,-1,13,3,-8,-19,-16,-14,-17,-21,-13,-8,-11,-18,-13,-17,-33,-25,-22,-15,-17,-15,-16,-12,-18,-33,-18,-21,-19,-17,-15,-18,-25,-23,-21,-22,-20,-17,-20,-19,-26,-22,-16,-18,-14}};
double pste[6][64] = {{0,0,0,0,0,0,0,0,6,-12,-1,-24,-38,8,-4,-24,-25,-13,4,12,-17,-5,3,-7,-22,-13,20,0,18,-27,-12,-4,-26,2,-5,-14,10,4,-2,-15,1,7,6,9,20,-20,4,-25,18,22,22,21,42,10,16,7,0,0,0,0,0,0,0,0},
{-41,-30,3,-17,-19,-19,-24,-45,-38,-36,-4,-7,-5,1,-32,-37,-1,-15,9,2,12,6,-17,-29,-17,-12,13,26,23,18,-13,-18,-19,-11,19,24,23,8,16,-20,-25,-8,12,19,17,19,-9,-23,-29,-21,7,-6,11,2,-28,-31,-39,-33,-17,-11,-16,-22,-31,-38},
{-41,-24,-24,-22,-16,-21,-35,-34,-29,-19,-9,-13,-19,-7,-20,-37,-32,-14,19,7,25,12,-9,-22,-23,-14,10,20,28,3,-2,-19,-19,-7,21,29,14,19,-0,-20,-8,10,10,5,12,4,6,-20,-32,-13,-6,3,-17,6,-23,-26,-47,-29,-21,-25,-13,-33,-24,-43},
{-43,-11,-10,-3,-3,-14,-18,-37,-28,-12,-8,-1,9,2,-11,-33,-20,-10,17,5,9,24,-7,-9,-11,-5,20,28,31,16,11,-19,-6,4,33,48,55,18,32,2,-24,2,11,32,29,30,7,-18,-35,-7,0,17,9,11,-6,-25,-49,-26,-17,-2,3,3,-10,-46},
{-31,-18,-5,-2,-13,-4,-13,-26,-27,-9,-6,1,1,4,-11,1,-12,-10,-11,-2,-13,6,-0,10,-5,-5,-2,-4,-7,-2,1,17,-1,-4,-2,-3,-2,3,0,4,1,-3,2,2,2,7,6,9,9,9,11,9,5,8,11,13,23,17,17,17,15,17,18,27},
{-38,-37,-45,-25,-20,-28,-36,-43,-30,-32,-22,-20,-9,-13,-16,-31,-25,-3,2,8,6,1,6,-21,-27,-2,23,37,26,20,0,-9,-3,15,28,25,38,13,6,-11,-5,9,11,24,1,16,-8,-12,-20,-9,-6,-6,-3,-1,-5,-23,-30,-26,-17,-13,-17,-14,-20,-31}};
double pstmgrad[6][64];
double pstegrad[6][64];
double pstmadagrad[6][64];
double psteadagrad[6][64];
double phase[6] = {0, 1, 2, 4, 6, 0};
int gamephase[2] = {0, 0};
double k = 0.002;
double evalm[2] = {0, 0};
double evale[2] = {0, 0};
int board[64];
int halfmove = 0;
int color;
double lr = 2;
ifstream input;
ofstream output;
string inputfile;
void initialize() {
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 64; j++) {
            pstmadagrad[6][64] = 0;
            psteadagrad[6][64] = 0;
        }
    }
}
void write_values(string outputfile) {
    output.open(outputfile, ofstream::app);
    output << "Material mg: \n{" << round(materialm[0]);
    cout << "Material mg: \n{" << materialm[0];
    for (int i = 1; i < 6; i++) {
        output << ", " << round(materialm[i]);
        cout << ", " << materialm[i];
    }
    output << "}\nMaterial eg: \n{" << round(materiale[0]);
    cout << "}\nMaterial eg: \n{" << materiale[0];
    for (int i = 1; i < 6; i++) {
        output << ", " << round(materiale[i]);
        cout << ", " << materiale[i];
    }
    output << "}\nPST mg: \n{{" << round(pstm[0][0]);
    cout << "}\nPST mg: \n\n{{" << pstm[0][0];
    for (int i = 1; i < 64; i++) {
        output << "," << round(pstm[0][i]);
        cout << ", ";
        if (i%8 == 0) {
            cout << "\n\n";
        }
        cout << pstm[0][i];
    }
    for (int i = 1; i < 6; i++) {
        output << "},\n{" << round(pstm[i][0]);
        cout << "},\n\n{" << pstm[i][0];
        for (int j = 1; j < 64; j++) {
            output << "," << round(pstm[i][j]);
            cout << ", ";
            if (j%8 == 0) {
                cout << "\n\n";
            }
            cout << pstm[i][j];
        }
    }
    output << "}}\nPST eg: \n{{" << round(pste[0][0]);
    cout << "}}\n\nPST eg: \n\n{{" << pste[0][0];
    for (int i = 1; i < 64; i++) {
        output << "," << round(pste[0][i]);
        cout << ", ";
        if (i%8 == 0) {
            cout << "\n\n";
        }
        cout << pste[0][i];
    }
    for (int i = 1; i < 6; i++) {
        output << "},\n{" << round(pste[i][0]);
        cout << "},\n\n{" << pste[i][0];
        for (int j = 1; j < 64; j++) {
            output << "," << round(pste[i][j]);
            cout << ", ";
            if (j%8 == 0) {
                cout << "\n\n";
            }
            cout << pste[i][j];
        }
    }
    output << "}}\n";
    cout << "}}\n";
    output.close();
}
void resetgradients() {
    for (int i = 0; i < 6; i++) {
        materialmgrad[i] = 0;
        materialegrad[i] = 0;
        for (int j = 0; j < 64; j++) {
            pstmgrad[i][j] = 0;
            pstegrad[i][j] = 0;
        }
    }
}
void parseFEN(string FEN) {
    gamephase[0] = 0;
    gamephase[1] = 0;
    evalm[0] = 0;
    evalm[1] = 0;
    evale[0] = 0;
    evale[1] = 0;
    int order[64] = {56, 57, 58, 59, 60, 61, 62, 63, 48, 49, 50, 51, 52, 53, 54, 55, 40, 41, 42, 43, 44, 45, 46, 47,
    32, 33, 34, 35, 36, 37, 38, 39, 24, 25, 26, 27, 28, 29, 30, 31, 16, 17, 18, 19, 20, 21, 22, 23,
    8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7};
    char file[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    int progress = 0;
    int tracker = 0;
    int castling = 0;
    int color = 0;
    while (FEN[tracker]!=' ') {
        char hm = FEN[tracker];
        if ('0' <= hm && hm <= '9') {
            int repeat = (int)hm-48;
            for (int i = 0; i < repeat; i++) {
                board[order[progress]] = 0;
                progress++;
            }
        }
        if ('A'<= hm && hm <= 'Z') {
            if (hm == 'P') {
                board[order[progress]] = 1;
                evalm[0]+=materialm[0];
                evale[0]+=materiale[0];
                evalm[0]+=pstm[0][order[progress]];
                evale[0]+=pste[0][order[progress]];
            }
            if (hm == 'A' || hm == 'B') {
                board[order[progress]] = 2;
                evalm[0]+=materialm[1];
                evale[0]+=materiale[1];
                evalm[0]+=pstm[1][order[progress]];
                evale[0]+=pste[1][order[progress]];
                gamephase[0]+=1;
            }
            if (hm == 'F' || hm == 'Q') {
                board[order[progress]] = 3;
                evalm[0]+=materialm[2];
                evale[0]+=materiale[2];
                evalm[0]+=pstm[2][order[progress]];
                evale[0]+=pste[2][order[progress]];
                gamephase[0]+=2;
            }
            if (hm == 'N') {
                board[order[progress]] = 4;
                evalm[0]+=materialm[3];
                evale[0]+=materiale[3];
                evalm[0]+=pstm[3][order[progress]];
                evale[0]+=pste[3][order[progress]];
                gamephase[0]+=4;
            }
            if (hm == 'R') {
                board[order[progress]] = 5;
                evalm[0]+=materialm[4];
                evale[0]+=materiale[4];
                evalm[0]+=pstm[4][order[progress]];
                evale[0]+=pste[4][order[progress]];
                gamephase[0]+=6;
            }
            if (hm == 'K') {
                board[order[progress]] = 6;
                evalm[0]+=pstm[5][order[progress]];
                evale[0]+=pste[5][order[progress]];
            }
            progress++;
        }
        if ('a'<= hm && hm <= 'z') {
            if (hm == 'p') {
                board[order[progress]] = -1;
                evalm[1]+=materialm[0];
                evale[1]+=materiale[0];
                evalm[1]+=pstm[0][56^order[progress]];
                evale[1]+=pste[0][56^order[progress]];
            }
            if (hm == 'a' || hm == 'b') {
                board[order[progress]] = -2;
                evalm[1]+=materialm[1];
                evale[1]+=materiale[1];
                evalm[1]+=pstm[1][56^order[progress]];
                evale[1]+=pste[1][56^order[progress]];
                gamephase[1]+=1;
            }
            if (hm == 'f' || hm == 'q') {
                board[order[progress]] = -3;
                evalm[1]+=materialm[2];
                evale[1]+=materiale[2];
                evalm[1]+=pstm[2][56^order[progress]];
                evale[1]+=pste[2][56^order[progress]];
                gamephase[1]+=2;
            }
            if (hm == 'n') {
                board[order[progress]] = -4;
                evalm[1]+=materialm[3];
                evale[1]+=materiale[3];
                evalm[1]+=pstm[3][56^order[progress]];
                evale[1]+=pste[3][56^order[progress]];
                gamephase[1]+=4;
            }
            if (hm == 'r') {
                board[order[progress]] = -5;
                evalm[1]+=materialm[4];
                evale[1]+=materiale[4];
                evalm[1]+=pstm[4][56^order[progress]];
                evale[1]+=pste[4][56^order[progress]];
                gamephase[1]+=6;
            }
            if (hm == 'k') {
                board[order[progress]] = -6;
                evalm[1]+=pstm[5][56^order[progress]];
                evale[1]+=pste[5][56^order[progress]];
            }
            progress++;
        }
        tracker++;
    }
    while (FEN[tracker] == ' ') {
        tracker++;
    }
    if (FEN[tracker] == 'b') {
        color = 1;
    }
    tracker+=6;
    int halfmove = (int)(FEN[tracker])-48;
    tracker++;
    if (FEN[tracker]!=' ') {
        halfmove = 10*halfmove+(int)(FEN[tracker])-48;
    }
}
double get_original_eval() {
    int midphase = min(48, gamephase[0]+gamephase[1]);
    int endphase = 48-midphase;
    double mideval = evalm[0]-evalm[1];
    double endeval = evale[0]-evale[1];
    int progress = 200-halfmove;
    double base = (mideval*midphase+endeval*endphase)/48;
    double eval = (base*progress)/200;
    return eval;
}
double get_total_error() {
    input.open(inputfile, ifstream::app);
    int total = 0;
    double totalerror = 0;
    while (input.good()) {
        string fen;
        getline(input, fen);
        parseFEN(fen.substr(2, fen.length()-3));
        double result = 0.5;
        if (fen[0] == 'w') {
            result = 1;
        }
        if (fen[0] == 'b') {
            result = 0;
        }
        totalerror+=pow(result-1.0/(1.0+exp(-k*get_original_eval())), 2);
        total++;
    }
    input.close();
    cout << total << " " << totalerror << " " << k << "\n";
    return totalerror/total;
}
double optimize_k() {
    double low = 0.0;
    double high = 0.1;
    double step = (high-low)/10;
    double best;
    double bestk;
    for (int i = 0; i < 7; i++) {
        best = 1;
        bestk = low;
        k = low;
        while (k <= high) {
            double error = get_total_error();
            if (error < best) {
                bestk = k;
                best = error;
            }
            k += step;
        }
        low = bestk-step;
        high = bestk+step;
        step = step/10.0;
    }
    return bestk;
}
void compute_gradients() {
    input.open(inputfile, ifstream::app);
    int total = 0;
    double totalerror = 0;
    while (input.good()) {
        string fen;
        getline(input, fen);
        parseFEN(fen.substr(2, fen.length()-3));
        double result = 0.5;
        if (fen[0] == 'w') {
            result = 1;
        }
        if (fen[0] == 'b') {
            result = 0;
        }
        double eval = get_original_eval();
        double num = k*exp(k*eval)*((result-1)*exp(k*eval)+result);
        double denom = pow(exp(k*eval)+1, 3);
        double dsigmoid = -num/denom;
        double midphase = min(1.0, (gamephase[0]+gamephase[1])/48.0);
        double endphase = 1.0-midphase;
        for (int i = 0; i < 64; i++) {
            if (board[i] > 0) {
                materialmgrad[board[i]-1] += midphase*dsigmoid;
                materialegrad[board[i]-1] += endphase*dsigmoid;
                pstmgrad[board[i]-1][i] += midphase*dsigmoid;
                pstegrad[board[i]-1][i] += endphase*dsigmoid;
            }
            if (board[i] < 0) {
                materialmgrad[-board[i]-1] -= midphase*dsigmoid;
                materialegrad[-board[i]-1] -= endphase*dsigmoid;
                pstmgrad[-board[i]-1][56^i] -= midphase*dsigmoid;
                pstegrad[-board[i]-1][56^i] -= endphase*dsigmoid;
            }
        }
        total++;
    }
    input.close();
    /*for (int i = 0; i < 6; i++) {
        materialmgrad[i] /= total;
        materialegrad[i] /= total;
    }*/
}
void update_gradients() {
    for (int i = 0; i < 6; i++) {
        materialm[i] -= lr*materialmgrad[i];
        materiale[i] -= lr*materialegrad[i];
        for (int j = 0; j < 64; j++) {
            pstmadagrad[i][j] += pow(pstmgrad[i][j], 2);
            psteadagrad[i][j] += pow(pstegrad[i][j], 2);
            pstm[i][j] -= lr*pstmgrad[i][j]/(0.00000001 + pow(pstmadagrad[i][j], 0.5));
            pste[i][j] -= lr*pstegrad[i][j]/(0.00000001 + pow(psteadagrad[i][j], 0.5));
        }
    }
}
void train(int epochs) {
    resetgradients();
    for (int i = 0; i < epochs; i++) {
        compute_gradients();
        update_gradients();
        resetgradients();
        if ((i%16) == 0) {
            get_total_error();
        }
    }
}
void parseui() {
    string command;
    getline(cin, command);
    if (command.substr(0, 5) == "input") {
        inputfile = command.substr(6, command.length()-6);
    }
    if (command.substr(0, 5) == "train") {
        int sum = 0;
        int add = 1;
        int reader = command.length()-1;
        while (command[reader] != ' ') {
            sum+=((int)(command[reader]-48))*add;
            add*=10;
            reader--;
        }
        train(sum);
    }
    if (command == "optimize") {
        optimize_k();
    }
    if (command.substr(0, 5) == "print") {
        string outputfile = command.substr(6, command.length()-6);
        write_values(outputfile);
    }
    if (command.substr(0, 5) == "setlr") {
        int value = (int)(command[command.length()-1]-48);
        lr = (double)value/1.0;
    }
    if (command.substr(0, 5) == "divlr") {
        int value = (int)(command[command.length()-1]-48);
        for (int i = 0; i < value; i++) {
            lr = lr/10.0;
        }
    }
}
int main() {
    initialize();
    //train(250);
    //write_values("tuned_values.txt");
    //optimize_k();
    //get_total_error();
    while (true) {
        parseui();
    }
    return 0;
}

