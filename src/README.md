# Cadaver C reimplementation

Work in progress, but largely playable: both games' levels run in the SDL front end (`--scene game`) with the room pictures, the player, the original's collision and pushing rules, doors and level changes, the ACL scripts and timers, the creatures (their TAD behaviour routines run on the 8086 interpreter), messages, labels, the verb menu and the rucksack (small view and scrolling grid), chests, held / thrown things, spells, potions and throwing weapons with flying projectiles, death and restart, the map, pay-to-save slots, and sound for all three PC drivers (speaker, AdLib, Sound Blaster) with the decoded triggers. Not done: the intro's story pages, some verbs and a few spell / potion effects (see `docs/roadmap.md`), the original's own save format, Roland music (exportable as MIDI with `tools/roland_midi.py`). Currently:

| File | |
|---|---|
| `engine/mjm.[ch]` | `mjm` container + PKWARE DCL explode + RNC1 unpacker |
| `engine/pc1.[ch]` | `.PC1` picture decoder |
| `engine/objsprites.[ch]` | object sprites (`MP1` section 2) |
| `engine/roomobjs.[ch]` | thing → screen projection (`object_project`), collision boxes, runtime room objects with animation, drawing |
| `engine/anim.[ch]` | sprite animation scripts (the game's step + command table) |
| `engine/collision.[ch]` | the game's box collision test (verified against the original routine) |
| `engine/roomexits.[ch]` | door boxes / exits of a room (verified for all rooms of levels 1-5) |
| `engine/acl.[ch]` | ACL script interpreter (about 80 of the 94 ops, event dispatch with conditions, timers, spawn / shot helpers, effect timers) |
| `engine/push.[ch]` | pushing objects |
| `engine/x86.[ch]` | 8086 interpreter for the native routines of `ROUTINEn.TAD` / `.PAK` |
| `engine/tad.[ch]` | TAD runtime: data-segment image, service table (energy, random, sound, timers, thing lookup, shots), countdown timers, behaviour and action routines |
| `engine/beeper.[ch]`, `engine/opl.[ch]` | sound drivers (`BEEPER` / `ADLIB` / `SBLASTER.BIN`) run on the interpreter (50 Hz tick), the play / stop protocol of `1000:156C/1613`, square-wave synthesis, an approximate OPL2, the Sound Blaster DSP / DMA samples |
| `engine/message.[ch]` | message scrolls: wrapped text in a parchment box (ANDATA blocks 2 and 3) |
| `engine/panel.[ch]` | `ANDATA.DAT` block 1 (the frame around the room view) and block 4 (status text: "DAY n", room name) |
| `engine/intro.[ch]`, `introroll.c` | title scroll and the rolling story / credits of `INTRO.EXE`: `FONT2.SPR` text, `FLAME.DAT` flames over `LOADING2.PC1`; the text is read from the packed `INTRO.EXE` at run time |
| `engine/exeimg.[ch]` | memory image of a self-extracting DOS executable (PKLITE, Pro-Pack): the stub runs on `x86.c` |
| `engine/rooms.[ch]` | floor tiles + wall tiles (`MP3`), floor maps, wall records + palette (`MP1`), room shell (floor + walls) renderer |
| `engine/sprites.[ch]` | `PCMAN.DAT` player sprites (48 frames) and the game palette |
| `engine/mp2.[ch]` | `LEVn.MP2` section tables: script records, room lists, things |
| `engine/lng.[ch]` | `.LNG` language string tables (6-bit packed text) |
| `engine/gamefiles.[ch]` | detects Cadaver / Cadaver: The Payoff, resolves DOS-style paths case-insensitively, loads + unpacks files |
| `engine/screen.h` | 320×200 indexed framebuffer + palette |
| `main.c` | SDL2 front end (title / scroll pictures and the player sprite sheet; Esc quits, any key switches scene; `--scene game|logo|loading|sprites|sprites2|room|tiles|walk|intro|objects [--level N --room R --tata --scroll Y --sim right:30,down:10]`; in the room scene PgUp/PgDn = room, `[`/`]` = level, `T` = DATA/TATA (Payoff), arrows scroll; in the walk scene the arrow keys move the player along the room axes with the original collision test, exits change room (headless: `--sim right:200`); End opens the verb menu next to an object. `--scene game` plays the title scroll and starts level 1 on Space/Return) |
| `tools/cadaver_unpack.c` | command-line unpacker (`cadaver_unpack in out`, `-t` to test) |
| `tests/` | `test_formats` compares the decoders with vectors from the Python reference (`tools/gen_test_vectors.py`) |

```bash
cmake -S src -B build && cmake --build build
ctest --test-dir build --output-on-failure
./build/cadaver -g cadaver1 [--scene loading] [--screenshot out.bmp]
```

Verification tools (Python, need Unicorn and the unpacked executables): `tools/verify_collision.py`, `tools/oracle_rooms.py` +
`tools/verify_exits.py` / `tools/verify_objects.py` compare the reimplementation with the original CAD.EXE running in an emulator.
See `docs/roadmap.md` for what is missing.

## Keys (`--scene game` / `--scene walk`)

Arrows (and Insert/Delete/PgUp/PgDn for diagonals) move, End = fire (take the touched object's verb menu / jump), Space or Return = rucksack, F1 = map (arrows scroll, Return zooms in, `-` out, End leaves), `I` = status dialog, H = hold the last handled carried thing (fire then throws it), F5 = save a position for gold (also S; F6 / L = load a slot), F10 = free quick save, F9 = quick load, F7 = pause, F8 = clear the messages, (price = max(previous price, 50 × level − 45), Y to pay, then a slot 0–9; the next price rises by 6 × level), **L = load a slot 0–9**, C = clear the messages, Pause = pause, F4 = four / eight direction movement (`cadaver.sav` in the working directory: the whole script VM plus the player's place and the clock; the original's `data\\mike_save` format is not decoded). `--sim` tokens (also `key:s key:y key:3 ...` for the prompts, `money:N` and `tp:X:Y[:Z]` to put the player somewhere): `right left up down rdown ldown rup lup end rend space ret wait info map save load`.

**Saved games of the original** (the files the original writes, e.g. `data\mike_save…`) load too: copy one to `cadaver.sav` (or `cadaverN.sav`) in the working directory and press **L** (slot) — the level, room, things, door locks, carried items, money, energy, timers and clock come from the file; `tools/make_orig_save.py` / `tools/verify_origsave.py` make and check such files with the original game in the emulator. Doors are locked as in the original: a door whose lock word is `FFFF` stays shut, a keyed one needs the key thing in the rucksack, and the player is refused at a door when a wall, a solid thing or a creature stands on the other side's landing spot.

With Cadaver 2 installed, `--scene game` starts Cadaver (DATA) with Space/Return and The Payoff (TATA) with `P` (or `--tata`); finishing Cadaver's last level continues into The Payoff's first.

The window keeps the original's 4:3 shape (the 320×200 picture was shown on a 4:3 monitor); F11 toggles full screen.

`--lang eng|fre|ger` picks the language file (default English); accented letters may be drawn with the wrong glyph (the status font table for them is not checked).

`--sound adlib|sblaster|beeper` chooses the sound driver (default: the first letter of `CADAVER.INI`); AdLib uses a small approximate OPL2 synthesiser (`engine/opl.c`). `cadaver_sound GAMEDIR beeper|adlib ID|music out.wav [seconds]` renders a sound to a WAV file.
