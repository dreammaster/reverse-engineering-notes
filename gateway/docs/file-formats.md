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
(`vocab_load` for `VOCAB.DAT`, `get_message` for the string/message
resource file — almost certainly `GATESTR.DAT`, not yet traced directly).
Signature: `huffman_decompress(dest, streamSize, fileHandle, huffmanTable,
huffmanTableSize, symbols, symbolsCount)`.

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
across all vocab words — worth confirming which subset), and the
message-file (`GATESTR.DAT`-shaped) call path through `get_message`,
which additionally uses the `symbols`/`symbolsCount` word-dictionary
extension `vocab_load` doesn't need.
