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
`gen/handlers_gen.cpp` has 607 of the 707 object handlers / exit functions / conversation callbacks / dynamic texts (`intptr_t objh_NNN(int phase, ...)`),
`gen/handlers_api.h` declares the 841 game variables and 95 engine functions they use (`Font_WriteMessage`, `object_move?` -> `object_move_q`, ...) plus the
`Command` enum and the `MSG(block, index)` macro, and `gen/handlers_todo.txt` lists the 100 handlers that use loops or far-pointer structure accesses (`es:[bx+..]`) and still need hand work.
The result compiles (`g++ -std=c++17 -fsyntax-only gen/handlers_gen.cpp`) but is only as faithful as the digest (a path-trie summary, not a decompilation); it is the skeleton
to port against, with the host providing the declared variables and functions.

### How the 707 handlers are produced (all of them are now covered)

`reversing/scripts/rebuild_handlers.sh <scratch>` runs the whole pipeline:

1. `extract_xanth_digest.py` builds the symbolic digest; `gen_handlers.py` turns it into C++ (`gen/handlers_gen.cpp`).
2. `lift_handlers.py` lowers the same functions instruction by instruction from `xanth.asm` (registers and flags in `gen/cpu.h`, gotos for jumps, jump tables as `switch`, tail jumps into IDA chunks followed, calls to engine functions as `H_<name>(Cpu &)` reading cdecl arguments from the emulated stack). The 707 lifts total 66,941 instructions and have no unresolved operands.
3. `make_trace_test.py` runs both versions of every handler under mock hosts that record the engine functions called (all-zero state, 14 phases x 12 verbs) and compares the sequences. Results of the last run: 607 of 647 digest translations agree with the lift in every one of 101,976 scenarios; 40 differ (the digest merges paths whose conditions it cannot keep).
4. The 40 disagreeing handlers (`gen/handlers_mismatch.txt`) and the 60 the digest could never express are taken from the lift: `gen/lifted_handlers.cpp` (100 handlers, 18,639 instructions, `lifted_api.h` / `lifted_data.h` / `cpu.h`). The other 607 stay as readable C++ in `handlers_gen.cpp`.

The trace test only compares the *sequence of engine calls* in one state, so it catches structural errors (it found: verb names compiled as undeclared variables, duplicated nested calls, hoisting of repeated tests, and wrong jump-table case values), not wrong arguments or flag values. The lifted code is exact as far as the 8086 semantics in `cpu.h` are modelled (no `pushf/popf`, no interrupts, DF assumed clear). Neither set has been run inside a real game.
