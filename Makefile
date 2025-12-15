EXE := Prolix
EVALFILE := shatranj-net44.nnue
ARCH := native
TUNE := native

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

C_SRCS := $(call rwildcard,src,*.c)
CPP_SRCS := $(call rwildcard,src,*.cpp)

CPP_OBJS := $(patsubst %.cpp,%.o,$(CPP_SRCS))
C_OBJS := $(patsubst %.c,%.o,$(C_SRCS))
OBJS := $(CPP_OBJS) $(C_OBJS)

CXX := g++
CC := gcc

ifeq ($(CXX), g++)
	CC := gcc
endif

CXXFLAGS := -O3 -march=$(ARCH) -mtune=$(TUNE) -std=c++17 -static -pthread -DEUNNfile=\"$(EVALFILE)\"
CFLAGS := -O3 -march=$(ARCH) -mtune=$(TUNE)
LDFLAGS :=

DEBUGFLAGS := -g -march=$(ARCH) -mtune=$(TUNE) -std=c++17 -static -pthread -DEUNNfile=\"$(EVALFILE)\"

SUFFIX :=

ifeq ($(OS), Windows_NT)
	SUFFIX := .exe
endif

OUT := $(EXE)$(SUFFIX)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(OUT) $^
	@echo "Build complete. Run with ./$(EXE)"

debug: $(SOURCES)
	$(CXX) $^ $(DEBUGFLAGS) -o debug$(SUFFIX)

clean:
	rm -f $(OBJS)