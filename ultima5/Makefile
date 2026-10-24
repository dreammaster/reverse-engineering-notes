# MinGW build of the Ultima V decompilation (src/).
#
#   mingw32-make                    build build/u5.exe (native Win32 backend, no SDL needed)
#   mingw32-make run                (re)create run/ from $(GAME_DIR) and launch the game from there
#   mingw32-make rundir             only (re)create run/
#   mingw32-make test               headless smoke tests (boot, 'Journey Onward' path; screenshots in run/)
#   mingw32-make unit               unit tests for the C ports of assembly routines (tests/)
#   mingw32-make clean
#
# Variables:
#   GAME_DIR=C:/games/ultima5       original game files (never modified; run/ holds the working copy and saves)
#   BACKEND=win32|sdl               win32 (default) = GDI window, no audio.  sdl needs SDL3 + SDL3_mixer
#                                   (SDL3_CFLAGS / SDL3_LIBS are taken from pkg-config if not given)
#   DEBUG=1                         -O0 -g, runtime asserts
#   WARN=1                          show compiler warnings (the decompiled code is very noisy; off by default)

ifeq ($(origin CC),default)
CC := gcc
endif
PYTHON ?= python
U5TOOL := $(PYTHON) tools/u5tool.py
GAME_DIR ?= C:/games/ultima5
BACKEND ?= win32
BUILD   := build
SRC     := src
TARGET  := $(BUILD)/u5.exe
RUN_DIR := run

COMMON_SRCS := \
	town mainout dungeon intro npc combat blckthrn lookobj dnglook outsubs shoppes endgame sjog cmds cast talk cast2 \
	zstats comsubs shoppes2 shoppes3 font vars ultima 0000 1000 2000 3000 4000 5000 6000 7000 savegame grap_drv \
	audio/audio audio/aud_mus audio/aud_sfx backend/backend common/file common/util common/settings common/debug \
	common/lzw common/memory graphics/animate graphics/image graphics/grap_buf graphics/origin graphics/grap \
	graphics/reveal graphics/wd

ifeq ($(BACKEND),win32)
BACKEND_SRCS := graphics/grap_win32 event/evt_win32 key/key_win32 time/time_win32
BACKEND_CFLAGS := -DU5_BACKEND_WIN32
BACKEND_LIBS := -lgdi32 -luser32 -ldbghelp
else
BACKEND_SRCS := audio/aud_sdl graphics/grap_sdl event/evt_sdl key/key_sdl time/time_sdl
SDL3_CFLAGS ?= $(shell pkg-config --cflags sdl3 sdl3-mixer)
SDL3_LIBS   ?= $(shell pkg-config --libs sdl3 sdl3-mixer)
BACKEND_CFLAGS := $(SDL3_CFLAGS)
BACKEND_LIBS := $(SDL3_LIBS) -ldbghelp
endif

SRCS := $(COMMON_SRCS) $(BACKEND_SRCS)
OBJS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SRCS)))
DEPS := $(OBJS:.o=.d)

# -std=gnu11: the code predates C23 (bool/true/false are typedef'd/defined in common.h)
# -fno-strict-aliasing, -fwrapv: the decompiled code reinterprets memory and relies on 16/32-bit wraparound
CFLAGS := -std=gnu11 -I$(SRC) -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -MMD -MP $(BACKEND_CFLAGS)
ifeq ($(DEBUG),1)
CFLAGS += -O0 -g
else
CFLAGS += -O2
endif
ifneq ($(WARN),1)
CFLAGS += -w
else
CFLAGS += -Wall -Wno-unused
endif

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(BACKEND_LIBS)

$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)/.dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/.dirs:
	$(U5TOOL) mkdir $(sort $(dir $(OBJS)))
	$(U5TOOL) mkdir $(BUILD)
	echo ok> $@

# ---- running ---------------------------------------------------------------------------------------------
# The game expects its data files in the current directory and its saves in SAVEGAME/ (see FILE_ReadFile).
rundir:
	$(U5TOOL) rundir "$(GAME_DIR)" $(RUN_DIR)

run: $(TARGET) rundir
	$(U5TOOL) exec $(RUN_DIR) $(TARGET)

# Headless smoke tests (see tools/smoke_test.py): boot to the Origin logo, reach the main menu, ...
test: $(TARGET)
	$(PYTHON) tools/smoke_test.py --game-dir "$(GAME_DIR)"

# Unit tests: C ports of assembly routines (tests/), linked against the engine objects (minus main)
UNIT := $(BUILD)/unit_tests.exe
TEST_OBJS := $(filter-out $(BUILD)/ultima.o,$(OBJS))

$(BUILD)/tests/%.o: tests/%.c | $(BUILD)/.dirs
	@$(U5TOOL) mkdir $(BUILD)/tests
	$(CC) $(CFLAGS) -c $< -o $@

$(UNIT): $(BUILD)/tests/test_asm_ports.o $(TEST_OBJS)
	$(CC) -o $@ $^ $(BACKEND_LIBS)

unit: $(UNIT)
	$(UNIT)

clean:
	$(U5TOOL) rmtree $(BUILD)

-include $(DEPS)

.PHONY: all run rundir test unit clean
