# Roadmap

Current status and prioritized next steps for the shared Chapter 2/3
engine work (`yendor2.idb`/`yendor3.idb`, `docs23/`, `src23/`). See
[overview.md](overview.md) for the full narrative and
[engine-diffs.md](engine-diffs.md) for the Chapter 2 vs. Chapter 3
behavioral-difference reference.

## Status (last updated 2026-09-23, in-world text module)

**Disassembly-level analysis is essentially done.** This is the
important thing to know before starting the C reimplementation: you
should not need to go back to IDA for basic engine logic — the
groundwork below is already in place.

- `yendor2.idb`: all 769 functions named, all global variables named,
  and the three major on-disk formats decoded (see `file-formats.md`).
- `yendor3.idb`: the full 197-function BinDiff match review against
  `yendor2.idb` is complete (all three confidence tiers), the
  segmented-addressing global-rename blocker is fixed, and every
  confirmed Chapter 2 vs. Chapter 3 behavioral difference is written
  up in `engine-diffs.md`. A handful of very minor, non-blocking
  function identities remain unresolved (`engine-diffs.md`'s "Review
  status" section has the current list) — none of them are load-bearing
  for reimplementation; they're mostly debug hooks, a studio-credits
  easter egg, and one new item-registry helper whose exact semantics
  aren't pinned down.
- `src23/`: the C reimplementation is under way. One module is done:
  `bcd4.c`/`bcd4.h` (packed-BCD arithmetic, now including the digit
  shifts and `bcd4MulPercent`, i.e. `MulBCD4ByWord`, which is really a
  multiply-by-percent). Build/test with MinGW GCC:
  `gcc -Wall -Wextra -std=c99 -I .. -o test_bcd4 test_bcd4.c ../bcd4.c`
  from `src23/tests/`. Second module, `savegame.c`/`.h`
  (`CURGAME`/`SAVGAMEn` container: layouts for both games, section and
  record accessors, name/header-field helpers), also done; tests in
  `tests/test_savegame.c`. Third module, `party.c`/`.h` (500-byte party
  record: fields, the 27-stat table, classes, inventory/equipment/bags,
  flag banks; game-aware for Chapter 3's small differences), with shared
  `game.h`; tests in `tests/test_party.c`. Fourth module, `item.c`/`.h`
  (`WORLD.DAT` item catalog for both games: records, target and effect
  tables, name joining), tests in `tests/test_item.c`. Fifth module,
  `monster.c`/`.h` (catalog blocks + type lookup for both games, the 156-byte
  live record, spawn, death flags), tests in `tests/test_monster.c`. Sixth
  module, `effect.c`/`.h` (`g_trapEffectDefs` for both games: cost,
  resistance and magnitude decoding), plus a new `random.c`/`.h` (faithful
  `RandomInRange` port — its range is inclusive, see `file-formats.md`),
  tests in `tests/test_effect.c` and `tests/test_random.c`. Seventh module,
  `worldmap.c`/`.h` (the world map: one continuous 800-column tile grid for
  both games, plus Chapter 2's wall/floor tile-legend tables), tests in
  `tests/test_worldmap.c`. **Big finding**: there is no per-level map data
  — the whole game (towns, wilderness, dungeons) is one seamless coordinate
  space. Eighth module, `document.c`/`.h` (in-world readable text: found
  books/notes/plaques, *not* NPC dialogue despite the disassembly's
  "RunConversation" name — see `engine-diffs.md`), tests in
  `tests/test_document.c`. Both halves of the previous "`WORLD.DAT` map/text
  blocks" item are now done. The core dungeon loop is next, which will need
  `random.c` for movement/combat rolls and `worldmap.c` for the map itself.

## Next: continue the C reimplementation

This is the current priority. Recommended approach (established
early in this project and still the right one): scope one module at a
time rather than trying to plan the whole engine up front, doing any
remaining disassembly-level verification inline as each module is
written rather than as a separate upfront pass — the groundwork above
means that should rarely be necessary now.

**Naming conventions** (established, keep following them):
lowerCamelCase for C functions/variables (e.g. `bcd4Add`,
`bcd4FromU16`); `g_` prefix for actual global variables; PascalCase
for type names (e.g. `Bcd4`).

**Write for both games from the start.** `src23/` is shared between
Chapter 2 and Chapter 3 specifically so this doesn't need to be
revisited later — when a function differs between the two (check
`engine-diffs.md` first), reimplement the union of behavior with a
runtime or compile-time switch rather than picking one game's version.

**Platform backend: SDL2**, decided 2026-09-22. Graphics, sound, and
input in the C reimplementation are built on SDL2, not a bespoke
DOS-VGA/EMS-shaped layer — chosen specifically because the eventual
target is a ScummVM engine, and SDL is the closest available API shape
to what ScummVM's own backend expects, so the adaptation later should
be smaller. Practical implications for upcoming modules:
- Keep a clean split between game logic (the data-model modules
  written so far: `bcd4`, `savegame`, `party`, `item`, `monster`,
  `effect`, `random`, all platform-independent) and a thin platform
  layer that owns the SDL2 calls — window/surface, blitting,
  palette/color conversion, audio playback, keyboard/mouse polling.
  Don't let SDL types leak into the data-model headers.
- The original renders into a palettized VGA framebuffer
  (`PICTURES.VGA`, a master 256-color palette at `WORLD.DAT` offset
  `0x8270A` — see `file-formats.md`); the SDL platform layer should
  decode/composite into an indexed or converted-RGB surface and let
  SDL handle the actual blit/present, rather than reimplementing
  VGA-register tricks.
- Audio: the original drives Sound Blaster/FM synth through
  `SBFMDRV.COM` and an in-EXE sound-event table (`TriggerSoundEvent`,
  `word_368A7` family) — not yet decoded. When that's tackled, target
  SDL_mixer or raw SDL audio callbacks rather than emulating the
  original driver protocol.
- No SDL-specific game code has been written yet; this section exists
  so the decision is on record before the rendering-dependent modules
  (dungeon loop, `PICTURES.VGA`) start.

**SDL2 is installed on this machine** (2026-09-22): SDL2 2.32.10 mingw
devel package at `C:\sdk\SDL2-2.32.10` (headers under `include\SDL2`,
import libs under `lib`, `SDL2.dll` under `bin`) — not part of this
repo, a machine-local SDK matching `C:\mingw64`'s toolchain. Verified
with a smoke test (`SDL_Init`, driver enumeration, clean `SDL_Quit`).
Build/link a program against it:
```
gcc -I C:\sdk\SDL2-2.32.10\include\SDL2 -c yourfile.c -o yourfile.o
gcc -o yourprogram.exe yourfile.o -L C:\sdk\SDL2-2.32.10\lib -lmingw32 -lSDL2main -lSDL2 -mwindows
```
`SDL2.dll` must be next to the built `.exe` to run (copy it from
`C:\sdk\SDL2-2.32.10\bin`). `-lmingw32 -lSDL2main` before `-lSDL2`,
plus `-mwindows`, are required — `SDL2main` supplies the real `main`
that sets up console/argv redirection before calling the program's
`main`. A future SDL-consuming module's own test/build instructions
should follow this same pattern (mirroring how `src23/tests/*.c` state
their own `gcc` command in a header comment). If this repo is set up
on another machine, SDL2 needs reinstalling the same way (download the
mingw devel package matching the installed GCC).

**Picking the next module** — candidates, roughly in a sensible
dependency order (not a hard sequence; pick whatever's most useful
next):
1. ~~Round out `bcd4`'s own scope~~ — **done 2026-09-19**
   (`bcd4ShiftLeftNibble`/`bcd4ShiftRightNibble`/`bcd4MulPercent`; also
   fixed a half-carry bug in `bcd4Add`, see `overview.md`).
2. ~~Savegame I/O~~ — **done 2026-09-19** (`savegame.c`/`.h`). Still
   open there: the "new game" initializer (`InitializeNewGameWorldState`
   writes every section's defaults), and decoding sections 2-6's
   internals (fog-of-war bit order, item-instance fields, state-byte
   meanings) — do those with the modules that consume them.
3. ~~Party/character record structures~~ — **done 2026-09-19**
   (`party.c`/`.h`). Still open there: `+0x12`, the five equipment
   ratings `+0x48..+0x50`, and the item catalog fields (in `WORLD.DAT`),
   which the inventory slots' item ids refer to.
4. **Core dungeon-crawling loop** (movement, 90°-turn rendering) —
   the gameplay Paul is most interested in eventually, per the
   Eye-of-the-Beholder-style description in `overview.md`; bigger and
   more rendering-dependent than the above, probably comes after the
   data-model pieces it needs are in place.

`WORLD.DAT` and `PICTURES.VGA` (both decoded, see `file-formats.md`)
will be needed once map/graphics loading is in scope, but don't need
their own dedicated module ahead of that.

## Remaining minor open threads (not blocking)

From `engine-diffs.md`'s "Review status" section — only worth chasing
if reimplementing the specific function that touches them:
- `DrawShadowedTextAlt`/`PlayStudioCreditsIntro` (yendor3): a studio
  credits easter egg, real identity still not found.
- `sub_1B085` (yendor3): a new random party-ailment mechanic, traced
  structurally but its exact trap-effect id semantics aren't decoded.
- `sub_11778` (yendor3): a "limited item instance" registry helper,
  purpose not fully pinned down.
- `sub_2566C` (yendor3): a small, generic, widely-reused palette-fade
  stub with no confirmed yendor2 name.

## Open questions (yendor2, from earlier sessions, still unresolved)

- Is `NUORE` a currency, a resource, or both? Appears alongside "MAGIC
  ORE" in several UI strings — looks like a 3-resource economy. Note:
  `engine-diffs.md` documents that Chapter 3 appears to remove/replace
  the NUORE mechanic with a new 5-artifact quest system, which may
  make this moot for Chapter 3 but still matters for Chapter 2.
- Relationship between the "region" passwords (`YENDORIAN`,
  `BARIAGIAN`, `OBVERSIAN`, `MONTESERIAN`, `SLATORIAN`, `HEARDONIAN`)
  and the "town" passwords (`PORT HOPE`, `THIEF'S DEN`, etc.) — two
  separate systems, or one list split across two string clusters.
  **Sharper hypothesis as of the world-map decode (2026-09-22)**: since
  the whole game is one seamless coordinate space with no per-level
  files (`file-formats.md`'s "World map"), these are plausibly just
  named teleport coordinates into that single map rather than separate
  loadable areas — worth checking against whatever function actually
  consumes a matched password next time this is picked up. **New lead
  (2026-09-23)**: the in-world text decode found a Chapter 3 "Note"
  entry that is literally `THE PASSWORD IS` `` `RUSE~ `` (`file-formats.md`'s
  "In-world readable text"), signed by an NPC — real evidence that at
  least some passwords in this game are narrative/spoken, discovered and
  entered by the player, which may or may not be the same mechanism as
  the region/town password list; worth cross-checking directly.
- What `sg0977`/`sg0ffc`/`sg1486`/`sg195C`/`sg1ABC` (the 5 segments
  with IDA hex-address names instead of sequential `segNNN`) actually
  are — likely harmless IDA bookkeeping, not investigated further
  since it hasn't blocked anything.
