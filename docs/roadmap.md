# Status and roadmap

## Where things stand

The request was a C implementation "that can play both games". That is a
large project: the executables are ~430 KB of hand-written, unlabeled 16-bit
assembler (≈35 000 lines of disassembly listing in `seg000`/`seg001`; the rest of the image is data) with
their own script language, planar-VRAM blitters and sound drivers. This first
pass concentrated on the groundwork that everything else depends on, and on
building oracles so later work can be *verified* rather than guessed.

| Area | State |
|---|---|
| `mjm` container, PKWARE DCL, RNC1 | **Done** — Python + C, identical output for all 101 decodable files, unit-tested |
| `.PC1` pictures | **Done** — Python + C, matches DOSBox pixel-for-pixel (visually) |
| Game detection / path resolution (Cadaver 1 vs Payoff, `DATA`/`TATA`, `?` disk placeholder, case-insensitivity) | **Done** (`src/engine/gamefiles.c`) |
| SDL2 front end | Shows the title and scroll pictures of either game. **No gameplay.** |
| Emulator harness + DOSBox capture | **Done** — runs the real game to the level-1 screen (see `docs/tools.md`) |
| `FONT2.SPR`, `FLAME.DAT`, title scroll | **Done** — pixel-exact against the running intro; animated scene in the SDL app (`--scene intro`). The story/credits pages of the intro are not sequenced yet |
| `*.LNG` text | **Done** — 6-bit packed text, Python + C, all 41 files verified, in-game strings confirmed in the emulator |
| `PCMAN.DAT` (player sprites) | **Done** — directory + 5-plane records, Python + C, all 48 frames, viewer in the SDL app |
| Palette | **Done** — 16 ST words, per level in `MP1` (all 9 levels) |
| Floor tiles (`MP3`) and floor maps (`MP1` section 0) | **Done** — isometric floor of every room renders like the game; Python + C, unit-tested on 14 level dirs; room viewer in the SDL app |
| `LEVn.MP2` | Sections, room lists and the thing table decoded and named (Python + C, unit-tested on all 9 levels); thing record fields only partly understood |
| `ANDATA` blocks | **Done** — 0 verb icons, 1 screen frame, 2 scroll tiles, 3 message font, 4 status font (5 = small sprites, not used yet); all verified against the running game |
| Player movement/animation (observed) | Frames per direction and screen motion measured in the running game (`formats.md` §6.11); **walk demo** in the SDL app (floor + animated player, stays inside the room's floor area; no collision/walls/things yet) |
| Wall tiles (`MP3` block 2) and wall layouts (`MP1` section 1) | **Done** — the complete room shell (floor + walls) reproduces the running game's first frame (all remaining differences are objects/the player); Python + C, unit-tested on all rooms of 14 level dirs; room viewer + walk demo draw it |
| Screen frame (ANDATA block 1) | **Done** — wings with slots and ornament, 98.2% of the original first frame with the room composite, unit-tested |
| Object sprites (`MP1` section 2, ~80 per level) | **Done** — format verified (blocks parse exactly), Python + C, unit-tested on 14 level dirs, sheet viewer in the SDL app. blocks are stored at `floor(size/2)` bytes (all 255 blocks now delimited exactly). The 32-byte header is partly decoded (box sizes etc.) |
| Objects in rooms | **Done for static sprites** — thing → sprite block (`rec[+6]`) and the exact screen position (`object_project`, 17/17 sprites exact) reproduce level 1 room 0; Python + C, unit-tested on every room of 14 level dirs; shown by `--scene room`. The SDL walk scene (`--scene walk`) uses the game's own collision test (verified) in 1/8-cell units, room exits lead to the next room, objects are depth-sorted approximately. Animated blocks are decoded and animated (script + per-frame offsets, 25 Hz step, verified against the running game). **Not yet**: draw order for moving objects, the player entity, creature behaviour (TAD x86 code) |
| `ROUTINEn.TAD` / Cadaver 2 `ROUTINEn.PAK` | **Runs** — 8086 interpreter (Python + C, verified against Unicorn on every timer/behaviour routine of levels 1–5); timer routines are executed in the walk scene (energy, random, sound services; event-8 messages). Not yet: the behaviour (creature) routines, most engine services, the Payoff's new levels (other stub) |
| Status line, labels, messages, verbs, energy bar | **Done** — "DAY n" + room name, touched-object name, parchment message scrolls, verb icons and the energy bar are pixel-exact against the emulator (`formats.md` §6.3a, §6.7). Verb *selection* (hold End + direction) and inventory are not decoded |
| ACL interpreter | **Largely done** — event dispatch with condition tests, room events (28 first visit, 6 entry, 14/20 timers, 19), thing events (7 touch, 9 contact), ~35 ops verified against the original handlers; missing ops: spawn/move/animation/change room/inventory etc. |
| Creatures | **Walk scene**: active creatures (sprite class 3) run the real TAD behaviour routines on the interpreter, move with the engine's movement-state rules (speed gate, step size, patrol tables), spawn through op 36, hit the player (energy −tail[1]); the engine's exact velocity application and creature/creature interaction are approximated |
| Verbs, chests, level progression, death, game clock | Verbs 2/3/4/7/11/14 act (valuables → money/score, chests open, events 0/5/16); op 37 changes room, op 51 the level (money/score/energy carry over); death restarts the level; the day counter follows the real-time clock; `--scene game` starts from the title |
| Inventory (carried things) | **Partly** — take sets `rec[10] = 0xFF`; Space/Return show the carried thing (picture, name, look/drop/leave, cycling); hold/use/insert/give and the 16-slot grid not done |
| Sound (PC speaker) | **Runs** — `BEEPER.BIN` on the interpreter (`engine/beeper.c`), SDL square-wave output; walking, pushing, chest, eating, death, creature ambient, TAD sound service, intro tune. Other triggers not yet |
| Cadaver 2 (Payoff) code differences | Not analysed (data differs only in `TATA/` and RNC-packed copies) |

## What was learned about the engine (for the next steps)

* **Video**: EGA-style planar 320×200×16 (mode 0Dh). The VRAM (256 KB) is used
  as storage: several screen-sized pages (the room is rendered into a buffer
  larger than the 320×136 viewport and scrolled), plus the sprite sheets in the
  last ~600 rows (column-planar layout), all blitted with the VGA latches and
  AND/OR masking. The bottom panel is a fixed picture (inventory frames, "DAY n",
  location name).
* **Memory after loading level 1** (measured in the harness): `LEV1.MP1`
  decompressed at `2917:0000`, `ENG1.LNG` at `6C28:000A`; other blocks of `MP1`,
  `MP2`, `ANDATA` are copied into other segments.
* **Files are loaded through one coroutine-based reader** (`1000:C7D1`, header
  at `1000:C8F1`), so every `mjm` file passes the same code: a good hook point
  for "what is this file used for".
* **Game start-up**: read `CADAVER.INI` → load sound driver (`BEEPER.BIN`) →
  decompress `ANDATA.DAT` and `PCMAN.DAT` → language prompt (`1/2/3`) →
  "restore game?" prompt (`0`–`9`/Esc; saves live in `data\mike_save`) → level
  files for the current disk/level → `ENGn.LNG`.

## Renames

Every routine/variable/string identified goes into `renames/<game>/<stem>.tsv`
(see `renames/README.md`) so it can be applied to the local IDBs; currently
`cad` 50 rows, `intro` 5, Cadaver 2 `game` 38 and `intro` 5, including the
mjm/DCL loader chain, the 50 Hz timer ISR, keyboard helpers, VRAM latch copy,
the PC1 decoder and the file-name strings. New findings add rows as we go.

## Suggested order of work

1. ~~Language strings~~ — **done** (`tools/lng.py`, `src/engine/lng.c`, `formats.md` §5).
2. **Level data** — structure of `MP2` (four sections) and `ANDATA` (six blocks)
   is known; `MP1`, `MP3` partially. Next: find the *consumers* — hook reads of
   the expanded tables (`ds:3DDA` rooms, `ds:5194`, `ds:5E13`, `ds:65E4`) with
   the harness while walking the player (`Emu.scan_queue`) and histogram
   `(cs, ip)`; name each field from how the code uses it. Compare compact records
   (file) with expanded ones (memory) to learn the expansion rules. `ANDATA`
   block 0 (8-byte entries) should give the sprite frame rectangles for
   `PCMAN.DAT`, whose placement in VRAM is already known.
3. **Renderer** — **room shells (floor + walls) are done** (see the status table); things are now placed exactly (`formats.md` §6.12); still missing: animated sprites, the exact draw order, the player, the status text; the screen frame (bottom wings with the slots and ornament) is **ANDATA.DAT block 1** (found by tracing which routine writes the lower screen and which memory it reads; §6.3). Original note:  the sprite format and the blit algorithm are known (formats.md §6.1/6.2); what is missing is *where room backgrounds and creature/object sprites come from* (they are not in the VRAM sprite area: only `PCMAN.DAT` is). — port the planar blitters to chunky C (masked blits by
   reading the AND/OR planes), driven by the object list. Match the harness
   frame pixel-for-pixel for level 1's first room.
4. **Object/ACL layer** — decode the opcode table (jump table in `seg000` near
   `1000:7AE0`) and the `ROUTINEn.TAD` tables; run the scripts in C.
5. **Player, input, timing** — keyboard map from `CADAVER.INI` (arrow keys,
   `J`/`G` for fire in Cadaver 1/2), 18.2 Hz tick, movement/collision.
6. **Sound** — *PC speaker done*: `BEEPER.BIN` runs on the 8086 interpreter
   (50 Hz tick, effect request/stop protocol of `1000:156C/1613`), SDL plays
   the resulting square wave; walking, pushing, chests, eating, death and the TAD
   sound service trigger sounds, the intro tune plays. Missing: the other call
   sites (creature hits, items) and the AdLib/SoundBlaster/Roland drivers.
7. **Cadaver 2** — diff `GAME.EXE` against `CAD.EXE` function by function
   (the data directories are shared, so most of the engine should be identical)
   and add the `TATA/` levels and Roland support.

## Notes for a ScummVM-style engine module

`overview.md` states that the long-term target is a ScummVM engine module. The
`src/engine/` code is deliberately free of platform calls (only `gamefiles.c`
touches the filesystem; `main.c` is the sole SDL user), so it can be ported to
ScummVM's `Common::File`/`Graphics::Surface` layer later.
