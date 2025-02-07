EXE := Prolix
EVALFILE := shatranj-net15.nnue

SOURCES := Prolix.cpp external/Fathom/tbprobe.cpp

CXX := clang++

CXXFLAGS := -O3 -march=native -static -pthread -DEUNNfile=\"$(EVALFILE)\"

SUFFIX := .exe

OUT := $(EXE)$(SUFFIX)


$(EXE): $(SOURCES)
	$(CXX) $^ $(CXXFLAGS) -o $(OUT)
