# Rob Blanc 1 engine reimplementation — build strategy

Companion to `../CLAUDE.md` (the reversing project) and this directory's
own `CMakeLists.txt`/`libs/` (the third-party build harness, already
working — see `smoketest.c`). This file plans the *next* layer: our own
C reimplementation of the AGS engine logic the disassembly work has
recovered, built so that **every commit compiles and links**, growing
outward from small, independently-testable milestones rather than a
big-bang "implement everything" pass.

Target for now: a **standalone native test harness** — an executable
that links `allegro`/`jgmod`/`almp3`/`libcda` (already built under
`libs/`) the same way the original `rb.exe` did, and that can load and
run the *real* Rob Blanc 1 game files from disk. Porting this to
ScummVM's own `OSystem`/event-loop abstraction (replacing Allegro
`BITMAP*` with `Graphics::Surface`, etc.) is deliberately **out of
scope for now** — a later, mostly-mechanical step once the engine
*logic* itself is proven correct against the real game data. Fighting
two unknowns (our own reimplementation's correctness, and ScummVM's
build/plugin conventions) at once would slow both down.

## Guiding principles

1. **Always compilable, never half-written.** A function's body is
   *either* a complete, cited real implementation, *or* a one-line stub
   (see below) — never something in between that fails to compile.
2. **Milestone-driven, not subsystem-complete-driven.** Each step below
   is a small, demonstrable, testable piece of real behavior (e.g.
   "print the game's title", "walk the player around room 1"), pulling
   in exactly the structs/functions it needs. This naturally avoids
   writing ~1000 mechanical stub functions up front for code paths we
   may not reach for months — the stub surface grows lazily, one real
   caller at a time.
3. **Stub calls are logged, not silent.** A function nobody has
   implemented yet still needs to *do* something reasonable when
   called against real game data — log which native/engine function
   was hit and return a harmless default. Played through far enough,
   this turns into a **data-driven priority list**: whatever the real
   game actually calls that we haven't implemented yet is exactly what
   to implement next, instead of guessing.
4. **Every real port cites its evidence**, the same convention already
   used in `matches.json`/`reversing/notes/struct-layout-drift.md` — a
   comment naming the struct-layout-drift.md section, matches.json
   entry, or `Common/`/`Engine/` reference location a given
   implementation is based on. Keeps the reconstruction readable as
   "close to the original" per the reversing project's own aim #3.
5. **Real game data is the regression suite.** Several loader stages
   already have working, verified Python prototypes
   (`reversing/scripts/parse_clib_manifest.py`,
   `dump_gamesetup_from_data.py`, `dump_characters_from_data.py`,
   `dump_interface_elements.py`). Porting one of these to C has an
   unusually strong acceptance test for free: **the C output must
   match the already-verified Python output byte-for-byte**, checked
   against the real local files (`C:\games\ags\robblanc1\ac2game.dat`,
   `C:\games\ags\robblanc1_win\rb.exe`).

## The stub convention

```c
/* src/engine/include/ags/stub.h */
#ifndef AGS_STUB_H
#define AGS_STUB_H

/* Every not-yet-implemented function's ENTIRE body is one of these two
 * macros -- never a partial implementation. Dedups by call site (not
 * by call count) so driving the real game through a loop doesn't spam
 * the log; writes to stderr and to stub_hits.log for later review. */
void ags_stub_hit(const char *func, const char *file, int line);

#define AGS_STUB_VOID() \
    do { ags_stub_hit(__func__, __FILE__, __LINE__); } while (0)

#define AGS_STUB(retval) \
    do { ags_stub_hit(__func__, __FILE__, __LINE__); return (retval); } while (0)

#endif
```

A stub for, say, `DisplayMessage` (not yet implemented) looks like:

```c
void DisplayMessage(int msnum) {
    AGS_STUB_VOID();  /* TODO: matches.json "DisplayMessage" */
}
```

Once implemented for real, the macro call is simply deleted and
replaced with the real body — the function's *signature* never has to
change (`prototypes.json`, already generated from the reference
source, is the source of truth for every signature), so a stub can be
promoted to a real implementation without touching any caller.

## Step 0: the skeleton (struct headers only, no logic)

Before any behavior, port every struct `struct-layout-drift.md` has
already closed into real C headers under `src/engine/include/ags/`.
These are pure data layout — trivially compilable, zero risk, and this
project has already done the hard work of nailing down every offset.
Group into headers roughly mirroring AGS's own `Common/` split:

