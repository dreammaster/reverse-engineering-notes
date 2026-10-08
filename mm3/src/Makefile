# Might and Magic III C reimplementation -- core (no SDL yet)
CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2

CORE = cc.c maze.c party.c events.c gfx.c dgroup.c mapbin.c evtrun.c font.c

test: tests/test_cc
	python3 tests/gen_testdata.py build/testdata
	tests/test_cc build/testdata

tests/test_cc: tests/test_cc.c $(CORE) cc.h maze.h
	$(CC) $(CFLAGS) -o $@ tests/test_cc.c $(CORE)

clean:
	rm -rf build tests/test_cc tests/test_real tests/test_state tests/test_mapbin tests/test_evtrun tests/view_run ccdump gfxsum viewer
.PHONY: test clean

ccdump: ccdump.c $(CORE) cc.h maze.h
	$(CC) $(CFLAGS) -o $@ ccdump.c $(CORE)

# compare the C extraction with tools/mm3_cc.py on real archives (DATA = folder with MM3.CC / MM3.CUR)
DATA ?= ../data
crosscheck: ccdump
	@for a in MM3.CC MM3.CUR; do \
	  rm -rf build/c build/py; mkdir -p build/c build/py; \
	  ./ccdump $(DATA)/$$a build/c && python3 ../tools/mm3_cc.py extract $(DATA)/$$a build/py && \
	  diff -rq build/c build/py && echo "$$a: identical ($$(ls build/c | wc -l) members)"; \
	done
.PHONY: crosscheck

tests/test_real: tests/test_real.c $(CORE) cc.h maze.h
	$(CC) $(CFLAGS) -o $@ tests/test_real.c $(CORE)

realtest: tests/test_real
	tests/test_real $(DATA)/MM3.CUR
.PHONY: realtest

tests/test_state: tests/test_state.c $(CORE) cc.h maze.h party.h events.h character.h
	$(CC) $(CFLAGS) -o $@ tests/test_state.c $(CORE)

statetest: tests/test_state
	tests/test_state $(DATA)/MM3.CUR
.PHONY: statetest

gfxsum: gfxsum.c $(CORE) gfx.h cc.h
	$(CC) $(CFLAGS) -o $@ gfxsum.c $(CORE)

# decode every graphics member of MM3.CC in C and in Python and compare
gfxcheck: gfxsum
	mkdir -p build
	./gfxsum $(DATA)/MM3.CC > build/gfx_c.txt
	python3 tests/gfxsum.py $(DATA)/MM3.CC > build/gfx_py.txt
	diff build/gfx_c.txt build/gfx_py.txt && echo "graphics: identical ($$(wc -l < build/gfx_c.txt) members)"
.PHONY: gfxcheck

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)

viewer: viewer.c $(CORE) gfx.h cc.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ viewer.c $(CORE) $(SDL_LIBS)
.PHONY: viewer

tests/test_mapbin: tests/test_mapbin.c $(CORE) cc.h mapbin.h dgroup.h party.h
	$(CC) $(CFLAGS) -o $@ tests/test_mapbin.c $(CORE)

# DGROUP.BIN is made from the game executable: python3 ../tools/mm3_dgroup.py MM3.EXE $(DATA)/DGROUP.BIN
mapbintest: tests/test_mapbin
	tests/test_mapbin $(DATA)/MM3.CUR $(DATA)/DGROUP.BIN
.PHONY: mapbintest

tests/test_evtrun: tests/test_evtrun.c $(CORE) evtrun.h events.h
	$(CC) $(CFLAGS) -o $@ tests/test_evtrun.c $(CORE)

evttest: tests/test_evtrun
	tests/test_evtrun $(DATA)/MM3.CUR
.PHONY: evttest

# ---- translated 3D view pipeline (see gen/README.md)
VIEW_SRC = view_host.c gen/view_gen.c
VIEW_FUNCS = prepareIndoorView renderIndoorView drawWallFaces drawRow1Sprites drawRow2Sprites drawRow3Sprites drawRow4Sprites \
  drawHudPieces scanObjectsAhead scanMonstersAhead clearViewFlags updateLight mazeGetWordRel mazeGetFlagsRel mazeNeighbourSlot \
  checkDrawListOverflow checkSkill

# regenerate gen/view_gen.c from the disassembly (only needed when mm3.asm / the translator change)
regen:
	python3 ../tools/asm2c.py ../mm3.asm ../mm3.idc gen/view_gen.c $(VIEW_FUNCS)
.PHONY: regen

tests/view_run: tests/view_run.c $(VIEW_SRC) view_host.h recomp.h
	$(CC) $(CFLAGS) -Wno-unused-label -o $@ tests/view_run.c $(VIEW_SRC)

# compare the translated view code with the original running in the emulator (needs `pip install unicorn`)
viewcheck: tests/view_run
	python3 ../tools/mm3_viewcheck.py ../data/MM3.EXE $(DATA)/MM3.CUR tests/view_run 300
.PHONY: viewcheck

mm3view: mm3view.c view_glue.c cc.c maze.c gfx.c dgroup.c $(VIEW_SRC) view_glue.h
	$(CC) $(CFLAGS) -Wno-unused-label $(SDL_CFLAGS) -o $@ mm3view.c view_glue.c cc.c maze.c gfx.c dgroup.c $(VIEW_SRC) $(SDL_LIBS)
.PHONY: mm3view
