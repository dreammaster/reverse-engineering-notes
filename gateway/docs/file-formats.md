# On-disk file formats

Formats traced from `gatemain.idb`'s code directly (not guessed from raw
bytes — see the `VOCAB.DAT` section below for why that distinction
mattered here). Cross-checked where possible against the real installed
game at `c:\games\gw` (Gateway) and, for confirming something is generic
engine behavior rather than Gateway-specific data, `c:\games\gw2`
(Gateway II).

## Huffman byte-stream compression (shared primitive)

`huffman_decompress` (`gatemain.asm:5690`) is a general-purpose canonical
Huffman decoder used by at least two different resource loaders
(`vocab_load` for `VOCAB.DAT`, `get_message` for `GATESTR.DAT` — see
below, now traced directly). Signature: `huffman_decompress(dest,
streamSize, fileHandle, huffmanTable, huffmanTableSize, symbols,
symbolsCount)`.

Mechanics, confirmed by direct read:
- `huffmanTable` is a flat array of **signed 16-bit words**, 2 per tree
  node (left/right child), read as `huffmanTable[node*2 + bit]`.
- The walk starts at `huffBase = huffmanTableSize - 2` (root = last
  node-pair in the table) and descends one bit at a time from the
  compressed stream (LSB-first within each byte, refilled via `fread`
  one byte at a time when the 8-bit counter runs out).
- A **negative** table value ends the walk: the decoded symbol index is
  `-value - 1`. Values `< 0x80` are plain byte values (the base
  alphabet); values `>= 0x80` index into a separate `symbols`/
  `symbolsCount` array (only used by the message-text loader, not by
  `vocab_load`, which always passes `symbols=NULL`/`symbolsCount=0`) —
  likely a dictionary of common whole words/substrings for better
  compression of prose specifically, not needed for the vocabulary file.
- A **non-negative** value is the next node index to continue the walk
  from.

## `VOCAB.DAT` — parser vocabulary file

Traced via `vocab_load` (`gatemain.asm:161780`), confirmed against the
real file at `c:\games\gw\VOCAB.DAT` (22,081 bytes). Layout, in read
order:

1. `nodeCount` (`u16`) — Huffman tree size for this file. (First word of
   the real file is `0x0054` = 84.)
2. `huffmanTable` — `nodeCount` signed 16-bit words (i.e. `nodeCount/2`
   node-pairs — the loader reads `nodeCount` **bytes** worth via `fread`
   with `size=2` and that same word count, so the buffer holds
   `nodeCount` words total, consistent with the decoder's `node*2+bit`
   indexing needing an even count).
3. `streamSize` (`u16`) — length in bytes of the compressed bitstream
   that follows.
4. The compressed bitstream itself (`streamSize` bytes) — Huffman-decoded
   directly into a fixed 9,794-byte (`0x2642`) buffer, `vocab_data`. This
   is the **decompressed text pool**: every vocabulary word's actual
   ASCII text lives here, referenced by byte offset. (This is why the
   raw hex bytes right after the small header look like noise — they're
   compressed, not a literal offset/index table as an initial guess from
   the hex dump alone suggested. Cross-checking the code path first
   avoided a wasted attempt at reverse-engineering compressed bytes as
   if they were plain data.)
5. `vocabCount` (`u16`) — must equal a value already known to the
   program (baked into `gatemain.exe`/`gatemain.ovl` itself, compared
   with a hard `"Error: vocabulary data mismatch"` abort on mismatch) —
   i.e. this file is tightly version-locked to the specific executable
   build, not independently re-orderable.
6. `vocabCount` × `VocabFileRec` (4 bytes each: `u16 _vocabOffset`,
   `u8 _flags`, 1 spare byte) — the **word table**. For each record `i`:
   `vocab_list[i]._textP = vocab_data + _vocabOffset` (a pointer into the
   decompressed text pool from step 4), `vocab_list[i]._flags =
   _flags`.
7. `vocabCount` (`u16` again — re-read fresh from the file at this
   point, not reused from step 5) — the count driving the **synonym/link
   table** below.
