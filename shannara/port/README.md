# Shannara resource readers (C++17)

Standalone, dependency-free C++ readers for the Shannara (Legend Entertainment, 1995) file formats that have been
reverse engineered and **validated on real data**. They are written in ScummVM style (tabs, `_member` names, no exceptions
other than `FormatError`, no STL streams) so they can be dropped into an engine module later; nothing here depends on ScummVM.

| Header | Format | Source of truth |
|---|---|---|
| `src/shanstr.h` | `SHANSTR.DAT` Huffman + phrase-dictionary message strings (7,719 strings) | `reversing/scripts/shanstr.py`, analysis section 16/20 |
| `src/pic.h` | `SHANxxx.PIC` containers: directory, frame offsets, palette, raw 8-bpp pixels, XOR sprites | `pic_scan.py`, section 12/22 |
| `src/rgn.h` | `SHANxxx.RGN` hotspot rectangles and polygons | `rgn_read.py` |
| `src/fnt.h` | `SHANxxx.FNT` bitmap fonts (mono and 8-bit colour, proportional tables) | `fnt_read.py` |
| `src/mus.h` | `SHAN001.MUS` / `Demo001.mus` music archives (each track a complete XMIDI file) | analysis section 25 |
| `src/objectdat.h` | `OBJECT.DAT` name/description string pool (761 strings) | section 21 |
| `src/le.h`, `src/retail_data.h` | DOS/4GW `LE` loader (linear address -> file bytes, no fixups) and the exe's initial static tables: **744 object records** and the room picture/music tables, read straight from the user's `SHANNARA.EXE` | sections 4, 5, 11, 29 |
| `src/savestate.h` | the complete mutable game state and its 32,668-byte savegame/undo format (objects + 92 regions); loads the shipped `HELP.DAT` (legacy 32,660 bytes) and round-trips it byte for byte | sections 6, 23 |
| `src/world.h` | the object world model every handler uses: containment tree (location/slot/next/child lists, `move`), attribute bits, the 800 game flags | disassembly of `object_*`, `bitset_*`; **differentially tested against the real machine code** |
| `src/text.h` | `print_obj`: articles, pronouns, names and verb conjugation for the `%s` inserts in every message | disassembly of `print_obj` (0x1C0E8); **verified against the real function** (`text_fuzz.py`, 12,000 calls) |
| `src/qvideo.h` | `.Q` block video (the demo's cutscene format): container + 4x4 block decoder incl. composite and raw modes | `q_decode_ref.py`, `q_play.py`, `analysis/q_codec_spec.md` |

Not ported yet: LHA `-lh4-` (no real file uses it), the savegame schema, FLC/MP3 glue.

## Build

```
cmake -S port -B port/build-cmake && cmake --build port/build-cmake
# or, without cmake:
g++ -std=c++17 -O2 -Wall -Wextra -Iport/src port/src/*.cpp -o port/build/shan_tool.exe
```

`shan_tool` is a small front end: `shan_tool shanstr|pic|rgn|fnt|q|mus|objdat|objects|save FILE`, plus `qvec` for the fuzz vectors.

## Tests (differential, against the verified Python references)

```
python port/tests/verify.py      # every real file: 7,719 strings, 29 PIC, 12 RGN, 13 FNT, 270 Q frames, MUS, OBJECT.DAT, 744 object records (needs game/, demo/, game_out/pics)
python port/tests/fuzz_q.py 1200 7   # random valid Q streams: C++ vs Python decoder, pixels and consumed bytes
python port/tests/text_fuzz.py 9000 5    # print_obj: C++ Text vs the ORIGINAL print_obj under unicorn
python port/tests/world_fuzz.py 6000 1   # random object-tree/attribute/flag operations: C++ World vs the ORIGINAL functions of SHANNARA.EXE run under unicorn
```

Status when written: `verify.py` 63 checks, 0 failed; `fuzz_q.py` 300 (seed 1) + 1,200 (seed 7) vectors, 0 mismatches; `world_fuzz.py` 6,000 operations x 2 seeds, 0 mismatches (the original code is executed from the exe by `reversing/scripts/retail_emu.py`, which applies the LE fixups). The Python
Q decoder was itself fuzz-verified against the real machine code under unicorn, so the chain is
real code -> Python reference -> C++.

Notes for engine integration: palettes are stored as 6-bit VGA components (`<< 2` for 8-bit output); Q frames use a
fixed 640-byte pitch surface; `ShanStr::string()` keeps the trailing NUL symbol exactly like the Python reference (message
text ends with `\v` or NUL depending on the string).

## The transpiled engine (`engine_tool`)

The game logic itself (all 1,258 translatable functions of the game layer: command dispatch, 637 object handlers, 78 verb defaults, rooms,
party, journal, score ...) can be run natively without any emulator:

```
python reversing/scripts/make_retn_map.py Shannara.asm game_out/retn_map.txt   # callee-cleaned argument sizes of library functions
python reversing/scripts/dump_insns.py ...                      # (function list + reversing/scripts/extra_entries.txt) IDAPython: decoded instructions of the game-layer functions -> JSON (needs Shannara.idb)
python reversing/scripts/transpile_handlers.py insns_all.json port/gen game_out/retn_map.txt     # static recompiler -> port/gen/*.cpp
bash port/build_engine.sh                                       # port/build/engine_tool.exe  (~25 s)
port/build/engine_tool.exe game/SHANNARA.EXE game ops.txt       # lines: cmd VERB DOBJ IOBJ | flag N 0|1 | move OBJ PARENT SLOT | goto ROOM | attr OBJ ATTR 0|1
```

`machine.h/.cpp` is the runtime (flat 16 MB image of the executable with the LE fixups applied, registers/flags/stack, shims for the UI/audio/library
boundary), `engine.h/.cpp` the front end (`Engine::command(verb, dobj, iobj, actor)` returns the original's text and result). The generated sources
are derived from the game's own code, so they are **git-ignored**; the generator, runtime and tests are committed. Tests:

```
python port/tests/engine_diff.py 3000 11      # random commands + flag/object/room/attribute changes: C++ engine vs the unicorn oracle (return value, state hash, text)
python port/tests/matrix_diff.py 7 game_out/cov.txt   # every verb x every 7th object (1 = all, ~1 h): C++ vs oracle; writes instruction coverage
python port/tests/state_diff.py 300 2 40 game_out/covs.txt   # random flags + room + items, then commands: C++ vs oracle
python port/tests/region_diff.py port/build/state_ops.txt INDEX  # which bytes of the data segment / savegame regions differ after op INDEX of the last state_diff run
python port/tests/coverage_report.py game_out/cov.txt   # fraction of handler code the run executed
python port/tests/trace_diff.py VERB DOBJ IOBJ  # first instruction-level divergence between oracle and C++ for one command (register trace)
```

Status: 12,000 operations over four seeds plus earlier runs, 0 mismatches. The shims for everything outside the game layer (graphics, sound, input, timing,
file I/O) are no-ops, exactly like the oracle's stubs, except `wprintf`/`wputchar`/`sprintf_` (text capture with SHANSTR ids and `%s`), `new_pointer` and a few C
runtime routines. Not yet covered: combat (`start_encounter?`), save/load (`load_game?`), cutscenes and dialog popups (stubbed on both sides).

`port/build/play_tool.exe game/SHANNARA.EXE game [script]` is a text front end for the same engine: type `look`, `take stick`, `put stick on pole`,
`talk to shella`, `inv`, `here`, `goto 98`, `verbs` ... (verb names come from the game's own vocabulary; the responses are the original game's). Example
transcript from the start: `take stick` -> "You pick up the stick."; `put stick on pole` -> "You tie the stick to the end of your fishing pole using some fishing line."

## Seeing a room

`play_tool` can draw the current room natively (needs the extracted `SHANnnn.PIC` files in `game_out/pics`, or `SHAN_PICS=dir`): `goto 173`, `shot room.png`, `reset` for the start state again.
The renderer (`src/render.cpp`) asks the recompiled game logic what to draw: background `get_room_pic_id`, overlay tables from the draw phase, visibility from `overlay_visible?`.
