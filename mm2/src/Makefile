# Build with MinGW:  mingw32-make from C:/mingw32/bin   (or any make + gcc)
#   make test        build and run the data/render tests (no SDL needed)
#   make mm2         build the SDL2 explorer (SDL_PREFIX points at the SDL2 MinGW package)
CC       = gcc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra -pedantic
BUILD   := build
CORE    := mm2_lzw.c mm2_files.c mm2_gfx.c mm2_game.c mm2_map.c mm2_monpic.c mm2_png.c mm2_text.c mm2_view.c mm2_data.c mm2_state.c mm2_events.c mm2_inn.c mm2_battle.c mm2_combat.c mm2_party.c mm2_reward.c mm2_rules.c mm2_smith.c mm2_spell_names_gen.c mm2_strings.c mm2_spells.c mm2_tavern.c mm2_time.c mm2_town.c mm2_ui.c mm2_tables_gen.c
CORE_O  := $(CORE:%.c=$(BUILD)/%.o)

SDL_PREFIX ?= C:/dev/SDL2/SDL2-2.32.10/i686-w64-mingw32
SDL_CFLAGS ?= -I$(SDL_PREFIX)/include/SDL2
SDL_LIBS   ?= -L$(SDL_PREFIX)/lib -lmingw32 -lSDL2main -lSDL2

all: $(BUILD)/test_main.exe

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/test_main.exe: tests/test_main.c $(CORE_O)
	$(CC) $(CFLAGS) $^ -o $@

test: $(BUILD)/test_main.exe
	./$(BUILD)/test_main.exe

$(BUILD)/mm2_shot.exe: main_shot.c $(CORE_O)
	$(CC) $(CFLAGS) $^ -o $@

shot: $(BUILD)/mm2_shot.exe

# SDL2 viewer (compile check only without the SDL2 runtime libraries)
$(BUILD)/main_sdl.o: main_sdl.c | $(BUILD)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

mm2: $(BUILD)/main_sdl.o $(CORE_O)
	$(CC) $^ -o $(BUILD)/mm2.exe $(SDL_LIBS)
	cp $(SDL_PREFIX)/bin/SDL2.dll $(BUILD)/

check-sdl: main_sdl.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -fsyntax-only main_sdl.c

clean:
	rm -rf $(BUILD)

.PHONY: all test shot mm2 check-sdl clean