8. For each of those `vocabCount` entries (in order, implicitly indexed
   0..vocabCount-1 by read order — no explicit index field): one
   `VocabFileRec` read as `links` (its `_vocabOffset` field reused here
   as "canonical/target vocab id" rather than a text offset — the same
   4-byte record shape serves double duty depending on context, not two
   different structs), then `links._flags` more `VocabFileRec` reads
   (`linkEntry`, `_vocabOffset` reused as "which vocab id this synonym
   entry describes"). For each: `vocab_list[linkEntry._vocabOffset]
   ._altVocabId = links._vocabOffset` and `..._logicNum =
   linkEntry._flags`.

**In memory**, each of the `vocabCount` entries becomes one `VocabEntry`
(7 bytes: `dword _textP`, `byte _flags`, `word _altVocabId`, `word
_logicNum`) in the `vocab_list` array. `_altVocabId`/`_logicNum` are
exactly the two fields `main()`'s `PARSER_OOPS`/`PARSER_UNDO`/
`PARSER_AGAIN` meta-command handling reads (see
`overview.md`'s write-up of `main`) — so this synonym/link table is how
e.g. multiple surface words (synonyms, abbreviations) resolve to one
canonical vocabulary id at parse time, and how a vocab id optionally maps
to a specific logic-script hook (`_logicNum`).

**Not yet decoded**: the exact alphabet the base 128 Huffman symbols
(`< 0x80`) represent (presumably ASCII letters/punctuation actually used
across all vocab words — worth confirming which subset).

## `GATESTR.DAT` — compressed message/string resource file

Traced via `gatestr_load` (`gatemain.asm:5191`, the loader) and
`get_message` (`gatemain.asm:5835`, the runtime lookup/decompression
path), confirmed against the real file at `c:\games\gw\GATESTR.DAT`
(349,805 bytes — the header's first few values match exactly, see
below). Considerably more elaborate than `VOCAB.DAT`: strings are
grouped into **sections**, each individually and independently
Huffman-compressed per string (not one bitstream for the whole file),
with a small in-memory LRU cache of already-decompressed strings so
repeat lookups skip decompression entirely.

**On-disk layout**, in read order (all via `gatestr_load`):

1. `gatestr_sectionsCount` (`u16`) — number of sections. Real file:
   `0x0038` = 56.
2. `gatestr_sectionsCount` × `StrHeaderEntry` (4 bytes each: `u16
   stringsCount`, `u16 streamSize`) — one header per section, held in
   memory as `gatestr_sectionsTable`. Real file's first few entries:
   `{61, 1063}`, `{142, 3119}`, `{11, 256}`, `{137, 6635}`, `{179,
   9416}` — all immediately plausible small string counts and byte
   sizes, strong confirmation of the struct shape. While reading this
   table, the loader also tracks `gatestr_maxEntryCount` (the largest
   single section's `stringsCount`, across all sections) and
   `gatestr_total_strings` (the sum of every section's `stringsCount`).
3. (Not read yet, but its start position is recorded as
   `gatestr_sectiionsOffset` via `ftell`, and the file cursor
   immediately seeks *forward* past it without reading:) **one `u16` per
   string across the whole file** (`gatestr_total_strings` of them
   total) — each string's **compressed byte length**, grouped
   contiguously by section. This is read lazily later, one section's
   slice at a time, into a shared scratch buffer `gatestr_entryBuffer`
   (allocated once up front, sized for the single largest section via
   `gatestr_maxEntryCount * 2` bytes, then reused/overwritten whenever a
   *different* section than the currently-cached one is requested — see
   `get_message` below).
4. `gatestr_huffmanTableSize` (`u16`), then that many signed 16-bit
   words into a single **global** `huffmanTable` — one shared Huffman
   tree for the *entire file* (unlike `VOCAB.DAT`'s tree, which is
   file-local; this one lives at the same fixed global `huffmanTable`
   symbol `vocab_load` doesn't touch).
5. `gatestr_commonStringsCount` (`u16`). If nonzero: that many `u16`
   byte-offsets into a following blob (read into a temporary stack
   table), then `gatestr_tableSize` (`u16`, the blob's total byte
   length), then the blob itself (`gatestr_commonData`, allocated to
   exactly that size). Each of the `commonStringsCount` offsets becomes
   one far pointer in `gatestr_commonStrings[i] = gatestr_commonData +
   offset[i]` — a dictionary of whole common words/phrases. This is
   exactly the `>= 0x80` extended-symbol case in `huffman_decompress`
   above: Huffman symbols 0-127 are raw bytes, symbols 128+ each expand
   to one full dictionary entry from this table — a simple but effective
   two-level compression scheme for English prose specifically (raw
   Huffman coding *of the dictionary/symbol stream*, where the
   "alphabet" includes whole common words, not just single characters).
6. The current file position (`ftell`) is recorded as
   `gatestr_strOffset2` — this is where the actual **per-string
   compressed bitstreams** begin, laid out back-to-back, section by
   section, string by string, in the same order as the length tables
   from step 3.
7. Finally, allocates the runtime decompressed-string working buffer
   `gatestr_buffer`: tries `0xC00` (3072) bytes first, and on allocation
   failure backs off by `0x100` (256) at a time down to a floor of
   `0x400` (1024) bytes, hard-erroring (`finish()`) only if even that
   minimum fails — a defensive "grab as much as available, within
   reason" strategy typical of this era's memory-constrained DOS code.

**Runtime lookup** (`get_message`, called with either a raw far
`char*` — recognized by a literal segment value of `0xF000`, meaning
"not a message id, already a pointer" — or a 16-bit **message id**):

- A message id packs `(sectionId << 10) | indexWithinSection` (top 6
  bits / bottom 10 bits — confirmed exactly via the decomposition code:
  `sectionId = msgId >> 10`, and the low-10-bits mask done as `and
  ah, 3` on the high byte of `msgId`, which combined with the untouched
  low byte is equivalent to `msgId & 0x3FF`). Both parts are
  range-checked against `gatestr_sectionsCount` and the target section's
  `stringsCount` before use; out-of-range returns `NULL` (a shared
  `error:` path also used for I/O failures).
- A small 32-entry **LRU cache** (`_textCache`, an array of
  `ResourceTextEntry { u16 _id, u16 _offset, u16 _ctr }`) is checked
  first by linear scan for a matching `_id`. On a hit: every *other*
  entry's `_ctr` (age) is incremented, the hit entry's own `_ctr` is
  reset to 0, and the result is `gatestr_buffer + _textCache._offset[i]`
  — no file I/O or decompression at all.
- On a miss: if the requested string's section isn't the one whose
  length-table is currently sitting in `gatestr_entryBuffer`
  (`gatestr_currentSecton`), seeks to `gatestr_sectiionsOffset + 2 *
  sum(stringsCount for every earlier section)` and re-`fread`s that
  section's `stringsCount` length-words into `gatestr_entryBuffer`,
  updating `gatestr_currentSecton`. Only one section's length-table is
  ever resident at a time.
- Computes the exact file offset of the requested string's compressed
  bytes: `gatestr_strOffset2 + sum(streamSize for every earlier
  section) + sum(gatestr_entryBuffer[j] for every earlier string in
  this section)`, seeks there, and calls `huffman_decompress` with
  `streamSize = gatestr_entryBuffer[indexWithinSection]` (that one
  string's own compressed length) straight into a `0x1014`-byte stack
  buffer.
- If appending the newly-decompressed string would overflow
  `gatestr_buffer` (tracked via a running write cursor,
  `gatestr_stringOffset`), or the cache is already at its 32-entry cap,
  calls `makeRoomInTextCache` (not traced yet — presumably evicts the
  coldest/highest-`_ctr` entries and compacts or wraps the buffer)
  before proceeding. Otherwise ages every existing cache entry, inserts
  a fresh `_textCache` entry (`_id = msgId`, `_offset =
  gatestr_stringOffset`, `_ctr = 0`), `strcpy`s the decompressed text
  from the stack buffer into `gatestr_buffer` at that offset, advances
  `gatestr_stringOffset` by the decompressed length, and returns the new
  pointer.

**Not yet traced**: `makeRoomInTextCache`'s eviction/compaction policy,
and the exact contents of the base 128-symbol alphabet (shared question
with `VOCAB.DAT` above, since both use the same `huffman_decompress`
mechanics even though each file's actual Huffman tree is independent —
`VOCAB.DAT`'s is file-local, `GATESTR.DAT`'s is the global
`huffmanTable`).

## Room/logic "format" — there isn't one; it's compiled code

Expected to find a third external resource format here, analogous to
`VOCAB.DAT`/`GATESTR.DAT`, given the AGI/SCI-style struct names
(`Room`, `LogicSection2`-`8`, `LogicIndexEntry`) — **that expectation was
wrong**. Traced `Logic_call`/`Logic_getMethodIndex` (the dispatch
mechanism the parser calls into) and confirmed `proc_table` — the
`LogicIndexEntry` array these structs describe — is **static data linked
directly into `gatemain.exe`/`gatemain.ovl`**, not read from any file at
runtime. Every room/object/handler's actual behavior is compiled 8086
machine code, reached via a `type`-tagged (1-8) metadata record whose
shape varies per type (`Room` is type 1, not a separate concept from the
`LogicSectionN` structs). Full writeup, since this is really an
engine-architecture finding rather than a file format, is in
[overview.md](overview.md#the-roomlogic-format--and-why-there-isnt-one-its-compiled-native-code-not-data).

Recorded here mainly so a future session doesn't waste time looking for
a `ROOM.DAT`/`LOGIC.DAT`-style file that doesn't exist — none of the
files at `c:\games\gw` match that shape either (only per-room-number
`.PIC`/`.RGN`/`.FNT`/`.MUS`, no generic logic/script resource).

## `OBJECT.DAT` — object/room/NPC name strings

Traced via `objects_load` (`gatemain.asm:162038`) and
`Logics_getObjectString` (`gatemain.asm:2692`), confirmed against the
real file at `c:\games\gw\OBJECT.DAT` (8,031 bytes). The simplest of the
three real external formats found so far — no compression, no sections:

1. `byteLength` (`u16`) — length in bytes of the blob that follows. Real
   file: `0x1F5D` = 8029, and `8029 + 2 (header) = 8031` = the file's
   exact total size.
2. `byteLength` raw bytes, read verbatim in one `fread` straight into a
   fixed `objects_array db 1F5Eh dup(0)` buffer (8,030 bytes — one more
   than the real header value, presumably headroom/alignment rather than
   anything meaningful). This blob is simply **concatenated
   NUL-terminated ASCII strings** — the real file starts
   `"Blast Zone\0Gray Plain\0Plateau\0..."`, immediately recognizable as
   room/location names, no framing needed since each string carries its
   own terminator.

**No index table exists in this file at all.** Confirmed by
`Logics_getObjectString(entityIndex)`: it looks up `entityIndex` in
`proc_table` — the *same* `LogicIndexEntry` tagged-union table from the
room/logic finding above — and, notably, **every one of the 8 per-type
variant structs (`Room`/`LogicSection2`-`8`) has one word in common at
offset 0** (not previously named/described, since neither struct
definition labels it — sits right before `_vocabArrIndex` at offset 2):
a **1-based byte offset into `objects_array`**. The final string address
is computed as simply `objects_array + offset - 1`. So the
offset-into-the-string-blob for every object's name is baked into
`proc_table`'s static compiled data (see the room/logic section above),
while only the actual string *bytes* live in this external, easily
re-editable file — a sensible split for a text adventure where prose
gets tweaked far more often during development than dispatch logic.

**Dynamic override layer, spotted in passing**: before consulting
`objects_array` at all, `Logics_getObjectString` first calls
`LogicStrings_call(entityIndex, action=16)` — a linear scan over a
small fixed 44-entry `LOGIC_STRINGS` table (`FunctionEntry` structs:
`{u16 id, far ptr fnPtr}`), and if `entityIndex` matches one of the 44
`id`s, calls that entry's own compiled function instead of ever touching
the static string blob. This is how a handful of specific objects (only
44 of `METHODS_COUNT` = 734 total) get a **computed** name/string
instead of a fixed one — e.g. something whose description changes with
game state. Not traced further (would mean reading each of the 44
`LogicStringsNN`-named handler functions individually); flagged as a
minor, self-contained follow-up, not blocking.

## Numbered resource files — shared naming convention

Every `GATE_XXX.<ext>`-style file (pictures, regions, fonts, music) goes
through one common helper, `open_file2` (`gatemain.asm:41209`):
`sprintf("%s_%03d.%s", filename_prefix, fileNumber, FILE_TYPES[fileType])`
— `filename_prefix` is a global (`"GATE"` for this game), `fileType` is a
small enum indexing a `FILE_TYPES` string table for the actual extension
(`FILETYPE_PIC`, `FILETYPE_RGN`, `FILETYPE_SAV2`, others not enumerated
yet), and `fileNumber` is the 3-digit number seen throughout the real
install (`GATE_000.PIC`, `GATE_101.FNT`, etc.).

## `GATE_XXX.RGN` — clickable-region files

Traced via `load_regions` (`gatemain.asm:56308`), matching the already
partially-defined `RegionIndex`/`RegionEntry` structs. A region file
holds clickable/hotspot rectangles for one picture, used for
point-and-click item interaction (Gateway's late-Early-engine hybrid
between full parser input and direct mouse targeting — see the
engine-lineage note at the top of this file). Confirmed by direct
read of the loader, not yet cross-checked against a real `.RGN` file's
raw bytes (the struct-level format is unambiguous from the code alone).

- The file begins with a flat, fixed-position array of `RegionIndex`
  (6 bytes: `u16 fileOffset`, `u16 field_2`, `u16 regionCount`) —
  addressed by direct seek to `entryNumber * 6` from the start (no count
  prefix needed since callers already know which entry they want). Each
  entry describes one **region set** for the picture (plausibly
  different game states of the same scene — e.g. before/after some
  event — though not confirmed which numbering scheme picks the entry).
- `RegionIndex.fileOffset` points elsewhere in the same file to that
  set's `regionCount` × `RegionEntry` records (6 bytes each: `u16
  itemId`, `u8 x1`, `u8 y1`, `u8 x2`, `u8 y2` — matching the struct
  definition exactly). Coordinates are stored in a small byte range and
  scaled up at load time: `x1`/`x2` are simply doubled; `y1`/`y2` are
  scaled by `96/224` or `168/224` depending on the active video mode
  (`_videoIndex` 0 or 1 respectively — EGA/Tandy-class modes with a
  shorter physical display height than the 224-line design/logical
  resolution), or used unscaled for higher-resolution modes (`_videoIndex`
  ≥ 2, e.g. VGA) that can show the full 224 lines directly. Each parsed
  region becomes one clickable hit-rect (`Regions_addRegion`, given the
  scaled coordinates plus the current window's screen offset) and its
  `itemId` is recorded in a `regionList[]` array for later lookup by
  region-slot index.

**Not yet decoded**: `RegionIndex.field_2`'s meaning (stored into a
global on load, not otherwise referenced in this function), and exactly
what numbering scheme selects which `RegionIndex` entry for a given
picture/state.

## `GATE_XXX.PIC` — picture/image files

Traced via `load_picture` (`gatemain.asm:50322`) and `Image_load`
(`gatemain.asm:52262`); confirms and extends the existing `PIC_HEADER`/
`PicIndexEntry`/`Picture`/`PictureDecoder` struct family. This is the
richest format traced so far — multi-frame pictures, an optional
embedded palette, and a picture-numbering scheme that exactly explains
the file groupings seen in the real install
(`GATE_0xx`/`1xx`/`2xx`/`3xx`/`4xx.PIC`).

**Picture numbering**: a picture is addressed by one 16-bit `picNumber`.
Bit `0x8000` set is a special case (use the current hardware
`_videoIndex` directly as the bank number, bypassing the normal
derivation below). Otherwise: `bank = picNumber >> 12` (top nibble); if
that's `0` *and* the hardware isn't the base video mode
(`_videoIndex != 0`), `bank` is forced to `1` instead — i.e. bank 0's
pictures need a hardware-specific substitute bank 1 on non-default video
hardware, while banks 1-4 don't. The on-disk **file number** is then
`bank*100 + ((picNumber >> 8) & 0xF)` — exactly matching the real
install's five file groups (`GATE_0xx.PIC` through `GATE_4xx.PIC`, each
covering up to 16 files) once resolved through `open_file2` (see above).
Whether `bank` 1-4 represent the game's four acts/areas (matching the
struct/file-numbering hints — plausible, given each bank groups a
contiguous run of picture files) or something else isn't independently
confirmed yet, only inferred from this numbering scheme and the file
listing.

**Within one physical file**, the low byte of `picNumber` selects a
**12-byte `PicIndexEntry`** at a direct seek to `(picNumber & 0xFF) *
12` from the start of the file — no count prefix, addressed directly
like `RegionIndex` above. Confirmed exactly 12 bytes via the seek-offset
math (`lowByte * 12`) matching what gets `fread`-loaded into the global
`pic_header`, whose fields resolved from usage:
`{ fileOffset: u32, flags: u8, frameCount: u8, field_A: u16 (a
2-byte value later split into two per-`Image` bytes — hotspot or offset
pair, not confirmed), width: u16, height: u16 }` (4+1+1+2+2+2 = 12).
`fileOffset == 0` means "no picture at this slot" (load fails cleanly).
`flags` bit `0x10` = has an embedded palette following the frame table
(`PICFLAG_HAS_PALETTE`, name already present from earlier work);
another bit selects pixel bit-depth (`PICFLAG_BIT_DEPTH`, mask not
pinned down to an exact value this pass); bit `0x40`'s role wasn't
traced (only that `Image_load` reads it to set one field byte).

**A picture can hold multiple frames** (`frameCount`, up to 255):
starting exactly at `fileOffset` sits a flat array of `frameCount` **4-byte
draw-position entries** (`{u16 x, u16 y}` each, read via two
`freadWord` calls) — frame 0 defaults to `(0, 0)` without needing a
table entry; frames 1+ read their `(x, y)` from `fileOffset +
(frameNumber-1)*4`. Right after this whole frame-offset table (`fileOffset
+ frameCount*4`) comes the real payload: an optional palette block (only
if the `0x10` flag is set, sized via a `video_palette_sizes[bitDepth]`
lookup) followed by the actual pixel data, handed off to
`PictureDecoder_load` for decompression/rasterizing (not traced this
pass — the actual pixel encoding is still unknown).

If the caller only wants metadata (`pic_headers_only_flag` set), loading
stops right after resolving the draw position, before ever reading
palette/pixel data — used by whatever needs picture dimensions without
displaying it.

**Not yet decoded**: the exact `PICFLAG_BIT_DEPTH` bitmask value, `flags`
bit `0x40`, and `field_A`'s real meaning. Also not confirmed: whether
bank 1-4 really correspond to the game's four story acts, or something
else entirely.

## Picture pixel compression — an LZ77+Huffman hybrid, plus per-video-mode blit

Traced `PictureDecoder_load`/`PictureDecoder_load2`/
`PictureDecoder_unpack`/`PictureDecoder_fetch` (`gatemain.asm:39727`
onward) — the actual pixel payload format `.PIC`'s frame data decodes
to, referenced but not opened in the section above. A proper
LZ77-style sliding-window decompressor with Huffman-coded tokens (not
unlike a simplified precursor to Deflate/LZH), not a simple RLE scheme —
sophisticated for a 1992 title.

**Setup** (`PictureDecoder_load2`, dispatched per video mode from
`PictureDecoder_load` — see below): reads a small fixed header from the
compressed stream (a code-size byte, used to build a bit-extraction mask
`0xFFFF >> (16 - codeSize)`) then loads several small **static constant
tables** (`PictureDecoder_DATA1`-`5`, sized 256/16/16/32/64 bytes) into
per-decoder working arrays (`_array3`, `_array5`-`_array7`), building
canonical-Huffman-style decode tables via a helper
(`PictureDecoder_setupArray`) fed by two further constant "reference"
tables (`PictureDecoder_REF1`/`REF2`). One header value (a version/mode
byte) selects between two slightly different setups — not fully
resolved which real files use which.

**Token stream** (`PictureDecoder_fetch`): reads one bit to choose
between two decode paths — a short canonical-Huffman lookup producing a
**match-length token** (added to `0x100`, i.e. tokens `≥ 0x100` are
matches) via `_array2`/`_array5`-`_array7`, or a longer, multi-level
canonical-Huffman lookup (peeking 8 bits into `_array10`, and on a
sentinel `0xFF` miss reading progressively more bits — 4, 6, or 8 —
through `_array11`/`_array12`/`_array13` — before a final `_array3`
lookup) producing a **literal byte value** (`< 0x100`). A terminator
token (`0x305`/773) ends the stream.

**LZ77 reconstruction** (`PictureDecoder_unpack`): literal tokens
(`< 0x100`) are written straight to the output buffer; match tokens
(`≥ 0x100`) give a length directly as `token - 0xFE` (so the minimum
token, `0x100`, gives a minimum match length of 2) and fetch a separate
back-reference **distance** via `PictureDecoder_getBlockOffset`
(not traced in detail), then `rep movsb`-copy `length` bytes from
`outputIndex - distance` — textbook LZ77. The output buffer is 8KB
(`0x2000`), used as **two 4KB (`0x1000`) halves in a sliding-window
double-buffer**: once the write cursor crosses the halfway point, the
first half is flushed to the screen via the video-mode-specific
`_copyFn` callback, and the second half is copied down to the start —
keeping the full 4096-byte back-reference window available for future
matches while streaming output incrementally, rather than requiring the
whole decompressed picture to fit in memory at once.

**Two blit strategies, selected by `_videoIndex`** (confirmed by
direct read of both callbacks, `gatemain.asm:49526`/`49576`):
- **`PicFile_copy_nonEga`** (`_videoIndex` 0 or 3) — a straight linear
  byte copy into the framebuffer (`PictureDecoder_image`, advanced by
  the copied count each call). Consistent with a chunky/packed
  byte-per-pixel mode (VGA 256-color).
- **`PicFile_copy_ega`** (`_videoIndex` 1, 2, or 4) — the classic 4-plane
  EGA/Tandy **planar bit-unpacking** technique: each decompressed byte
  is *not* pixel data but a **4-bit plane-membership mask** per pixel
  group (bit 0 → OR the current color into bitplane 0 at
  `PictureDecoder_image`, bit 1 → bitplane 1 at
  `PictureDecoder_image + word_D22CA` — the per-plane scanline byte-width
  computed in `ega_setup`'s width math above — bit 2/3 → the further
  planes at `+word_D22CA*2`/`*3`), with a rotating bit-position mask
  (`word_D22C4`, starting at `0x80`) selecting which pixel within the
  current output byte each mask bit affects. This is the standard "write
  mode 0, plane-serial" EGA pixel-plotting idiom used throughout early
  1990s DOS graphics code, confirming the compressed stream really does
  carry EGA-native plane data for those modes rather than a
  device-independent pixel format decoded differently per mode.

**Not yet decoded**: the exact semantics of `PictureDecoder_getBlockOffset`
(the match-distance decode — presumably another Huffman-coded or
fixed-width bit field, not traced), the precise roles of `_array1`
through `_array13` individually (enough is understood to confirm the
overall LZ77+Huffman architecture, not each table's exact bit-packing),
and the header/version byte's second setup path (`ega_setup`'s
alternate branch in `PictureDecoder_load`, only partially distinguished
from the primary path).

## `GATE_XXX.FNT` — bitmap font files

Traced via `Font_LoadFont` (`gatemain.asm:41729`), matching the
already-defined `Font` struct. File number is `_videoIndex*100 +
fontNumber` — the same banking convention as `.PIC` (and matching the
real install's `GATE_1xx`/`2xx`/`3xx`/`4xx.FNT` groupings). `fontNumber
== 0` is special-cased entirely in memory with **no file opened at
all** — a hardcoded default font description (`bytesPerLine = 0`,
`minPrintableChar = 0`, `maxPrintableChar = 0xFF`, `linesPerChar =` the
global `_fontLineHeight`, `fixedWidth = 8`, `fixedSpacing = 0`, no
per-character tables) — plausibly meaning "defer to some other built-in
rendering path (e.g. the BIOS's own hardware font)" rather than a
loaded bitmap, though not confirmed beyond the mechanical defaults
themselves.

For `fontNumber != 0`, the file is a clean, simple format, read one
byte at a time via `file_readByte` in this exact order:

1. `field_4`, `field_5` (`u8` each, roles not identified).
2. `bytesPerLine` (`u8`) — bytes per scanline of each glyph's packed
   1-bit-per-pixel bitmap (so glyph width in pixels is
   `bytesPerLine * 8`).
3. `minPrintableChar`, `maxPrintableChar` (`u8` each) — the inclusive
   character-code range this font actually covers (not necessarily
   0-255).
4. `_linesOffset` (`u8`, role not identified).
5. `linesPerChar` (`u8`) — glyph height in scanlines.
6. `fixedWidth` (**signed** `i8`) — a non-negative value is a literal
   fixed pixel width for every character; a **negative** value means
   "variable width" and triggers reading a separate 128-byte
   `charWidths` table (one byte per character code 0-127, regardless of
   `minPrintableChar`/`maxPrintableChar`) immediately following the
   header.
7. `fixedSpacing` (**signed** `i8`) — same convention as `fixedWidth`:
   negative triggers a separate 128-byte `charSpacings` table.
8. `field_D` (`u8`, role not identified).

After the header and any variable-width/spacing tables, the rest of the
file is the **glyph bitmap blob**: `(maxPrintableChar -
minPrintableChar + 1) * linesPerChar * bytesPerLine` bytes, read in one
shot — a flat, packed 1-bit-per-pixel bitmap covering only the declared
printable-character range (not the full 256-entry byte range), stored
glyph-by-glyph, scanline-by-scanline.

**Not yet decoded**: `field_4`/`field_5`/`_linesOffset`/`field_D`'s
actual roles (read but not yet seen consumed anywhere in this pass).

## `GATE_XXX.MUS` — background music files (partial, low confidence)

Traced the file-numbering convention and general loading architecture
via `sub_1FE5C` (`gatemain.asm:37309`, the periodic "refresh loaded
background music channels" routine, gated on a sound-enabled flag and
an "any song selected" check) — **considerably murkier** than every
other format in this file, and flagged as such rather than presented
with false confidence. No struct was pre-defined for this format
(unlike every other resource type traced so far), and this pass didn't
reach the same level of certainty.

**Confirmed**: file numbering follows the same packed-word convention
as pictures/regions — a table of up to 4 active-channel song selectors
(`{fileNumber: u8, songIndex: u8}` packed into one word each, in a
zero-terminated 4-entry list), opened via `open_file2(FILETYPE_MUS,
fileNumber)`, matching the real install's `GATE_00x`-`00x.MUS`
numbering (only bank 0 has `.MUS` files in the real install, unlike
`.PIC`/`.FNT`'s multiple banks).

**Plausible, not fully confirmed**: for each active channel, the code
seeks to a computed offset (`(songIndex*2 - capabilityFlag) * 8` bytes
from the start of the file — `capabilityFlag` derived from a hardware/
sound-capability bit) and reads a small fixed-size (12-byte) record from
there into a shared scratch buffer, extracting at least a byte-length
value (`count`) plus two other fields at fixed offsets from it. `count`
is then compared against an available-memory budget
(`word_C8578`, scaled) — **only if the full track fits** does the code
do a *second* seek+read, pulling `count` bytes of the actual track data
into a freshly allocated buffer for playback; otherwise that channel is
left unloaded this pass. This reads as a **lazy, memory-budget-gated
full-track-residency model** — load a song's data wholesale only when
there's room, rather than streaming or caching partial data the way
`GATESTR.DAT`'s LRU cache does — but the exact byte-level layout of the
12-byte per-song directory record (and what its non-`count` fields
mean) wasn't pinned down this pass.

**Follow-up needed**: a proper trace of this format would benefit from
finding whichever code actually feeds bytes to the sound hardware
(PC speaker / Ad Lib / whatever `word_C8582`'s bits select) rather than
starting from the memory-management side as this pass did — that would
likely clarify the per-song record's fields (probably includes a tempo
or loop-point value, given the shape of the surrounding code) much
faster than continuing to infer them from the allocator-adjacent logic
alone.
