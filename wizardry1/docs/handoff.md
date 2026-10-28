# Handoff guide — where the Wizardry I port stands

Written 2026-10-28 for someone picking the project up cold.  Read this first; it points at the detailed
documents.  The per-segment reference docs (`town.md`, `maze.md`, `combat.md`, …) predate the current
approach in places — each now opens with a note saying so — and this file is the authority on *what is
current*.

## 1. What this project is

Rebuild **Wizardry I (DOS "Ultimate Wizardry Archives" release)** as a standalone C++ program that behaves
exactly like the original, as a stepping stone to a ScummVM engine shared by Wizardry I–III (Phase 4,
paused — see `roadmap.md`).

* The DOS game is UCSD Pascal: `SYSTEM.INTERP` (x86 p-machine) runs `SYSTEM.PASCAL` (p-code, 16 segments).
  There is **no DOS source**.  Thomas Ewers' Apple II Pascal reconstruction (`sources/`) is a parallel
  codebase — a good map, not a literal match.
* **Ground truth is the DOS executable.**  The p-code is decompiled (`tools/pcode_decomp.py`) and ported
  line by line.  The user's DOSBox screenshots are *only* used to sanity-check results — an early attempt to
  derive screen layouts from screenshots was wrong in structure and was explicitly rejected (see
  `docs/history/` and `docs/drawing.md`).
* Every ported screen is verified against an **oracle**: a Python p-machine (`tools/wizvm.py`) that runs the
  real `SYSTEM.PASCAL` on a transcription of the native console driver (`tools/conunit.py`).  The C++ port
  must reproduce, at every key wait, the composed screen cells, the CGA VRAM checksum and the RANDOM
  generator state.  `tools/screens_check.py` automates this (50 scenarios, all passing).

## 2. State of play (end of 2026-10-28)

`wiz1 dosgame` plays the game from the title screen through town, the Training Grounds, the maze, camp,
special squares, combat and the spoils, using only the DOS-faithful ports.  `ctest` is green (52 tests).

| p-code segment | C++ module | notes |
|---|---|---|
| WIZARDRY (mainline: window manager, text procs, XGOTO dispatch) | `dos.cpp`, `console.cpp`, `dos_flow.h` | `Console` = the CONUNIT window compositor, bit-exact vs the oracle (`console_replay`) |
| title | `title.cpp` | static + animation verified |
| CASTLE | `castle.cpp` | hub, Tavern, Inn (level-up), Edge of Town entry |
| SHOPS | `shops.cpp` | Edge of Town, Boltac's, Temple of Cant, leaving the maze, victory, party wiped |
| ROLLER | `roller_dos.cpp` | Training Grounds: create / inspect / delete / reroll |
| RUNNER | `maze_dos.cpp` | first-person view (procs 3–44), HUD, movement, every square type, encounter test, maze entry (UTILITIE:9–14) |
| CAMP | `camp_dos.cpp` | roster rows, character sheet, trade/drop/pool/use, camp spells (DIOS, MILWA, … KADORTO) |
| SPECIALS | `specials_dos.cpp` | scripted squares, message pager, I)NSPECT, DUMAPIC |
| COMBAT, CINIT, CUTIL, SWINGASW, CASTASPE | `combat_dos.cpp`, `combat_cutil.cpp`, `combat_act.cpp` | orders, rounds, melee, spells, breath, run, help, wish |
| REWARDS | `rewards_dos.cpp` | experience, treasure, chest (traps, inspect/calfo/disarm/open), drain refund, camp jobs (READ, IDENTIFY, KANDI) |
| UTILITIE (camp side) | `utilities_dos.cpp` | derived scores, E)QUIP incl. cursed items and special powers, R)EORDER, MALOR |
| GAMEUTIL | — | **not ported**: XGOTO 28/29 (Edge of Town → U)TILITIES) ends the flow (`dos_flow.h` falls through to `else break`) |
| KANJIREA | — | not ported (Japanese kana/kanji input; not on the English play path) |

