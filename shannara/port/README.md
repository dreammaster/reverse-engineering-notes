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
python port/tests/world_fuzz.py 6000 1   # random object-tree/attribute/flag operations: C++ World vs the ORIGINAL functions of SHANNARA.EXE run under unicorn
```

Status when written: `verify.py` 63 checks, 0 failed; `fuzz_q.py` 300 (seed 1) + 1,200 (seed 7) vectors, 0 mismatches; `world_fuzz.py` 6,000 operations x 2 seeds, 0 mismatches (the original code is executed from the exe by `reversing/scripts/retail_emu.py`, which applies the LE fixups). The Python
Q decoder was itself fuzz-verified against the real machine code under unicorn, so the chain is
real code -> Python reference -> C++.

Notes for engine integration: palettes are stored as 6-bit VGA components (`<< 2` for 8-bit output); Q frames use a
fixed 640-byte pitch surface; `ShanStr::string()` keeps the trailing NUL symbol exactly like the Python reference (message
text ends with `\v` or NUL depending on the string).