- `gamesetup.h` — `OriGameSetupStruct`/`OriGameSetupStruct2`
  (`GameSetupStructBase`), `MouseCursor`, `InventoryItemInfo`,
  `InterfaceElement`
- `gamestate.h` — `GameState`, `ScreenOverlay`, `CharacterExtras`
- `character.h` — `OldCharacterInfo` (`CharacterInfo`)
- `room.h` — `RoomStruct`, `RoomStatus`, `RoomObject`, `PolyPoints`,
  `sprstruc`
- `interaction.h` — `EventBlock`, `AnimationStruct`/`FullAnimation`
  (né `EventBlockCmd`), `GameAnimation`
- `view.h` — `ViewStruct272`/`ViewFrame272`, `MoveList`
- `dialog.h` — `DialogTopic`, `WordsDictionary`
- `gui.h` — `GUIObject` base + `GUIMain`, `GUIButton`, `GUILabel`,
  `GUITextBox`, `GUIListBox`, `GUIInv`, `GUISlider`
- `script.h` — `ccScript`, `ccInstance`, `SystemImports`,
  `ExecutingScript`
- `sound.h` — `SOUNDCLIP` base + `MYWAVE`, `MYMP3`, `MYSTATICMP3`
- `sprite.h` — `SpriteCache`, `SpriteListEntry`
- `clib.h` — `MultiFileLib` (the CLIB asset-manifest format)
- `misc.h` — `TreeMap`, `OnScreenWindow`

Every struct gets a `static_assert(sizeof(X) == N, "...")` right under
its declaration, `N` taken directly from the already-confirmed total
size in `struct-layout-drift.md` — turns this project's own hard-won
struct-recovery conclusions into a permanent compile-time regression
check. (CSCI dialog controls — `NewControl`/`PushButton`/`MyListBox`/
`MyLabel`/`MyTextBox` — are deliberately deferred; they only matter for
the save/restore/setup dialogs, not for playing the game.)

## Directory layout (proposed, will adjust as it grows)

```
src/
  libs/                 (existing — third-party, unchanged)
  engine/
    include/ags/        (Step 0 headers, plus stub.h)
    src/
      clib/              CLIB manifest + asset reading
      loader/            ac2game.dta -> GameSetupStructBase/chars/etc.
      script/            ccScript/ccInstance + interpreter + imports
      room/              RoomStruct load, LZW/RLE, EventBlock dispatch
      gfx/                thin wrappers over Allegro (blit, palette)
      char/              CharacterInfo/View animation & movement
      gui/               GUIMain/GUIObject hierarchy
      sound/             PlayMusic/PlaySound/PlaySpeech wiring
      dialog/            DialogTopic/run_dialog_script/do_conversation
      mainloop/          mainloop/do_main_cycle/process_event
      api/               the ~239-entry script-exported native API
      stub.c             ags_stub_hit()'s own implementation
    tests/               one small harness per milestone below
  CMakeLists.txt         (existing; gains add_subdirectory(engine))
```

