# Tooling

Everything here runs on Linux with Python 3 (`pillow`, `numpy`, `unicorn`),
`ndisasm`, optionally DOSBox + Xvfb. Nothing needs IDA; the IDA exports in
`disasm/` are read-only inputs.

| Tool | What it does |
|---|---|
| `tools/mjm.py` | Decode `mjm0/1/2` files (PKWARE DCL, RNC1). `python3 tools/mjm.py FILE...` prints sizes; import `mjm_unpack` for scripting. |
| `tools/rnc1.py` | RNC method 1 decompressor (used by `mjm.py` and for the Cadaver 2 `.BIN` drivers). |
| `tools/objsprites.py` | Object sprite blocks of `LEVn.MP1` section 2 → sprite sheet PNG (`python3 tools/objsprites.py cadaver1/DATA/1/LEV1.MP1 sheet.png`). |
| `tools/objplace.py`, `tools/room_objects.py` | Thing → screen position (the game's `object_project`, verified) and a room renderer with objects (`python3 tools/room_objects.py cadaver1/DATA/1 0 room.png`). |
| `tools/roomview.py` | Render a room's isometric floor (`python3 tools/roomview.py cadaver1/DATA/1 0 room0.png`); also the reference decoder for tiles, floor maps, wall records (`render_shell` draws floor + walls) and the level palette. |
| `tools/pcman.py` | `PCMAN.DAT` → sprite sheet PNG (`python3 tools/pcman.py cadaver1/DATA/PCMAN.DAT sheet.png`). |
| `tools/things.py` | Dump a level's room lists and things by name (`python3 tools/things.py cadaver1/DATA/1 [--things]`). |
| `tools/lng.py` | `.LNG` language tables → text (`python3 tools/lng.py cadaver1/DATA/1/ENG1.LNG [index]`). |
| `tools/mpsections.py` | Walks the section structure of `LEVn.MP2` and the block layout of `ANDATA.DAT`; flags the truncated Payoff file. |
| `tools/pc1.py` | `*.PC1` → PNG (`python3 tools/pc1.py cadaver1/LOGO.PC1 out.png`). |
| `tools/dosemu.py` | **Emulator harness** (Unicorn): runs an *unpacked* original EXE with a DOS INT 21h subset, keyboard/timer feeding and a planar EGA/VGA model. Lets us run the game's own loaders and renderer and inspect memory/VRAM. |
| `tools/emu_run.py` | Convenience driver around `dosemu.py`: feeds keys, writes a PNG every 25 M instructions, and dumps conventional memory (`*_mem.bin`) and VRAM (`*_vram.bin`) at the end. |
| `tools/dosbox_capture.sh` | Runs the *retail* game in real DOSBox on a virtual X display, sends keys, takes native-resolution screenshots. The independent reference to compare the emulator and the C port against. |
| `tools/renames.py`, `tools/renames_lib.py` | Maintain the rename tables in `renames/` (check / fill / port between the two games / merge renames exported from IDA / stats). See `renames/README.md`. |
| `ida_scripts/apply_renames.py`, `ida_scripts/export_renames.py` | IDA 8.3 scripts (run through `run_ida_script.ps1`): apply a table to your local `.idb`, or dump the names already in an `.idb`. |
| `tools/test_renames.py` | Tests for the above (the apply script is run against a fake IDA API backed by the real binaries). |
| `tools/gen_test_vectors.py` | Regenerates `src/tests/vectors.h` (sizes + FNV hashes from the Python decoders) for the C unit test. |
| `tools/status.py` | Reference renderer for the status line ("DAY n" + room name, ANDATA block 4 font; formats.md §6.3a). |
| `tools/message.py` | Reference renderer for message scrolls (layout, box, text; formats.md §6.7). |
| `tools/verbs.py` | Reference for the verb list / verb icons shown in the panel slots while touching an object (formats.md §6.7). |
| `tools/statuspanel.py` | Reference renderer for the character status dialog (health, gold, XP, level, completion, rank; formats.md §6.7). |
| `tools/smoke_walk.py` | Headless smoke test of the SDL walk scene over the creature rooms of every level of both games (crash detection). |
| `tools/x86.py`, `tools/verify_x86.py` | 8086 interpreter (Python reference) for the native routines in `ROUTINEn.TAD`, and its verification against Unicorn running the same routines (timer + behaviour entries of all five levels, random start states, identical memory/registers/service-call traces). |

## Reproducing the in-game frame

```bash
python3 tools/emu_run.py cadaver1 unpacked/cadaver1/CAD.EXE /tmp/g1 400 0231 011b
# 0231 = key '1' (English), 011b = Esc ("don't restore a saved game")
```

After ~150 M instructions the harness shows the same level-1 cavern frame as
DOSBox (`tools/dosbox_capture.sh cadaver1 out cadaver "6:SHOT 4:SHOT 4:SHOT 5:1 …"`).
Throughput is ≈4 M instr/s without video access and roughly 1 M instr/s while
the game is drawing, so a 400 M run takes several minutes.

### How the harness works / caveats

* The whole 1 MB address space is flat RAM except `A0000–AFFFF`, which is an
  MMIO window implementing a planar VGA model: 4×64 KiB planes, latches,
  write modes 0–3 (function select, rotate, bit mask, set/reset), read modes
  0/1, map mask, CRTC start address, attribute-controller palette and the DAC.
* INT 8 is injected every 20 000 instructions when the game has hooked it. Raw
  scancodes queued in `Emu.scan_queue` (make = `sc`, break = `sc|0x80`) are delivered
  one per slice through the game's INT 9 handler and port 60h, so the emulated
  player can be walked around (the object names in `formats.md` §6.4 were captured
  this way: hook `text_get_string`, `1000:75AF`, and print DX and the registers).
  `INT 16h` keys given as `pending_keys` are released only to **blocking**
  reads (AH=0/10h) — the game flushes the keyboard buffer with non-blocking
  peeks before each prompt, so releasing earlier loses the key.
* Only the INT 21h/10h/16h/1Ah calls these games use are implemented; unknown
  ones are logged with `verbose=True`. Sound drivers are loaded (the game
  reads `BEEPER.BIN`) but no audio is produced; `tools/beeper.py` runs the driver on `tools/x86.py`
  (`python3 tools/beeper.py EFFECT [TICKS]` prints the divisor changes, `python3 tools/beeper.py wav ID|music OUT.wav [SECONDS]` writes a WAV) and `tools/verify_beeper.py` checks it against Unicorn.
* Useful pattern for finding a decoder without reading the unlabeled disassembly:
  subclass `Emu`, override `on_read` to learn where a file's bytes landed, then
  `uc.hook_add(UC_HOOK_MEM_READ, …)` over that range and histogram
  `(cs, ip)` of the readers. This is how the `.PC1` decoder (`INTRO.EXE
  0B09`) was found in one run.
* Segment values printed by the harness are real load segments (the image is
  loaded at `1000:0`, so `1000:xxxx` equals IDA's `seg000:xxxx`).

## Building the C port

```bash
cmake -S src -B build && cmake --build build
ctest --test-dir build --output-on-failure   # skips if retail data absent
./build/cadaver -g cadaver1                   # or cadaver2
```

## Fuzzing the port (sanitizer build)

```bash
cmake -S src -B /tmp/asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS='-fsanitize=address,undefined' && make -C /tmp/asan
python3 tools/fuzz_sim.py SEED COUNT      # random --sim key sequences over both games and all levels
python3 tools/fuzz_items.py SEED COUNT    # the same with a carried spell / potion / weapon (CAD_CARRY, CAD_SPELLS lists them), held, fired, cast, drunk and S / L prompts
```

Debug environment variables of the port: `CAD_CARRY=thing[:A0[:A1]]`, `CAD_SPELLS`, `CAD_LOOKTEXT=thing`, `CAD_CLASS=thing`, `CAD_MOVE=thing:x:y`, `CAD_SHOT=thing`, `CAD_CREAT=thing|-1`, `CAD_ENERGY=n`, `CAD_SNDLOG`, `CAD_OPS`, `CAD_OPTRACE`, `CAD_ENDING` (the ending screen), `CAD_DEATH=thing` (event 23 for a thing at tick 8), `CAD_FINDBEH=n` (things whose behaviour byte is n), `CAD_FINDF7=n` (things whose animation script runs action n), `CAD_SCRIPT=thing` (a thing's animation script), `CAD_HDRS`, `CAD_BOXES`, `CAD_DUMPBOX`, `CAD_POSLOG` (with `CAD_SNDLOG`: every player step), `CAD_TADDIFF`, `CAD_VARS`.

`tools/roland_midi.py` runs `ROLAND.BIN` on the Python 8086 interpreter and writes the driver's MPU-401 output as a Standard MIDI file (`python3 tools/roland_midi.py music 1 tune.mid 60`; effects 1-8 with `effect N`).

`tools/scan_tad_errors.sh` runs every room of both games headless and lists failures of the native routines (`CAD_TADERR`); `CAD_X86TRACE=1` traces every instruction and `CAD_X86WATCH=<hex offset>` reports writes to a byte of the TAD image (this found the Payoff stack / code overlap).

`tools/gen_place_vectors.py` runs the original drop-spot routine (`1000:6C72`) in the emulator for 480 random cases and writes `src/tests/vectors_place.h`; `test_formats` checks `engine/collision.c place_spot` against them.

`tools/gen_zone_vectors.py` does the same for the room zone overlap test (`1000:2022`): 400 random cases into `src/tests/vectors_zone.h`, checked against `engine/collision.c zone_overlap`.

`tools/gen_grid_vectors.py`: 2080 cursor moves of the whole-rucksack grid (every size 1-32, the four keys, many states) through the original key handler into `src/tests/vectors_grid.h`; `engine/ruckgrid.c` must reproduce them.

`tools/gen_describe_vectors.py`: 300 random spell / potion / weapon / ammunition structs through the original description routine (`1000:84D9`, level 1 English) into `src/tests/vectors_describe.h`; `engine/describe.c item_description` must reproduce the texts.

`tools/gen_verb_vectors.py`: 600 (carried thing, touched thing) pairs from levels 1-3 through the original carried-verb builder (`1000:12C72`) into `src/tests/vectors_verbs.h`; `engine/panel.c carried_verbs_list` must give the same lists (up to 8 verbs: the port shows them in two pages of six).

`tools/verify_walk.py LEVEL ROOM SEED [N]` compares random walking segments of the port (`--sim ... dump:0`) with the original (emulator, `room_enter` + scancodes) per room: many rooms agree exactly (positions, exits, pushes), the harness stalls in some runs (the emulated game opens a scroll screen after `room_enter` was called by hand), so it is a screening tool. `tools/verify_climb.py LEVEL FIRST LAST` compares the climbing steps, `tools/verify_grab.py` the grab / pull. `CAD_BOXES=1` prints the collision boxes of a room, the sim token `dump:0` the player position and room.

`tools/verify_intro_roll.py cadaver1|cadaver2` compares the story roll with the running `INTRO.EXE` at every page-break wait (the emulator saves a frame and releases the wait); `CAD_INTRO_T=seconds` starts the port's intro roll that far in, `CAD_INTRO_DBG=1` prints the script size.

`tools/verify_push2.py SEED [N]` compares random walking / pushing segments in the start room of level 1 (the original booted normally, segments counted in game steps through a hook on `game_step`, a 44-tick settle after each segment so that the 17-step room events of both programs have run, creatures left out): after every segment the player position and the six-byte collision boxes of all objects (`CAD_DUMPBOX=1` prints them for the port) are compared; a one-step difference is `drift` and the original is re-synchronised with the port's state, anything larger is `DIFF` and stops the run. Seeds 1, 3, 4, 5, 11 and 12 run through without a difference (the pushing of the coin and the diary into the walls included). Known limits: the water creature can kill the player in the original at the pond (the emulated game then stops calling `game_step`: reported as `stalled`; the port's creature behaviour differs there), and the re-synchronisation can leave the original's object records inconsistent after a drift (seed 6 / 10: thing 60 ends in different places).

`GAME=cadaver2 tools/verify_push2.py SEED [N]` runs the same differential for the **Payoff** (GAME.EXE, TATA level 1 room 0, the player starts at (48, 22), `START=x:y` overrides): seeds 1–4 and 6–10 (8–14 segments each) run through without a difference after the locked-door fix (the first run showed the port leaving by an exit whose door word is `FFFF`); seed 5 ends in a `DIFF` caused by a small self-moving object that is one pixel off in phase (the exact mismatches are listed after a `DIFF`). Stalls (`Stalled`) are the same pond / creature case as above. Room changes were compared the same way: with the lock word of door 0 cleared in both programs (`--sim unlock:0` in the port, the word at `ds:5194 + 2` in the emulator) a walk left from (48, 22) leaves room 0 at x = 13 and arrives in room 8 at y = 36, x = 62–64 in both (the port's landing rule `room_landing` agrees).

`tools/verify_door_targets.py LEVELDIR...` checks the target room of every door statically: the original's own rule (`15C53`: the door's map cell against the four sides of each room record, first room other than the current one) and the port's rule (the other room that lists the door id) give the same room for all about 2,100 doors of levels 1–5 of both games and the four TATA levels.

`tools/verify_exits_tata.py [LEVEL]` is the exit-box oracle for the Payoff's own levels: GAME.EXE is booted to TATA level 1 (the level is forced with `ds:708` before the file-name patcher `1000:CB24` for 2–4), the room-entry routine `1000:DAB1` is called for every room and the exit entries at `ss:[ds:3A]` are compared with `tools/roomexits.py` (= the port's `room_build_exits`): **TATA 1: 32 rooms, TATA 2: 70, TATA 3: 53, TATA 4: 44 agree exactly** (two rooms of level 4, 17 and 22, make the emulator fault on an unmapped read and are skipped).

`tools/verify_landing_tata.py [LEVEL]` walks through every door of a TATA level in the original (the player is put next to each exit, the lock word of the door record is cleared) and compares the landing place with `room_landing`: **TATA 1: 44 of 45 crossings agree** (11 more did not cross within 80 ticks); levels 2–4 cannot be run this way (the forced-level boot never reaches the game step in the emulator within 6000 ticks; the exit boxes of those levels are compared by `verify_exits_tata.py`). The one difference (room 17, door 17, east exit) is not a landing formula problem: the original **refuses the crossing** and the player stays at the door. The room-entry routine (`1000:DAB1` in the Payoff, `DA60` in CAD.EXE) places the player at the landing spot and tests it with the collision routine; when objects overlap it, it tries to **move those objects away** (`11D06`: a table-driven search for a free spot in the directions allowed by the last move, in growing steps, per blocking object; each moved object gets its engine entry updated by `15353`) and tests again; when the first hit is a wall / exit or a solid object (`[hit + 1] = FDh`), or an object cannot be placed, the routine returns with the sign set (`ax = FFFDh`) and the caller (`1000:0A35`) puts the player back in the old room. The port instead keeps the objects and moves the player to the nearest free spot (`walker_find_free`), so a thing lying at a door's landing spot behaves differently (rare in the shipped rooms; not ported). In the case found the landing spot in room 18 is (14, 44) and the creature 227 (class 3, box x 8..15, y 32..39, z 0..10) stands over it at the start of the room: the original does not let the player in through that door while the creature is there.

`tools/verify_cross_c1.py LOG` runs the port through every door crossing recorded by `tools/oracle_landing.py` (Cadaver 1 level 1, original in the emulator) and compares the room reached, refusals by locked doors included (the oracle does not clear lock words, the port keeps them too): rooms 0–13, **28 crossings, all agree** after the arrival rule was ported (the one apparent difference, room 12 door 66, is the port's 30-tick walk continuing through the next door; the landing in room 13 at x = 20 is the original's). Note that `oracle_landing.py` run over all rooms floods its log with emulation errors after the first rooms (an unmapped read at `1000:A78C`; it only works for the first dozen rooms here), so it is run on a room range. The 11 doors of TATA level 1 that show "no crossing" in `verify_landing_tata.py` are explained: the oracle clears the lock words right after `DAB1`, but the room-entry event queued by `DAB1` (`4006h`, room event type 28) has not been delivered yet; a write watch on the word (room 20, door 20) shows `1000:7C27` — ACL op 1Bh, "set the door's word" — writing `FFFF` back at the next game step. The port runs the same room script when a room is entered (`CAD_OPTRACE=1`: `ROOM 20 event type 28 … op 27: 14 ff ff`): these rooms lock their door on entry, and a test that unlocks the door afterwards (`--sim unlock:-1` after the room is up) is the only way through in the port as well.

`tools/make_orig_save.py OUT [LEVEL] [WALK]` makes a save file with the original game in the emulator (walk with key:ticks pairs, F5, `Y` to pay; the disk / slot prompt `1000:C1AB` is skipped by a hook that is installed before the boot but only acts once armed — the same routine is the restore prompt at start-up — and the game directory is a symlink copy in a temp dir) and prints the engine entries at that moment; `tools/verify_origsave.py [LEVEL] [WALK]` loads the file in the port and compares the player's place and the collision boxes of the room's things: **level 1 at the start, level 1 after walking to the diary and taking it (17 things, the player at (59, 34)) and level 3 all agree.** Loading is described in `docs/formats.md` (the original's save file). `GAME=cadaver2` makes and checks a Payoff save the same way (TATA level 1 start and after a walk to (29, 31): both agree).
