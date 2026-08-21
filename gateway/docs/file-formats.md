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
