EXE := Prolix
EVALFILE := shatranj-net60.nnue
KPFILE := shatranj-kp1.bin
PPFILE := shatranj-pp1.bin
PPXKFILE := shatranj-ppxk1.bin
EVAL_EXISTS := $(wildcard $(EVALFILE))
KP_EXISTS := $(wildcard $(KPFILE))
PP_EXISTS := $(wildcard $(PPFILE))
PPXK_EXISTS := $(wildcard $(PPXKFILE))
ARCH := native
TUNE := native
DEBUG := no
BUILD_DIR := build

# Build date (UTC, not local) baked into the engine name for UCI and xboard.
# CI overrides this so the embedded date, the exe name and the release title all
# come from one timestamp instead of three separate clock reads.
VERSION ?= $(shell date -u '+%m/%d/%y')

# Where `make net` pulls weight files from.  Single, permanently-edited release
# acting as an asset bucket; add a new net with
#   gh release upload nets shatranj-netNN.nnue
NET_BASE_URL ?= https://github.com/sscg13/Prolix/releases/download/nets

rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

C_SRCS := $(call rwildcard,src,*.c)
CPP_SRCS := $(call rwildcard,src,*.cpp)

CPP_OBJS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SRCS))
C_OBJS := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))
OBJS := $(CPP_OBJS) $(C_OBJS)

CXX := clang++
CC := clang

ifeq ($(CXX), g++)
	CC := gcc
endif

ifeq ($(DEBUG), no)
	CXXFLAGS := -O3 -march=$(ARCH) -mtune=$(TUNE) -std=c++17 -static -pthread -DEUNNfile=\"$(EVALFILE)\" -DKPfile=\"$(KPFILE)\" -DPPfile=\"$(PPFILE)\" -DPPXKfile=\"$(PPXKFILE)\" -DPROLIX_VERSION=\"$(VERSION)\"
	CFLAGS := -O3 -march=$(ARCH) -mtune=$(TUNE)
else
	CXXFLAGS := -g -march=$(ARCH) -mtune=$(TUNE) -std=c++17 -static -pthread -DEUNNfile=\"$(EVALFILE)\" -DKPfile=\"$(KPFILE)\" -DPPfile=\"$(PPFILE)\" -DPPXKfile=\"$(PPXKFILE)\" -DPROLIX_VERSION=\"$(VERSION)\"
	CFLAGS := -g -march=$(ARCH) -mtune=$(TUNE)
endif

CXXFLAGS += -MMD -MP
CFLAGS += -MMD -MP

DEPS := $(OBJS:.o=.d)

# The HAS_*FILE gates below silently downgrade the engine when a weight file is
# absent (see resolveevallevel in src/eval.cpp -- no EVALFILE means topevallevel
# drops from 8 to 5).  That is the right default for local hacking and dead
# wrong for a release, so STRICT_NETS=yes turns a missing net into a hard error.
ifeq ($(STRICT_NETS), yes)
ifeq ($(EVAL_EXISTS),)
$(error STRICT_NETS=yes but EVALFILE '$(EVALFILE)' is missing -- run `make net` first)
endif
ifeq ($(KP_EXISTS),)
$(error STRICT_NETS=yes but KPFILE '$(KPFILE)' is missing -- run `make net` first)
endif
endif

ifneq ($(EVAL_EXISTS),)
	CXXFLAGS += -DHAS_EVALFILE
endif
ifneq ($(KP_EXISTS),)
	CXXFLAGS += -DHAS_KPFILE
endif
ifneq ($(PP_EXISTS),)
	CXXFLAGS += -DHAS_PPFILE
endif
ifneq ($(PPXK_EXISTS),)
	CXXFLAGS += -DHAS_PPXKFILE
endif

LDFLAGS :=
SUFFIX :=

ifeq ($(OS), Windows_NT)
	SUFFIX := .exe
endif

OUT := $(EXE)$(SUFFIX)

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(OUT) $^
	@echo "Build complete. Run with ./$(EXE)"

$(BUILD_DIR)/eval/nnue/nnue.o: src/eval/nnue/nnue.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -mno-avxvnni -c $< -o $@

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(DEPS)

# Weight files are gitignored and live on the `nets` release.  EVALFILE and
# KPFILE are required; PPFILE and PPXKFILE are experimental and simply skipped
# when the release has no such asset.
#
# The HAS_*FILE wildcards above are expanded when make parses this file, so
# `make net` has to be its own invocation:  make net && make
net:
	@bash scripts/fetch-nets.sh "$(NET_BASE_URL)" required $(EVALFILE) $(KPFILE)
	@bash scripts/fetch-nets.sh "$(NET_BASE_URL)" optional $(PPFILE) $(PPXKFILE)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: net clean
