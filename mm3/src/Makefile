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
	rm -rf build tests/test_cc tests/test_real tests/test_state tests/test_mapbin tests/test_evtrun tests/test_walk tests/view_run tests/ui_run ccdump gfxsum viewer
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

viewer: viewer.c $(CORE) ui_text.c gfx.h cc.h ui_text.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -o $@ viewer.c $(CORE) ui_text.c $(SDL_LIBS) -lm
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
VIEW_SRC = recomp.c view_host.c gen/view_gen.c
VIEW_FUNCS = prepareIndoorView renderIndoorView drawWallFaces drawRow1Sprites drawRow2Sprites drawRow3Sprites drawRow4Sprites \
  drawHudPieces scanObjectsAhead scanMonstersAhead clearViewFlags updateLight mazeGetWordRel mazeGetFlagsRel mazeNeighbourSlot \
  checkDrawListOverflow checkSkill drawViewOutdoors renderOutdoorView mazeUpdateSlot

# regenerate gen/view_gen.c from the disassembly (only needed when mm3.asm / the translator change)
regen:
	python3 ../tools/asm2c.py ../mm3.asm ../mm3.idc gen/view_gen.c $(VIEW_FUNCS)
.PHONY: regen

tests/view_run: tests/view_run.c $(VIEW_SRC) view_host.h recomp.h
	$(CC) $(CFLAGS) -Wno-unused-label -o $@ tests/view_run.c $(VIEW_SRC)

# compare the translated view code with the original running in the emulator (needs `pip install unicorn`)
viewcheck: tests/view_run mm3view
	python3 ../tools/mm3_viewcheck.py ../data/MM3.EXE $(DATA)/MM3.CUR tests/view_run 300
	python3 ../tools/mm3_viewcheck.py ../data/MM3.EXE $(DATA)/MM3.CUR tests/view_run 300 5 ./mm3view $(DATA)
.PHONY: viewcheck

GLUE_SRC = view_glue.c cc.c maze.c party.c gfx.c dgroup.c mapbin.c $(VIEW_SRC)

mm3view: mm3view.c $(GLUE_SRC) view_glue.h
	$(CC) $(CFLAGS) -Wno-unused-label $(SDL_CFLAGS) -o $@ mm3view.c $(GLUE_SRC) $(SDL_LIBS) -lm
.PHONY: mm3view

tests/test_walk: tests/test_walk.c $(GLUE_SRC) view_glue.h
	$(CC) $(CFLAGS) -Wno-unused-label -o $@ tests/test_walk.c $(GLUE_SRC)

walktest: tests/test_walk
	tests/test_walk $(DATA)
.PHONY: walktest

# ---- text engine and windows (hand-written port of the video module's text code)
tests/ui_run: tests/ui_run.c ui_text.c ui_text.h font.c cc.c
	$(CC) $(CFLAGS) -o $@ tests/ui_run.c ui_text.c font.c cc.c

# fuzz the C text engine against the original video module in the emulator (needs `pip install unicorn`)
uifuzz: tests/ui_run
	python3 ../tools/mm3_ui_fuzz.py ../data/MM3.EXE $(DATA) tests/ui_run 150 2
	MM3_FUZZ_ITEMS=70 python3 ../tools/mm3_ui_fuzz.py ../data/MM3.EXE $(DATA) tests/ui_run 60 5
.PHONY: uifuzz

# ---- translated game rules (character stats, event conditions); see gen/README.md
RULES_FUNCS = ifProc getMaxSP getMaxHP getArmorClass getStat getAge isBitSet checkSkill conditionMod getCurrentLevel itemScan \
  statBonus getAttributeCategory getElementalCategory
RULES_SRC = recomp.c rules_host.c gen/rules_gen.c

regen_rules:
	python3 ../tools/asm2c.py ../mm3.asm ../mm3.idc gen/rules_gen.c $(RULES_FUNCS)
.PHONY: regen_rules

tests/rules_run: tests/rules_run.c $(RULES_SRC) recomp.h
	$(CC) $(CFLAGS) -Wno-unused-label -o $@ tests/rules_run.c $(RULES_SRC)

rulescheck: tests/rules_run
	python3 ../tools/mm3_rules_check.py ../data/MM3.EXE $(DATA) tests/rules_run 300
.PHONY: rulescheck

# ---- the whole game logic, translated (everything reachable from exploreLoop except the routines in gen/hosts.txt)
regen_game:
	mkdir -p build
	cat gen/hosts.txt gen/readable.txt > build/hosts_all.txt
	python3 ../tools/mm3_reach.py ../mm3.asm exploreLoop,loadMonsterData,loadSavedGame,rosterMenu,introSequence,copyPartyToRoster build/hosts_all.txt > gen/game_funcs.txt
	ASM2C_WEAK_HOSTS=1 python3 ../tools/asm2c.py ../mm3.asm ../mm3.idc gen/game_gen.c $$(cat gen/game_funcs.txt | tr '\n' ' ')
.PHONY: regen_game

# the translated originals of the routines that have readable replacements (gen/readable.txt): the reference for `mm3game --difftest`
regen_ref:
	python3 ../tools/mm3_reach.py ../mm3.asm $$(cat gen/readable.txt | tr '\n' ',' | sed 's/,$$//') gen/hosts.txt > build/ref_funcs.txt
	ASM2C_STATIC_CALLS=1 python3 ../tools/asm2c.py ../mm3.asm ../mm3.idc gen/ref_gen.c $$(cat build/ref_funcs.txt | tr '\n' ' ')
.PHONY: regen_ref

# ---- the recompiled game
GAME_SRC = recomp.c gen/game_gen.c gen/ref_gen.c logic.c game_logic.c game_diff.c rules.c game_rules.c x86.c opl.c game_sound.c game_machine.c game_files.c game_video.c ui_text.c font.c gfx.c cc.c

mm3game: game_main.c $(GAME_SRC) game.h recomp.h
	$(CC) -std=c11 -O1 -w -DRECOMP_STACK_IN_DG $(SDL_CFLAGS) -o $@ game_main.c $(GAME_SRC) $(SDL_LIBS) -lm
.PHONY: mm3game

# ---- readable rules vs the translated originals
tests/test_rules_diff: tests/test_rules_diff.c rules.c $(RULES_SRC) rules.h recomp.h
	$(CC) $(CFLAGS) -Wno-unused-label -o $@ tests/test_rules_diff.c rules.c $(RULES_SRC)

rulesdiff: tests/test_rules_diff
	./tests/test_rules_diff $(DATA)/DGROUP.BIN 3000
.PHONY: rulesdiff
