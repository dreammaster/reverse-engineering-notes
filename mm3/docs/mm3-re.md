# Might and Magic III: Isles of Terra — DOS resource formats

Reverse-engineering notes for the New World Computing (NWC) engine shipped with
*Might and Magic III: Isles of Terra* (1991, DOS), written for anyone building
an interpreter for the game. An independent decoder built from these notes
reproduces our reference copy's graphics, and the palette was confirmed by
rendering `ITIT0.VGA` under it and pixel-diffing against a known-good reference.

Prior work: Jeff Ludwig extracted `MM3.CC`'s members in-memory via debugger
and published the decrypted files at
[jeffludwig.com/mm3/resources.php](https://jeffludwig.com/mm3/resources.php);
we used those unpacks as an oracle to confirm our static extraction was on
the right track. ScummVM's Might and Magic engine documents the Xeen-era
formats (its `convertNameToId` hash and `.CC` TOC cipher carry over to MM3
unchanged) and is the baseline the Xeen comparisons below are drawn against.

## Relationship to World of Xeen

MM3 is the NWC engine generation before Clouds/Darkside/World of Xeen. Two
pieces are identical to Xeen: the `.CC` archive layout with its TOC cipher, and
the filename→id hash (ScummVM Xeen's `convertNameToId`). Four differ:

| | MM3 | World of Xeen |
|---|---|---|
| Executable packing | NWC LZW self-extractor → EXEPACK | PKLITE |
| `.CC` member packing | LZHUF-compressed | stored, whole-member XOR-`0x35` |
| Palette | one master embedded in a code module (`0x8f99` @ `0x39C`) | standalone 768-byte `.pal` members, per scene |
| Sprite scanline codec | literal / skip / fill, 16-bit line lengths | byte line lengths + backref / pair / pattern opcodes |

The sprite index framing and 4×`u16` layer header are the same as Xeen; only
the per-scanline encoding differs. Raw full-screen images (320×200 mode 13h)
are identical in both.

---

## 1. Executable packing (`MM3.EXE`)

`MM3.EXE` is packed in two layers: an LZW self-extractor on the outside,
wrapping a Microsoft EXEPACK executable. We have seen the outer packer only
in NWC titles (also King's Bounty, Tunnels & Trolls, Planet's Edge); whether
it is an in-house packer or a third-party tool is not established. EXEPACK
is the standard, publicly documented packer; only the NWC layer is described
here.

### Detection

MZ header with `cs:ip = 0:0` (entry at the very start of the load module); the
byte at the entry (file offset `e_cparhdr * 16`) is a near `JMP` (`0xE9`)
whose target begins `push cs; pop ds; mov [imm16], ax` with
`mov bx, es:[0x0002]` 13 bytes after the target (the loader reading the PSP
top-of-memory word).

### Packed layout

```
[0]                 MZ header; its single relocation entry patches an immediate
                    inside the loader so a load-base computation cancels out
[e_cparhdr*16]      loader stub; the u16 at stub+0x4B is `hdr`, the file
                    offset of the stored header block
[hdr]               the original file's entire header block, verbatim:
                    28-byte MZ header, relocation table at its e_lfarlc,
                    padding out to its e_cparhdr*16
[hdr + orig e_cparhdr*16]           LZW stream
[max(declared end, stream end)..]   original overlay data, verbatim
```

At run time the stub reopens its own file, relocates itself to the top of
memory, re-reads the stored header block, LZW-decompresses the stream over
itself, applies the stored relocations as DOS would (all samples we have
store zero — the payload is EXEPACK, which carries its own relocations one
layer down), restores `ss:sp`, and far-jumps to the original `cs:ip`.

### LZW codec

The `compress`/GIF lineage of variable-width LZW, at `min_code_size = 8`:

- Codes are read LSB-first. Literals are `0..=255`; `256` = clear, `257` =
  end; the first dictionary code is `258`.
- Code width starts at 9 bits and grows by one whenever the next free slot
  reaches `1 << width`, capping at 12 bits (4096 entries). A clear code
  resets the dictionary and width.
- Dictionary entries are `(prefix_code, appended_char)`; a code equal to the
  next free slot is the usual KwKwK continuation.

### Reconstruction

```
hdr          = u16 at (e_cparhdr*16 + 0x4B)
header_bytes = stored e_cparhdr * 16
module_len   = declared_end(stored e_cp, e_cblp) - header_bytes
image        = lzw_decode(file, from = hdr + header_bytes)
output       = file[hdr .. hdr+header_bytes]          (stored header, verbatim)
             + image[.. module_len]                   (truncate: the stream decodes
                                                       slightly past module_len into
                                                       zero padding / stale buffer bytes)
             + file[max(packed declared end, stream end) ..]   (overlay, verbatim)
```

MM3 keeps its FBOV overlay segment past the packed module's declared end; the
packed module is padded so the overlay sits at the same file position as in
the original. In our copy, `MM3.EXE` is 280032 bytes both packed and
reconstructed. The packed file contains no plaintext; after unpacking both
layers, the filename string table spans roughly `0x1A3D7`–`0x2170F`
(`logy5.raw` … `view.icn`).

---

## 2. The `.CC` archive

A `.CC` file is a flat archive with no magic:

```
u16                 member count
member count × {    TOC entry, 8 bytes each:
    u16   id        filename hash (§3)
    u24   offset    little-endian, byte offset of the payload
    u16   size      payload byte length
    u8    pad
}
payload bytes …     tile the file contiguously from the end of the TOC to EOF
```

The absence of a magic number is compensated by a structural check: sorted by
offset (entries may be out of file order), the members must tile the file
exactly — first payload immediately after the TOC, each next starting where
the previous ended, the last ending at EOF.

Members carry no filenames — only the hashed `id`.

### TOC cipher (identical to Xeen)

The member count is plaintext; the 8-byte entry table that follows is
encrypted per byte:

```
decrypted[i] = rol2(cipher[i]) - (0x54 + 0x99*i)   (mod 256)
```

where `rol2` is an 8-bit rotate-left-by-2 and `i` is the byte index within
the entry table.

### Member compression: LZHUF

Each member is LZHUF-compressed — the canonical Okumura/Yoshizaki `LZHUF.C`
codec (LZSS over a 4096-byte ring buffer, literal/length symbols coded with
an adaptive Huffman tree, match positions with static tables). Member layout:

```
u8    fill          (equals the next byte)
u8    fill
u16   size          BIG-endian, the decompressed length
bytes stream        LZHUF bitstream
```

Deltas from stock `LZHUF.C`: this 4-byte header replaces its `u32 LE` size
header, and the ring buffer is pre-filled with the `fill` byte instead of
`0x20`. Everything else is canonical:

- Window `N = 4096`, max match `F = 60`, `THRESHOLD = 2`; write position
  starts at `N - F`. Bits are read MSB-first.
- 314 coded symbols: `0..=255` are literals, `256..=313` are match lengths
  `3..=60` (`len = symbol - 253`). The adaptive tree starts with all
  frequencies 1 and is update-per-symbol with the sibling-property swap;
  when the root frequency reaches `0x8000`, all frequencies are halved
  (rounding up) and the tree rebuilt.
- A match position's upper 6 bits come from the static `d_code` table
  indexed by the next 8 bits of the stream; `d_len[byte] - 2` further bits
  are shifted in, and the low 6 bits of the result complete the position.
  Copy source is `(r + N - 1 - pos) mod N` where `r` is the current window
  write position.

A handful of incompressible members are stored verbatim instead. Detection:
treat a member as stored if its first two bytes differ, its size field is 0,
or the LZHUF decode of `size` bytes does not consume the stream to within 2
bytes of its end.

---

## 3. Filename hash and name recovery

The `id` in each TOC entry is the interpreter's filename hash — the same one
ScummVM calls `convertNameToId` for Xeen:

```
u16 name_id(name):
    name = uppercase(name)
    t = name[0]
    for c in name[1..]:
        t = ((t & 0x007F) << 9) | ((t & 0xFF80) >> 7)   # rol9 of the 16-bit acc
        t = (t + c) & 0xFFFF
    return t
```

Filenames live in `MM3.EXE` (after unpacking, §1). Recovering the name table
is: harvest the ASCII filename strings, hash each with `name_id`, and match
to a member `id`. Two caveats:

- Some names are built at runtime from `printf` format strings (e.g.
  `pow%d.icn`, `cr%d.vga`) and must be expanded before hashing.
- This recovers names for 319 of our copy's 558 members. The remainder —
  including the video driver module itself (§4) — seem to be addressed by
  `id` alone.

---

## 4. The master palette (inside a code module)

MM3 has one 256-colour VGA palette, embedded in the game's video-driver code
module — the `.CC` member with TOC `id = 0x8f99` (its filename does not
appear among `MM3.EXE`'s strings, so it can only be addressed by id). Within
that member:

- `0x39C`: the palette — 256 × 3 bytes, native 6-bit VGA DAC values (every
  byte `< 0x40`; scale by 4 for 8-bit RGB).
- `0x69C`: a mode-13h row-offset table, 200 × `u16` (`row * 320`), which the
  module's draw core indexes through. In our copy this table is all-zero on
  disk — it is computed by the module's init code at run time, not shipped
  precomputed. A validator should accept either the `k*320` sequence or
  all-zero.

Whether the game rebinds palettes per scene is not established; this master
palette appears to reproduce the reference art exactly.

---

## 5. Graphics codec (sprites and screens)

One word-based line format serves every graphics family the game ships:
`.fac` (faces), `.icn` (interface), `.out`/`.vga` (screens/backgrounds),
`.til` (tiles), `.brd`, `.pic`, `.mon` (monsters), plus the fixed 320×200
screens. Cracked from the draw core in member `0x8f99` at member offset
`0x1D70`.

In our copy's 558 members: 12 raw screens, 403 sprite resources (3705 frames
total), 143 non-graphics (text, music, drivers, data tables, §6's undecoded
formats).

### Full-screen images

A member of exactly 64000 bytes is a bare mode-13h 320×200 image — raw
palette indices, no header — rendered under the master palette. (We classify
by size alone; all 12 such members are named `*.raw`.)

### Sprite resources

```
u16                 frame count
frame count × {     index pair:
    u16  offset1    byte offset of the frame's first layer; never 0
    u16  offset2    byte offset of an optional second layer (0 = none)
}
… layers …          each referenced offset points at a layer (below)
```

Every frame in our corpus has a first layer; only `offset2` may be 0.

A layer is a header then a scanline stream:

```
u16  x_off          registration origin, x
u16  width
u16  y_off          registration origin, y
u16  height
… scanlines …       exactly `height` of them
```

Each scanline:

```
u16  line_len       byte length of the rest of the line, counting the line_x
                    word and the opcode bytes; 0 ⇒ row fully transparent
                    (no line_x, no opcodes, next scanline follows directly)
u16  line_x         starting x within the row (pixels left of it stay
                    transparent)
… opcodes …         line_len - 2 bytes of opcode stream
```

Advance to the next scanline by the end pointer (`line_len`), not by
counting opcode bytes.

Each opcode byte:

| byte range | operation | length |
|---|---|---|
| `0x00`–`0x7F` | literal — copy N operand bytes verbatim | `N = opcode + 1` |
| `0x80`–`0xBF` | transparent run — advance x, drawing nothing | `N = (opcode & 0x3F) + 1` |
| `0xC0`–`0xFF` | fill — one operand byte repeated | `N = (opcode & 0x3F) + 3` |

That is the entire opcode set — literal, skip, fill. (Contrast Xeen, whose
control byte is a 3-bit command + 5-bit length and adds RLE, LZ
back-reference, pair, and pattern opcodes over byte-sized line lengths.)

Transparency is index 0: no opcode in our corpus ever draws palette index 0,
so an untouched pixel (a skipped run, the gap before `line_x`, a zero-length
line, an unwritten row) reads back as 0 and is treated as transparent. (This
shortcut does not hold for Xeen, which can draw index 0.) Writes past a
layer's declared `width` do not occur in our corpus; a defensive decoder
should clip them, matching the drawer's own screen-edge clip.

### Two-layer frames

When `offset2` is non-zero the frame has a second layer drawn over the
first. Each layer is positioned independently by its own `(x_off, y_off)`
against a shared on-screen baseline — the draw core restores the baseline
row/column registers (member `0x8f99` offsets `0x1DBA` / `0x1DCE`) before
reading the second layer's header, so layer 2 is not simply overlaid onto
layer 1's rectangle. A live interpreter draws both layers in order; a static
exporter composites them into the union rectangle enclosing both, without
clipping either (in our corpus, 968 of 2651 two-layer frames have a second
layer that exceeds the first on at least one side).

---

## 6. Still undecoded

- `s#.s` scene scripts (7) and `out*.spl` outdoor sets (4): formats unknown.
- `Mon*.dat` monster/data tables: not decoded.
- 223 sprite members have no recovered filename (not among `MM3.EXE`'s
  strings, or runtime-built and unexpanded).
- The video driver module `0x8f99`'s own filename.
- Whether/how the engine rebinds the palette per scene.