`DosFlow` (`engine/src/dos_flow.h`) is the single XGOTO dispatcher shared by `dosgame` and the
`castle-test` regression harness.  The XGOTO table (also in the header comment of `dos_flow.h`):
1 ROLLER · 2/3 CASTLE · 4 CAMP · 5/6/10/16/25 SHOPS · 7 RUNNER · 8 COMBAT · 9/13/14/15/20/24 UTILITIE ·
11/22/24 REWARDS · 12/17/18/27 CAMP · 19 ROLLER · 23/26/24 SPECIALS · 28/29 GAMEUTIL.
XGOTO 24 is overloaded: `G36` (`MazeGlobals::g36`) picks 0 READ, 1 IDENTIFY, 2 KANDI (REWARDS),
3 DUMAPIC (SPECIALS), 4 MALOR (UTILITIE).

### Legacy code still in the tree

Before the oracle existed (≤ 2026-09-04) the engine was a port of the *Apple* source with an invented UI
(`textscreen`, `town_ui`, `maze_ui`, `camp_ui`, `combat_ui`, `roller_ui`, `maze3d`, `inn`, `temple`,
`cemetery`, `equip`, `combat`, `rewards`, …).  It is **not used by `dosgame`**; it remains only because its
CMake tests (`town_flow`, `maze_walk`, `combat_*`, `camp_*`, …) and `wiz1 town|maze|roller` still exercise
it.  Retiring it is on the to-do list (§6).  Do not extend it; do not copy its screen layouts.

Shared, still-current helpers the DOS ports use: `Scenario`, `StringPool`, `Rng`, `Character`, `Party`,
`Roster`, `Shop`, `UcsdVolume`, `MonsterArt`, `Font`.

## 3. Setting up

Prerequisites (Windows; all paths below are the ones hard-coded in the tools — change them if yours differ):

* The game: `C:\games\wizard15\WIZ1.DSK` (from the Ultimate Wizardry Archives CD).  Not redistributable;
  never committed.  `extracted/`, `*.bin`, `wiz1_interp` are gitignored.
* Python 3 (no third-party packages).
* Visual Studio 2022 Community (MSVC + the bundled CMake/Ninja).  Optional SDL2 for the `dosgame` window:
  set `WIZ_SDL_DIR` (default `C:/dev/scummvm_libs_2015`); without it only the headless commands build.
* Optional: IDA Pro 8.3 (`ida_scripts/`) — only needed to redo the `SYSTEM.INTERP` analysis; the `.asm`/`.idc`
  backups are committed.

```powershell
# 1. unpack the disk (from wizardry1/)
python tools/ucsd_disk.py extractall "C:/games/wizard15/WIZ1.DSK" extracted/wiz1
# 2. p-code listings (used by the decompiler and several tools)
python tools/pcode_dis.py dis-all extracted/wiz1/SYSTEM.PASCAL extracted/pcode
# 3. build  (from anywhere; PowerShell — the execution policy blocks it from some shells)
engine\build.ps1            # -> engine\build\wiz1.exe
# 4. play  (needs the SDL build)
engine\build\wiz1.exe dosgame extracted\wiz1 <stateDir>    # keeps roster.dat, party.dat, shop.dat, maze.dat there
# 5. test
ctest --test-dir engine\build          # includes console_replay and castle_screens (skipped if no disk/python)
```

The engine never touches the game's own `SCENARIO.DATA`; it seeds `roster.dat` etc. from it and keeps its own
`.dat` files (gitignored).

## 4. The verification workflow (how every screen was ported)

1. **Read the original.**  `python tools/pcode_decomp.py <SEGMENT> <proc>...` (or `all`) prints a
   goto-style decompilation with global and string names; `extracted/pcode/*.pd` has the raw listings.
   Conventions: `Uk.n` = local *n* of the frame *k* static levels up; params are `a1..` (a1 = highest word);
   `WIZARDRY:n` are the shared UI procs (`docs/procmap.md`, `docs/drawing-inventory.md`); `XGOTO`
   assignments + `EXIT(seg:proc)` show control flow between segments.
