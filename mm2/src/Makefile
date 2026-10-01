# Build with MinGW:  C:\mingw32\bin\mingw32-make   (or any make + gcc)
#   make test        build and run the data/render tests (no SDL needed)
#   make mm2         build the SDL2 viewer (needs SDL2 headers/libs: set SDL_CFLAGS / SDL_LIBS)
CC       = gcc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra -pedantic
BUILD   := build
CORE    := mm2_lzw.c mm2_files.c mm2_gfx.c mm2_map.c mm2_view.c mm2_data.c mm2_state.c mm2_events.c mm2_rules.c mm2_tables_gen.c
CORE_O  := $(CORE:%.c=$(BUILD)/%.o)

SDL_CFLAGS ?= -IC:/dev/scummvm_libs_2015/include/SDL2 -Dmain=SDL_main
SDL_LIBS   ?= -lmingw32 -lSDL2main -lSDL2

all: $(BUILD)/test_main.exe

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/test_main.exe: tests/test_main.c $(CORE_O)
	$(CC) $(CFLAGS) $^ -o $@

test: $(BUILD)/test_main.exe
	./$(BUILD)/test_main.exe

# SDL2 viewer (compile check only without the SDL2 runtime libraries)
$(BUILD)/main_sdl.o: main_sdl.c | $(BUILD)
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -c $< -o $@

mm2: $(BUILD)/main_sdl.o $(CORE_O)
	$(CC) $^ -o $(BUILD)/mm2.exe $(SDL_LIBS)

check-sdl: main_sdl.c
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -fsyntax-only main_sdl.c

clean:
	rm -rf $(BUILD)

.PHONY: all test mm2 check-sdl clean
