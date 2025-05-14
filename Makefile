EXE := Prolix
EVALFILE := shatranj-net23.nnue

SOURCES := Prolix.cpp uci.cpp xboard.cpp search.cpp nnue.cpp tt.cpp history.cpp viriformat.cpp board.cpp external/Fathom/tbprobe.cpp

CXX := clang++

CXXFLAGS := -O3 -march=native -static -pthread -DEUNNfile=\"$(EVALFILE)\"

DEBUGFLAGS := -g -march=native -static -pthread -DEUNNfile=\"$(EVALFILE)\"

SUFFIX :=

ifeq ($(OS), Windows_NT)
	SUFFIX := .exe
endif

OUT := $(EXE)$(SUFFIX)


$(EXE): $(SOURCES)
	$(CXX) $^ $(CXXFLAGS) -o $(OUT)

debug: $(SOURCES)
	$(CXX) $^ $(DEBUGFLAGS) -o debug$(SUFFIX)