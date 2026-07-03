EXE := Prolix
EVALFILE := shatranj-net59.nnue
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
	CXXFLAGS := -O3 -march=$(ARCH) -mtune=$(TUNE) -std=c++17 -static -pthread -DEUNNfile=\"$(EVALFILE)\" -DKPfile=\"$(KPFILE)\" -DPPfile=\"$(PPFILE)\" -DPPXKfile=\"$(PPXKFILE)\"
	CFLAGS := -O3 -march=$(ARCH) -mtune=$(TUNE)
else
	CXXFLAGS := -g -march=$(ARCH) -mtune=$(TUNE) -std=c++17 -static -pthread -DEUNNfile=\"$(EVALFILE)\" -DKPfile=\"$(KPFILE)\" -DPPfile=\"$(PPFILE)\" -DPPXKfile=\"$(PPXKFILE)\"
	CFLAGS := -g -march=$(ARCH) -mtune=$(TUNE)
endif

CXXFLAGS += -MMD -MP
CFLAGS += -MMD -MP

DEPS := $(OBJS:.o=.d)

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

clean:
	rm -rf $(BUILD_DIR)