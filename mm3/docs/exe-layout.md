# MM3.EXE layout and how `mm3.idb` is built

Rebuild everything with `powershell -NoProfile -ExecutionPolicy Bypass -File mm3\ida_scripts\rebuild_all.ps1`
(needs the installed game, IDA 8.3, Python 3).  Result: `mm3\mm3.idb` (+ `mm3.asm`, `mm3.idc`).

## Unpacking (`tools/unpack_mm3.py`)

The shipped `MM3.EXE` (280,032 bytes) has three layers (the outer one is described in `mm3-re.md`):

1. NWC LZW self-extractor (outer MZ, entry `0:0`).  Output of this stage = an **EXEPACK**ed EXE,
   127,889 bytes (`stage1`).  Its header says `cs:ip = 1EA1:0012`, i.e. the EXEPACK header sits at
   file `0x1EC10` of `stage1` (`real_ip/cs = 0:0`, `real_ss:sp = 2784:0080`, `dest_len = 278Ch`
   paragraphs, `skip_len = 1`, signature `RB`).
2. EXEPACK: the stream before the EXEPACK header is *not* all compressed.  The tail (decoded
   backwards from the last non-FFh byte: `cmd, u16 count`, `B0/B1` = fill, `B2/B3` = copy, low bit =
   last chunk) expands to 85,392 bytes; the part in front of the final chunk is stored verbatim
   (76,592 bytes).  Together they are the 161,984-byte (`278Ch` paragraphs) load image.  The 794
   relocations are stored after the text `Packed file is corrupt`: 16 groups (segment `n*1000h`),
   each `u16 count` + `count` × `u16 offset`.
3. The rebuilt MZ header (`0xE0` paragraphs = 3,584 bytes, 794 relocs) declares 165,568 bytes
   (`0x286C0`); the Borland overlay block (**`FBOV`**) follows at that offset, 114,464 bytes,
   verbatim from the packed file.  The reconstructed file is again 280,032 bytes.

## Overlays (Borland TLINK VROOMM overlays, int 3Fh)

* Root image = 9 code segments + the overlay manager (`seg010`, `__OVRINIT`, `__SwapOverlay`, ...) +
  segment table + stubs + `dseg` (DGROUP, selector `286Fh`) + stack.  The C runtime and the overlay
  manager are Borland C++ 1991 ones (the string `91 Borland Intl.` is in DGROUP; IDA names `__OVRINIT`, `__ReadOvrDisk`, ...).
* `FBOV` block: `"FBOV"`, `u32 size (0x1BF10)`, `u32 0x18BA0`, `u32 0x2E`, then the 13 overlay
  bodies.  Each body is code (`size` bytes) followed by its relocation table (`u16` offsets).
* Every overlay has a **stub segment** in the root (`0x27F20` ...; IDA names `stub01..13`):
  a 32-byte descriptor (`CD 3F`, `u32` file offset relative to `FBOV+0x10`, `u16` code size,
  `u16` reloc bytes, `u16` thunk count) followed by 5-byte thunks `CD 3F <off16> 00`
  (`int 3Fh` + offset of the function inside the overlay).  Far calls into an overlay go
  `call stubNN:thunk`.
* The relocation words in an overlay body hold the offset of a row in the **segment table**
  (`0x27DA0`, 8-byte rows: paragraph, size, flags, owner); the loader replaces them with the row's
  paragraph.  In our files they only ever point at root segments and at stub segments.
* Overlay sizes: 8,291 / 8,953 / 8,424 / 5,988 / 8,292 / 8,339 / 8,673 / 8,559 / 8,788 / 8,741 /
  8,346 / 8,484 / 8,709 bytes.

## What IDA does for us

IDA 8.3's MS-DOS loader understands this layout: it maps the overlays as `ovr022..034` above
`0x378C0`, turns the thunks into `jmp far` and applies the relocations.  `ida_scripts/build_idb.py`
only has to (1) rename the segments (`ovl01..13`, `stub01..13`, `stack`), (2) default `ds` to
DGROUP, (3) split the over-long functions the first pass produces by re-creating one function per
`push bp; mov bp,sp` / thunk target.

Segments after the build:

| IDA range | name | contents |
|---|---|---|
| `10000-14BE3` | seg000 | runtime start + C library + shared helpers (213 routines) |
| `14BE3-268BD` | seg001..seg008 | game code modules (`seg005`/`seg006` are single 8 KB routines; `seg007` holds a 9 KB data blob) |
| `268BE-27BF1` | seg009, seg010 | overlay manager (`__OVRINIT` at `269A9`) |
| `27C00-27F20` | seg011 | overlay manager data + segment table |
| `27F20-286EA` | stub01..13 | overlay stubs (descriptor + thunks) |
| `286F0-37840` | dseg | DGROUP |
| `378C0-52165` | ovl01..13 | the overlays |

## Compiler and names

The program is Borland C++ (1991) code, not Microsoft: the thunk/overlay scheme is Borland's VROOMM
(`int 3Fh`, `FBOV`), and BinDiff against the Xeen database matched the Borland runtime library
(`_textmode`, `__VPRINTER`, `__scanner`, `_setvbuf`, ...) at similarity 1.00.  Names that came from
that BinDiff run (>= 0.90 similarity) are kept in `names/mm3.tsv`; `rebuild_all.ps1` re-applies them
(`ida_scripts/apply_names.py`) after a rebuild, and `ida_scripts/export_names.py` writes the names
of the open database back to that file.

## `MM3.CFG` (4 bytes, read by `openMm3Cc`)

Copied to DGROUP `ACBDh` (`byte_333AD`..`byte_333B0`) and consumed by `_main`:
* byte 0: video mode selector 0-4, mapped by a 5-way switch to the internal video mode `word_34C0E` (0 -> 0, 1 -> 4, 2 -> 3, 3 -> 1, 4 -> 5); the intro uses `logy5.raw` when it is 4 (the VGA logo);
* byte 1: sound device 0-6, a 7-way switch that sets one of seven driver flags (`byte_28842`..`28848`) which select the sound driver to load (candidates `ADLIB`, `ROLAND`, `BLASTER`, `COVOX`, `TANDY`, `IBM`, `DEMO`; the mapping was not traced); `TIMER.DRV` is always loaded;
* bytes 2 and 3: passed to the driver initialisation `sub_2693F(byte2, byte3)` -- most likely the port and IRQ.
The GOG copy is `00 01 20 02`: video selector 0, sound device 1, parameters 20h and 2 (the device-to-number assignment of byte 1 was not worked out).
