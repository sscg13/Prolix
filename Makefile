EXE := Prolix
EVALFILE := shatranj-net33.nnue
ARCH := native

SOURCES := src/Prolix.cpp src/uci.cpp src/xboard.cpp src/search.cpp src/datagen/datagen.cpp src/eval/nnue.cpp src/tt.cpp src/history.cpp src/datagen/viriformat.cpp src/board.cpp src/external/Fathom/tbprobe.cpp

CXX := clang++

CXXFLAGS := -O3 -march=$(ARCH) -static -pthread -DEUNNfile=\"$(EVALFILE)\"

DEBUGFLAGS := -g -march=$(ARCH) -static -pthread -DEUNNfile=\"$(EVALFILE)\"

SUFFIX :=

ifeq ($(OS), Windows_NT)
	SUFFIX := .exe
endif

OUT := $(EXE)$(SUFFIX)


$(EXE): $(SOURCES)
	$(CXX) $^ $(CXXFLAGS) -o $(OUT)

debug: $(SOURCES)
	$(CXX) $^ $(DEBUGFLAGS) -o debug$(SUFFIX)