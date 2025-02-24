EXE := Prolix
EVALFILE := shatranj-net17.nnue

SOURCES := Prolix.cpp uci.cpp xboard.cpp search.cpp nnue.cpp tt.cpp history.cpp viriformat.cpp board.cpp external/Fathom/tbprobe.cpp

CXX := clang++

CXXFLAGS := -O3 -march=native -static -pthread -DEUNNfile=\"$(EVALFILE)\"

SUFFIX := .exe

OUT := $(EXE)$(SUFFIX)


$(EXE): $(SOURCES)
	$(CXX) $^ $(CXXFLAGS) -o $(OUT)
