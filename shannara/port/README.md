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
| `src/qvideo.h` | `.Q` block video (the demo's cutscene format): container + 4x4 block decoder incl. composite and raw modes | `q_decode_ref.py`, `q_play.py`, `analysis/q_codec_spec.md` |

Not ported yet: XMIDI directory (`.MUS`), LHA `-lh4-` (no real file uses it), savegame/`OBJECT.DAT`, retail `.VOC`.

## Build

```
cmake -S port -B port/build-cmake && cmake --build port/build-cmake
# or, without cmake:
g++ -std=c++17 -O2 -Wall -Wextra -Iport/src port/src/*.cpp -o port/build/shan_tool.exe
```

`shan_tool` is a small front end: `shan_tool shanstr|pic|rgn|fnt|q FILE`, plus `qvec` for the fuzz vectors.

## Tests (differential, against the verified Python references)

```
python port/tests/verify.py      # every real file: 7,719 strings, 29 PIC, 12 RGN, 13 FNT, 270 Q frames (needs game/, demo/, game_out/pics)
python port/tests/fuzz_q.py 1200 7   # random valid Q streams: C++ vs Python decoder, pixels and consumed bytes
```

Status when written: `verify.py` 57 checks, 0 failed; `fuzz_q.py` 300 (seed 1) + 1,200 (seed 7) vectors, 0 mismatches. The Python
Q decoder was itself fuzz-verified against the real machine code under unicorn, so the chain is
real code -> Python reference -> C++.

Notes for engine integration: palettes are stored as 6-bit VGA components (`<< 2` for 8-bit output); Q frames use a
fixed 640-byte pitch surface; `ShanStr::string()` keeps the trailing NUL symbol exactly like the Python reference (message
text ends with `\v` or NUL depending on the string).