Each `src/` subfolder above becomes its own small CMake static-library
target (mirroring `src/libs/*/CMakeLists.txt`'s own convention),
collected into one `agsengine` target that the milestone test
executables and, eventually, a real game executable link against.

File-to-function assignment isn't arbitrary: use `matches.json`'s own
`source_file` field (e.g. `Engine/AC.CPP`, `Common/acroom.h`) to decide
which of the folders above a given function's real implementation
belongs in — keeps this new C tree cross-referenceable against the
disassembly annotations it's built from.

## The milestone ladder

Each milestone lists what becomes *real*, what stays *stubbed*, and
the concrete pass/fail check. Milestones build strictly on prior ones.

- **M0 — done.** Third-party libs built & linked (`smoketest.c`).

- **M1 — "Read the manifest."** Port `parse_clib_manifest.py`'s logic
  (`csetlib`/`clibfopen`/`read_new_format_clib`, `matches.json` already
  has these) to C. No stubs needed — genuinely leaf-level code. **Test:**
  print CLIB version + file count for both the real DOS and Windows
  installs; must match the Python script's own already-verified output
  exactly.

- **M2 — "Read the header."** Extend `load_ac2game_dta`'s own header
  parse (from `dump_gamesetup_from_data.py`) to fill a real
  `OriGameSetupStruct` (Step 0's struct, not a placeholder) and print
  `gamename`/`numfonts`/`numviews`/`numcharacters`/`color_depth`. **Test:**
  `gamename == "Rob Blanc I"`, `numfonts == 3` — matches the Python
  prototype and the CLIB manifest's own independently-counted `.wfn`
  file count.

- **M3 — "Meet the cast."** Extend the loader through
  `WordsDictionary`/the compiled-script blob/`ViewStruct272[]`/a second
  skip (`dump_characters_from_data.py`'s own exact byte-consumption
  sequence) to reach the real `CharacterInfo[]` array. The compiled
  script blob is *seeked past*, not parsed — an honest, structural stub
  (we don't need script bytecode yet, just to skip its known size to
  reach what comes after it), self-checked the same way the Python
  version was (`"SCOM"` signature + `0xBEEFCAFE` trailing sentinel).
  **Test:** decodes the real 5 characters — `ROB`/`HIGH ONE`(x2, script
  names `HIGHONE`/`HIGHTWO`)/`DROID`/`HOLOGRAM` — byte-for-byte matching
  the Python prototype.

- **M4 — "Run the interpreter on nothing."** Implement `ccScript`
  loading (`fread_script`'s field format), `ccCreateInstance(Ex)`
  (export/import resolution), and the interpreter's full 38-opcode
  `SCMD_*` dispatch (fully mapped already, zero unknowns) — but every
  `SCMD_CALLEXT`/import call is an `AGS_STUB` that logs the native
  function name and returns 0. **Test:** load Rob Blanc 1's *real*
  compiled global script and run it. This is the first genuinely
  self-prioritizing milestone: whatever native calls it logs become the
  concrete "implement these next" list for M9+, instead of guessed
  priority.

- **M5 — "Open a room."** `load_room`/`load_main_block`'s block-type
  dispatch, LZW/RLE decompression (`load_lzw`/`lzwexpand`,
  `loadcompressed_allegro`/`cunpackbitl` — all already matched in
  detail) for real; `EventBlock` structures loaded but not yet
  dispatched. **Test:** print room 1's width/height/hotspot names/object
  count from the real `.crm` file.

- **M6 — "See the room."** Wire the decompressed background into a
  real Allegro `BITMAP*` and blit it to an actual window. First
  visually-verifiable milestone — proves the graphics pipeline (already
  linked, unused until now) end-to-end.

- **M7 — "Meet the room's people."** Draw the player `CharacterInfo`'s
  current view/loop/frame sprite at its room position — static only, no
  animation or movement logic yet.

- **M8 — "The world starts moving."** Real `mainloop`/`do_main_cycle`,
  keyboard/mouse polling (Allegro, already linked), `walk_character`/
  `update_stuff`'s animation-advance logic, the already-mapped
  `MoveList`/route-finding subsystem. **Test:** player character walks
  around room 1 under keyboard/mouse control.

- **M9 — "Say something."** Promote the highest-value native calls
  M4's log surfaced: `DisplayMessage`/`Display`, `GetHotspotAt`,
  `RunHotspotInteraction`/`run_event_block`'s full `respond[]`
  dispatch (all 15 values already documented). **Test:** clicking a
  hotspot in room 1 produces the real game's own message/behavior.

- **M10 — "It has a voice."** `PlayMusic`/`PlaySound`/`PlaySpeech`
  wired into the already-linked JGMOD/ALMP3/Allegro-MIDI calls.

- **M11+ — the long tail.** GUI rendering (`GUIMain`/`GUIObject::Draw`
  hierarchy), dialog system (`run_dialog_script`/`do_conversation`),
  save/restore, inventory, and the remaining ~230 script-API entries —
  implemented continuously, prioritized by the growing "unimplemented
  native call" log from actually playing the real game as far as the
  current build allows, not by working subsystem-by-subsystem in the
  abstract.

## Explicitly deferred / out of scope for now

- **ScummVM `OSystem`/plugin integration** — a later, separate port
  once the logic above is proven against real game data.
- **CSCI legacy dialog controls** (`NewControl`/`PushButton`/
  `MyListBox`/etc.) — only needed for the save/load/setup dialogs, not
  for playing the game itself.
- **Editor-only paths** (things `matches.json` already notes are
  compiled out of the shipped engine, e.g. `SPLITRESOURCES`).
- **Third-party library internals** — per `CLAUDE.md`'s own scope
  rule, we call into Allegro/JGMOD/ALMP3/libcda's *public* API only;
  never reimplement their insides.
- **Windows Explorer `.ags` file-association integration** — confirmed
  absent from this build already (`ags-archives` cross-reference), not
  reimplemented.

## Next action

Start Step 0: create `src/engine/include/ags/` and port the struct
headers listed above (no behavior), each with its `static_assert`.
Then M1 (`src/engine/src/clib/`) — the CLIB manifest reader, the
smallest genuinely real milestone, directly testable against both real
local game installs today.