2. **Port literally** into `engine/src/<seg>_dos.cpp`, naming each routine after its p-code proc
   (`// CAMP:41`).  Keep call order identical — including RNG calls.
3. **Write a scenario** in `tools/screens_check.py` (`SCEN`): a key string, an optional `patch=` of roster
   words (`"slot:word:value,..."`) to put a character into the situation, and the engine command
   (`castle-test`).  Keys are the same for both sides.
4. **Run it:** `python tools/screens_check.py engine/build/wiz1.exe [scenario ...]`.  It runs the oracle
   (`wizvm.py --ops --shotdir`), runs the engine with `WIZ1_RNG`/`WIZ1_PATCH` set to match, then
   `cmpgrids.py` compares each key-wait screen (cells + VRAM CRC + RNG).  A mismatch prints the differing
   cells.  `tools/showgrid.py` prints a `.grid` dump as ASCII; `tools/rngsteps.py` counts RANDOM calls
   between two states (find a missing/extra `rnd()`).
5. Search aids: `tools/mazepath.py` (key strings that walk to a special square), `tools/findenc.py`
   (key strings that trigger a wandering encounter at a known RNG state).

Hard-won gotchas:

* **Type-ahead.**  Any `GetKey` consumes the next scripted key; only keys the prompt accepts matter, and
  menus match **upper-case** hotkeys.  Scenarios must end at a plain key wait, never in a spin loop.
  This is why combat's F)IGHT path cannot be driven by script (see §6).
* **`LAND`/`LOR` evaluate both operands** in p-code — a port that short-circuits will skip a `RANDOM` call
  and desynchronise everything after it.  (Conversely, when the decompiler prints two consecutive
  `if not … goto` lines, that *is* nested/short-circuit.)
* `EXIT(seg:proc)` unwinds to the caller of that proc; ports use C++ exceptions or bool returns.
* RNG sync: the oracle and engine start from the same RANDOM state captured at the first castle screen;
  the shipped RNG is the weak 128-value one (`docs/rng-validation.md`).
* Build only with `engine\build.ps1` from the PowerShell tool/terminal.
* Windows paths: tools open `C:\games\wizard15\WIZ1.DSK` literally.
* Heredocs/inline Python mangle backslashes in some shells — put scripts in files.

## 5. Where things are

| path | what |
|---|---|
| `../docs/` (repo root) | engine-wide reference shared by Wiz I–V: `engine.md`, `pmachine.md` (opcodes, SBIOS, RNG), `pcode.md`, `file-formats.md`, `rng-validation.md` |
| `docs/drawing.md` | **how the DOS build draws** — CONUNIT selectors, window compositor, text procs; the basis of `Console`. `docs/drawing-inventory.md` is generated by `tools/drawcalls.py --md` |
| `docs/globals.md`, `procmap.md` (+ `.tsv`) | global-variable and procedure name maps used by the disassembler/decompiler |
| `docs/town.md`, `maze.md`, `combat.md` | per-area analysis (partly Apple-derived; read with the notes at their tops) |
| `docs/roadmap.md` | phase checklist |
| `docs/history/` | the full working conversation and screenshots (§7) |
| `engine/` | C++: `include/wiz/*.h`, `src/*.cpp`; `README.md` has the module map and commands |
| `tools/` | Python: disk/p-code tools, the oracle, regression scripts (index below) |
| `ida_scripts/`, `wiz1_interp.asm/.idc` | IDA automation and committed analysis backups of `SYSTEM.INTERP` |
| `sources/` | Apple II Pascal reference |

### Tool index (`tools/`)

