# Chapter 1 — analysis and reimplementation strategy

Written 2026-10-22, after the IDA databases were created (see
[`overview.md`](overview.md)) and a first structural survey of them. Numbers
below come from that survey; anything not yet traced is marked *(to confirm)*.

## What the survey found

1. **`SW.EXE` is a state machine, not the game.** `start` adjusts memory, runs
   some setup, then repeatedly `EXEC`s a `PROGn.EXE` through one helper
   (`sub_1144C`, the only `int 21h/4B00h` in the binary), reads the child's
   **exit code** (`int 21h/4Dh`) and branches on it (codes 0–9, 0x20, 0x21 seen
   so far) to pick the next program. Programs are full-screen modules; the exit
   code is the edge label in a program-transition graph.
2. **`VID16.DRV` is a resident driver on `INT 62h`.** It installs itself
   (`INT 21h` 25h, vector 62h), uses `INT 10h` for mode setting and `INT 33h`
   for the mouse, and ends with terminate-and-stay-resident. `SW.EXE` and every
   `PROGn.EXE` call it: about **2,170 `int 62h` call sites** in 20 binaries,
   roughly **35–40 distinct function numbers** (AH; a heuristic count, to
   confirm). So graphics, palette, mouse and probably timing/sound are one
   small, well-defined ABI. `SW.EXE` names `VID256.DRV`/`VIDP256.DRV` too, but
   they aren't in this shareware install: the target is the 16-colour (EGA/VGA
   640x350/400) path. `VIDP16.DRV` (817 bytes) is a second blob whose role is
   unknown.
3. **Little exact code sharing.** 1,674 functions across 22 databases, 1,454
   unique by byte hash (`dump_func_hashes.py`). Almost all of the duplication
   is `PROG12`/`13`/`14`, whose code is byte-identical (44 functions, 4 KB of
   code in 55–64 KB files — the rest is data). So there is no big common
   runtime to reimplement once; the work divides by program. Total code is only
   ~217 KB.
4. **Every program opens the same file set** (`FILE00-06.16`/`.PAL`,
   `CURRGAME\{PARTY,NPC,AREA,BATPIC}.DAT`, `WORLD/TOWN/MINE.DAT`). Game state
   is passed between programs through the `CURRGAME` files, not memory, so the
   file formats *are* the inter-module interface. `SAVEGAME.A/` is a saved
   game we can use as test data.

## Guiding decisions

- **C++ with one namespace per executable.** The end product is a ScummVM
  engine, and the game is 20 separate programs that each have their own
  globals and routines with no linker ever having seen them together (the same
  name can mean different things in `PROG4` and `PROG9`). Each original
  executable is therefore its own namespace under a common root:
  `Yendor1::Sw` (the `SW.EXE` shell), `Yendor1::Prog0` … `Yendor1::Prog18`,
  `Yendor1::Install` if it is ever needed, plus the shared layers below. Names
  inside a namespace need no program prefix, so `Prog4::drawPanel` and
  `Prog9::drawPanel` can coexist and each stays traceable to its own database.
- **Mirror the original's structure.** One module (directory + namespace) per
  original program plus `Sw` for `SW.EXE`. A program's entry point is
  `int Yendor1::ProgN::run()` returning the same exit code, and `Sw` keeps the
  original's dispatch table. This keeps every function traceable to a
  disassembly address and lets us bring programs up one at a time, with the rest
  stubbed to "return to shell".
