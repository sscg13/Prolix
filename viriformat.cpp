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
    void pack(Bitboard Bitboards, int score);
}
void Marlinboard::pack(Bitboard Bitboards, int score) {
	U64 occupied = (Bitboards.Bitboards[0] | Bitboards.Bitboards[1]);
	occupancy = occupied;
	pieces[0] = 0ULL;
	pieces[1] = 0ULL;
	data = 0ULL;
	int i = 0;
	int convert = {0, 2, 4, 1, 3, 5};
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
			pieces[1] |= (((U64)color) << 4*i-61);
			pieces[1] |= (((U64)piece) << 4*i-64);
		}
		else {
			pieces[1] |= (((U64)color) << 4*i+3);
			pieces[1] |= (((U64)piece) << 4*i);
		}
		i++;
	}
	int color = Bitboards.position & 1;
	data |= (((U64)color) << 7);
	data |= (((U64)Bitboards.halfmovecount()) << 8);
	data |= (((U64)(unsigned short int)score) << 32);
}
class Viriformat {
	Marlinboard initialpos;
	std::vector<int> moves;
	public:
	void initialize(Bitboard Bitboards);
	void push(bool filtered, int move, int score);
	void writewithwdl(std::ofstream output, int wdl);

}