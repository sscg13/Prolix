#include "board.h"
struct Marlinboard {
    U64 occupancy;
	U64 pieces[2];
	U64 data;
	//8 bit epsquare
	//8 bit halfmoveclock
	//16 bit fullmove
	//16 bit eval (signed)
	//8 bit wdl
	//8 bit extra
    void pack(const Board &Bitboards, int score);
};
void Marlinboard::pack(const Board &Bitboards, int score) {
	U64 occupied = (Bitboards.Bitboards[0] | Bitboards.Bitboards[1]);
	occupancy = occupied;
	pieces[0] = 0ULL;
	pieces[1] = 0ULL;
	data = 0ULL;
	int i = 0;
	int convert[6] = {0, 2, 4, 1, 3, 5};
	while (occupied) {
		int square = __builtin_ctzll(occupied);
		bool color = ((1ULL << square) & Bitboards.Bitboards[1]);
		int piece = 0;
		for (int j = 1; j < 6; j++) {
			if ((1ULL << square) & Bitboards.Bitboards[j+2]) {
				piece = j;
			}
		}
		if (i > 15) {
			pieces[1] |= (((U64)color) << (4*i-61));
			pieces[1] |= (((U64)piece) << (4*i-64));
		}
		else {
			pieces[1] |= (((U64)color) << (4*i+3));
			pieces[1] |= (((U64)piece) << (4*i));
		}
		occupied ^= (1ULL << square);
		i++;
	}
	int color = Bitboards.position & 1;
	data |= (((U64)color) << 7);
	data |= (((U64)(Bitboards.position >> 1)) << 8);
	data |= (((U64)(unsigned short int)score) << 32);
}
class Viriformat {
	Marlinboard initialpos;
	std::vector<unsigned int> moves;
	public:
	void initialize(const Board &Bitboards);
	void push(int move, int score);
	void writewithwdl(std::ofstream &output, int wdl);
};

void Viriformat::initialize(const Board &Bitboards) {
	initialpos.pack(Bitboards, 0);
	moves.clear();
}

void Viriformat::push(int move, int score) {
	unsigned int data = (move & 4095) << 16;
	if (move & (1 << 20)) {
		data |= 0xC0000000;
	}
	data |= (short int)score;
	moves.push_back(data);
}

void Viriformat::writewithwdl(std::ofstream &output, int wdl) {
	initialpos.data |= (((U64)wdl) << 48);
	moves.push_back(0);
	output.write(reinterpret_cast<const char *>(&initialpos), 32);
	output.write(reinterpret_cast<const char *>(moves.data()), 4*moves.size());
}