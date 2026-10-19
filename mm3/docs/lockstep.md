# Lockstep comparison: the recompiled game against the original machine code

The recompiled game (`src/`) is the original program translated to C (`tools/asm2c.py`, `src/gen/game_gen.c`) plus
hand-written replacements for everything the original did through DOS, the BIOS, the C library, the video/sound modules and
the routines that were rewritten as readable C.  Translation and replacement errors (a wrong address, a wrong return
convention, a different stack depth ...) show up as odd gameplay.  The lockstep tools find them automatically by running the
**original machine code** on the same inputs and comparing.

## How it works

```
cd mm3/src
make mm3game_tape                                   # the game built with the recorder (src/tape.c)
MM3_TAPE=build/run.tape SDL_VIDEODRIVER=dummy ./mm3game_tape ../data --headless --keys 4800,4800,4B00
python3 ../tools/mm3_lockstep.py build/run.tape     # replay against the original code (Unicorn, 16-bit)
```

1. **Record.**  Every call from translated code into a hand-written host routine is logged: name, arguments, the registers,
   the stack pointer, and *every byte of memory that changed since the previous event* (the translated code's work in between,
   and later the host's own effect).  The stack pointer at the entry of every translated routine is logged too, and the top-level
   routines `game_main.c` calls (`T`/`U` events).  The tape starts with the full initial memory image.
2. **Replay.**  `tools/mm3_lockstep.py` loads that image into an emulator and starts the original code at the same top-level
   routine with the same registers.  The original runs on its own; the tool intercepts
   * the original's call of a **host routine** -- it must be the next call on the tape, with the same arguments, the same
     stack depth and return-address size, and the data segment must hold exactly what the recompiled game had (`compare_memory`);
   * the original's entry into a **translated routine** -- it must be the next `F` event, with the same stack pointer and `bp`.
3. **Two kinds of host routines.**
   * **R (run):** the readable rewrites (`src/gen/readable.txt`), the rules (`getStat` ...), and pure C-library helpers
     (`_sprintf`, `_memset`, `SCOPY@`, `LXMUL@` ...).  The original routine is *executed* in the emulator; its result registers and
     all memory it changed are compared with what the C replacement did.  This checks the replacement itself.
   * **E (replay):** DOS, file access, the memory manager, input, timers, video, sound.  The original cannot run without the real
     machine, so the recorded effect (return registers and memory changes) is applied to the emulator.  Only the *callers* are
     checked for these, not the routine.  `tools/mm3_hostwrites.py` adds a static cross-check for them (below).
4. **First divergence stops the run** and prints the original's call chain, the last matched calls, and for memory differences
   the data-segment symbol names (from `mm3.idc`).

Timing is never compared: only order, arguments and state.

## What it found so far
* `stricmp` returned the lower-case difference where the original returns the upper-case one.
* The translator classified far calls to "collapsed" library routines (`_memcpy_0`, `SCOPY@` ...) as near calls
  (harmless for the game, but wrong for stack bookkeeping).
* `SCOPY@` / `F_LUDIV@` / `LUMOD@` pop their own arguments (`retf 8`); the lockstep's stack check needed that modelled.

## Scenarios, coverage
* `tests/lockstep.sh [scenario ...]` (explore, combat, intro, title, fuzzN) runs scenarios in parallel (4 at a time) and prints
  `no divergence` or the report.  `KEEP_TAPES=1` keeps the tapes; `LOCKSTEP_OPTS=...` passes options on.
* `tools/mm3_coverage.py build/ls_*.json > docs/lockstep-coverage.md` lists every host routine with how it is checked
  (lockstep-R, difftest, or replay only) and how often runs exercised it, and which translated routines no run ever entered.
* `tools/mm3_hostwrites.py build/ls_*.json` -- for the E routines: which data-segment variables did the host write, and does the
  original routine (statically, including everything it calls) ever store there?  A host writing a variable the original never
  touches is the "wrong offset" class of bug (e.g. `MemFree` writing the free-memory figure to the wrong variable).

## Per-routine fuzzing (`tests/calltest.sh`)
Scenarios only reach what a script can drive (spells, shops, many events need long menu sequences).  `tests/calltest.sh [-n CALLS]
[-s SEED] [NAME ...]` instead calls **each translated routine directly**: from the loaded game state it mutates a few dozen words of
the data segment (mostly in the party, maze, combat and monster areas), clears the stack, pushes random arguments and calls the
routine in the recompiled game (`mm3game_tape --calltest NAME N SEED`, a 4 s watchdog drops routines that loop) and then the same
call in the original code through the lockstep replay.  Host calls the routine makes are compared as usual, so a wrong address,
a wrong flag, a missed instruction in the translation shows up as a divergence of the data segment or of the call sequence.  One
process and one tape per routine; four run in parallel; results in `build/ct/NAME.report`.

## Details worth knowing
* Overlay 1 (`378C:0`) lies inside the 64 KB the recompiled game uses for DGROUP (its stack sits on top of that code in the flat
  image).  The recompiled game never executes that memory; the emulator does, so it runs a copy of the overlay placed in low
  memory (`OVL1_LOW_SEG` in `mm3_lockstep.py`).
* Inside an R routine the readable rewrite runs translated helpers (hooks such as `moveMonsters` -> `sub_27F5E`) at a different
  stack depth and with different registers than the original; stack-depth, register and intermediate memory checks are skipped
  there, and everything is compared again when the routine returns.
* Routines that return a `char` leave `ah` unspecified; only `al` is compared (listed in the report).
* The data segment is compared up to `--dg-limit` (default F200h): above that is the stack, and the original's C library uses far
  more scratch stack than the C replacements do.

## Limits
* E routines are believed; `mm3_hostwrites.py` only checks *where* they write, not what.
* Only code the scenarios reach is compared.  Use more scenarios / the fuzz seeds, and read the coverage report.
* Stores through registers cannot be resolved statically (reported as `indirect`).
* The replay needs `unicorn` and `capstone` (`pip install unicorn capstone`).
