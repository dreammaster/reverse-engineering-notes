# Legend "Later engine" format readers (C++17)

Ports of the Python readers in `../reversing/scripts`, verified against the Companions of Xanth game files.
No dependencies; intended to be dropped into a ScummVM engine (wrap `Common::SeekableReadStream` instead of `readFile`).

| Class | File | Reference | Verified on |
|---|---|---|---|
| `dclExplode` | PKWARE DCL implode (PIC pixels) | `dcl_explode.py` | all 3,294 pictures decode to exactly width*height |
| `PicFile` | `XANTH_nn.PIC` | `xanth_pic.py` | 27 files; pixels/palette identical to Python |
| `RgnFile` | `XANTH_nn.RGN` | `rgn_read.py` | 17 files, region id == object id |
| `MessageFile` | `XANTHSTR.DAT` | `shanstr.py` | 80 blocks / 5,542 strings identical to Python |
| `MusFile` | `XANTH_nn.MUS` | `xanth_mus.py` | 86 MIDI records, sizes consistent |
| `RsFile` | `*.RS` (29 files) | overview section 15 | header, length and full-scale consistent |
| `VocFile` | `XANTH_01.VOC` | overview section 10 | 1,326 tracks, last offset == file size |

```
g++ -std=c++17 -O2 -o legend_tool legend_formats.cpp legend_tool.cpp
./legend_tool check ../game          # decode everything, exit status 1 on any failure
./legend_tool pic ../game/XANTH_02.PIC 0 bedroom.ppm
./legend_tool str ../game/XANTHSTR.DAT 13 6
./legend_tool mus ../game/XANTH_01.MUS 1 0 tune1.mid
./legend_tool voc ../game/XANTH_01.VOC 3 line3.wav     # assumes 22050 Hz (rate not stored in the file)
```

`DclExplode` is the same algorithm as Mark Adler's `blast.c` (zlib contrib); ScummVM already has an equivalent (`common/dcl.h`, `Common::decompressDCL`) which can replace it.
Shannara's retail `.PIC` pixels are stored raw: `PicFile::decode(i, false)`.

## gen/ - handler prototype (generated)

`../reversing/scripts/gen_handlers.py` translates the handler digest (`analysis/xanth_handler_digest.txt`) into C++:
`gen/handlers_gen.cpp` has 647 of the 707 object handlers / exit functions / conversation callbacks / dynamic texts (`intptr_t objh_NNN(int phase, ...)`),
`gen/handlers_api.h` declares the 841 game variables and 95 engine functions they use (`Font_WriteMessage`, `object_move?` -> `object_move_q`, ...) plus the
`Command` enum and the `MSG(block, index)` macro, and `gen/handlers_todo.txt` lists the 60 handlers that use loops or far-pointer structure accesses (`es:[bx+..]`) and still need hand work.
The result compiles (`g++ -std=c++17 -fsyntax-only gen/handlers_gen.cpp`) but is only as faithful as the digest (a path-trie summary, not a decompilation); it is the skeleton
to port against, with the host providing the declared variables and functions.