- **Dependency rule.** A `ProgN` namespace may depend on the shared layers but
  **never on another program's namespace**. The shared layers are
  `Yendor1::Vid` (the `INT 62h` driver API over the platform backend),
  `Yendor1::Data` (file-format readers/writers for the `CURRGAME` records, maps
  and graphics — the real inter-program interface) and `Yendor1::Shared` (small
  utilities; named `Shared` rather than `Common` so it can never be confused
  with ScummVM's own `Common` namespace). Code only moves into `Shared`/`Data`
  once it is verified to be the same behaviour in every program that has it (checked in the disassembly, not
  assumed from the name); until then each program keeps its own copy, which
  makes a wrong "these are the same" guess cheap to fix. The one planned
  exception is `Prog12`/`13`/`14`, whose code is byte-identical: it is
  implemented once and the other two are thin wrappers supplying their data.
- **Reimplement the video driver as the platform layer.** `INT 62h` is the
  hardware abstraction line. `Yendor1::Vid` implements that ABI; its backend
  (SDL2 for development, ScummVM's `OSystem`/`Graphics::Surface`/events later) is
  the only code that touches a platform API. Programs call `Vid::xxx()`
  functions that map 1:1 to driver function numbers.
- **Keep the original file formats and save compatibility.** Read/write the
  real `CURRGAME`/`SAVEGAME` files, so the original saves are test data and
  state hand-off between modules needs no new design.
- **Chapter 1 is a separate engine** — don't share code with `src23`, which is C
  and Chapter 2/3-specific. Naming follows ScummVM style so the later port is
  mechanical: PascalCase types and namespaces, lowerCamelCase functions and
  variables, `g_` for namespace-level globals. Tests live in
  `src1/tests/` and skip when game files are absent.
- **Write ScummVM-friendly C++ from the start.** C++11 at most, no exceptions or
  RTTI, no `std::string`/iostreams/`<stdio>` in game code, fixed-width integer
  types, explicit little-endian field reads (never cast a struct over file
  bytes), and all file access and time behind a small `Shared`/`Vid` interface
  that ScummVM's `Common::File`, `Common::Serializer` and `OSystem` can replace.
  Check against ScummVM's `forbidden.h` list when porting, not before.
- **Verify against the real thing.** Run the original in DOSBox as an oracle
  for screens and behaviour (the other games' `game/` folders already carry a
  `dosbox.conf`), and check every data-format claim against the real files, not
  a single sample.

## Phases

### Phase 1 — Make the disassembly readable (tooling, ~1 session)

1. **Document the `INT 62h` ABI from `VID16.DRV`.** Only 15 functions /
   2.7 KB of code. Name its functions and its dispatch table, and write
   `docs1/video-api.md`: function number, registers in/out, what it does.
2. **Annotate every `int 62h` site in all 20 clients** with the function name
   (an IDA script keyed on the preceding `mov ah, N`, applied to every
   database). This turns ~2,170 anonymous calls into labelled ones and is the
   largest readability win available for the effort.
3. **Cross-database tooling**: propagate names between byte-identical functions
   (trivially covers `PROG12/13/14`), and use BinDiff (already installed with
   IDA) for near-matches across programs; a per-program report of which data
   files and driver functions it touches. Check whether any FLIRT signature
   matches a C runtime in the startup code.

### Phase 2 — The map of the game: `SW.EXE` (small, ~1 session)

Fully analyse `SW.EXE` (43 functions): the dispatch table, the exit-code
transition graph, and the state it keeps between programs. Output:
`docs1/program-graph.md` (program → exit code → next program) and a first
role for every `PROGn`. This is the reimplementation's skeleton, so it
determines the order of everything after it.

### Phase 3 — Data formats and an asset viewer (parallel with 2)

Standalone C++ tools under `src1/tools/`, each validated visually or against a
real save:
1. `FILEnn.16` + `.PAL` — the planar 16-colour graphics. First milestone with a
   *visible* result: an SDL viewer that shows every image.
2. `CURRGAME`/`SAVEGAME.A` records (`PARTY`, `NPC`, `AREA`, `BATPIC`).
3. `WORLD`/`TOWN`/`MINE.DAT` maps (256 KB each — probably the three explorable
   areas). `docs1/file-formats.md` is the living reference.

### Phase 4 — Vertical slice

Build the platform layer (`Vid`, input incl. mouse, timing), the `Sw` shell with
every program stubbed, then reimplement the **first program(s) in game flow**
(title / start-up path) completely. Exit criterion: launch, show the real title
screen, accept input, and move through the shell's graph into the next stub.
This proves the video ABI, the data loaders and the shell contract before the
big programs.

### Phase 5 — Programs, in game-flow order

Per program, the same routine: classify (files touched, driver functions,
string content) → name functions → document in `docs1/progNN.md` → implement
the C++ namespace/module → unit tests on its logic → compare with the DOSBox original.
Order by dependency and size:

| Group | Programs | Notes |
|---|---|---|
| start-up / menus / small screens | `PROG0`–`PROG3`, `PROG6`, `PROG16`–`PROG18` | 6–16 KB each, 12–58 functions; cheap wins, establish the module contract |
| mid-size feature screens | `PROG5`, `PROG7`, `PROG8`, `PROG9`, `PROG11`, `PROG15` | 25–55 KB |
| the large engines | `PROG4` (300 funcs), `PROG10` (204 funcs) | largest and most coupled; leave until the shared state and data are solid |
| data-heavy triplet | `PROG12`/`13`/`14` | identical code: one implementation (in `Prog12`) parametrised by three data blobs; `Prog13`/`Prog14` are thin wrappers |
| installer | `INSTALL.EXE` | not reimplemented as such; mined once for the new-game `PARTY.DAT` bootstrap and config defaults |

### Phase 6 — Integration and polish

Whole-game playthrough against the original, save/load round-trips with the
original files, then the ScummVM-facing cleanup.

## Open questions

- What `VIDP16.DRV` is (looks like a flat blob loaded by `VID16.DRV` or
  `SW.EXE`; no one has traced the loader yet).
- How `sub_1144C`'s two arguments select the program (a program index plus a
  command-tail string?) and whether it also runs the driver load.
- Whether sound (PC speaker / AdLib) goes through `INT 62h` too — decides if
  audio is part of `Vid` or a separate backend.
- Intended scope: 16-colour only (all this install supports), original-save
  compatibility, and whether a ScummVM engine is the end product from the start
  or a later port.

## Suggested next step

Phase 1, items 1–2: finish `VID16.DRV` and annotate every `int 62h` site. It is
the cheapest work that makes everything after it faster.