| tool | use |
|---|---|
| `ucsd_disk.py` | list / extract files from a `.DSK` UCSD volume |
| `pcode_dis.py` | disassemble `SYSTEM.PASCAL` (`dis-all`, `dis`, `procs`, `segments`) |
| `pcode_decomp.py` | **decompile** a segment/proc: `pcode_decomp.py CAMP 41` or `UTILITIE all` |
| `wizvm.py`, `conunit.py` | the oracle: run the real p-code on a transcribed CONUNIT driver (`run --keys … --shotdir … --ops --patch … --rng …`) |
| `screens_check.py`, `cmpgrids.py`, `showgrid.py` | oracle-vs-engine screen regression and viewers |
| `replay_check.py` | `Console` vs the oracle's recorded driver calls |
| `drawcalls.py`, `conunit_calls.py`, `opsum.py` | inventory of drawing procs / CONUNIT call sites / condensed op logs |
| `mazepath.py`, `findenc.py`, `rngsteps.py` | key paths to special squares, find encounters, count RNG draws |
| `scenario.py`, `monsters.py`, `strpool.py`, `globals.py`, `procmatch.py` | data decoders and name maps |
| `vmtrace.py` | trace proc entries in an oracle run |
| `export_transcript.py` | regenerate `docs/history/` from a Claude Code session log |

## 6. Known gaps and suggested next steps

1. **GAMEUTIL (XGOTO 28/29)** and the Edge-of-Town **U)TILITIES** entry; examine what the segment offers
   and port it the same way (decompile → `gameutil_dos.cpp` → scenario).
2. **More oracle coverage of combat and rewards.**  Verified so far: combat intro, orders, party spells
   (MALIKTO), monster attacks, full rounds, group wipe → chest with traps, TPK cemetery.  Not yet driven
   against the oracle: U)SE items, R)UN, D)ISPELL, breath, call for help, HAMAN/MAHAMAN, level drain,
   trap effects, disarm, CALFO, drain refund.  The F)IGHT path (`SWINGASW` memberAttacks) is transcribed but
   unscriptable because of type-ahead — consider adding a "forced key" hook to the oracle or a patch that
   pre-fills the action orders.
3. **Level-drain refund** uses an assumed 0 for an uninitialised value (`U2.18`) — check against the
   p-code's frame set-up.
4. **Persistence:** monster/maze record changes made during play (`MazeGlobals::levelRec`, `monsterRec`) live
   in memory only; the original writes them back into `SCENARIO.DATA`.
5. **Verify `UTILITIE:15` (derive) against the shipped roster** at level 1 (the maze-entry scenarios pass,
   which already exercises it silently for the default party).
6. **Retire the legacy modules** (§2) and their CMake tests once nothing needs them; remove the unused
   `Ui`/`TownWorld` objects still instantiated in `dos_game.cpp`.
7. **Docs to refresh as you touch them:** `town.md`, `maze.md`, `combat.md`, `ui.md` describe the Apple-derived
   model in places.
8. `KANJIREA`, `HAS.CACHE`, `KANA.KEYMAP`, `HAS.STROPS` ABI remain unidentified (low priority).
9. **Phase 4 (ScummVM)** waits on Wiz II/III RE.  `SYSTEM.INTERP` is byte-identical across WIZ1–3, so
   Wiz II/III should mostly be new scenario data plus diffs in `SYSTEM.PASCAL`; the oracle and the
   `*_dos` structure were built with that reuse in mind.

## 7. History and context

`docs/history/` holds the complete working conversation (two sessions, 2026-09-02 → 2026-10-28) rendered to
Markdown, plus every screenshot that appeared in it.  See `docs/history/README.md` for an index, the
reference screenshots the project owner supplied, and a guide to which part to read for which topic.  The
pre-compaction summaries embedded at the start of several parts are the densest record of decisions.

Working conventions: commit + push after each documented increment; commit messages end with
`Co-Authored-By: Claude …`; scratch output goes outside the repo.
