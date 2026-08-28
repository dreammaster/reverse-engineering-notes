# Picture decompression — full decode spec (`CXBUF2BUFEXPAND`)

Completes the follow-up flagged in [`compression_lzhuf.md`](compression_lzhuf.md):
`CXBUF2BUFEXPAND` and its `decode_*` / `read_*_len` / `make_table` helpers
were traced line-by-line from `Shannara Demo.asm`. This is enough to
reimplement picture decoding from scratch without a reference source.

**Confidence: very high.** Every routine is a line-for-line match against the
public LHA v2 sources (`slide.c` / `huf.c` / `maketbl.c` by Haruyasu
Yoshizaki & Haruhiko Okumura). Only the wiring (buffer-to-buffer glue, the
`_cx_*` globals, the DOS-extender segment copies) is Legend/`CXpack`-specific.

---

## 1. Correction to the earlier note

`compression_lzhuf.md` called this "LZHUF (LZSS + adaptive Huffman), the
classic Yoshizaki `lzhuf.c` algorithm." That's *almost* right but the
specific variant matters for reimplementation:

- It is **not** Okumura's 1988 `lzhuf.c` (which uses a single *adaptive*
  Huffman tree updated per symbol — `DecodeChar`/`DecodePosition`/`update`/
  `reconst`). None of those routines exist here.
- It **is** the **LHA `-lh4-` / `-lh5-` family**: per-block *static*
  canonical Huffman. Each block carries its own Huffman code-length tables
  (`read_c_len` / `read_pt_len`), decoded via `make_table`. Function names in
  the binary (`decode_c`, `decode_p`, `read_c_len`, `read_pt_len`,
  `make_table`, `fillbuf`, `getbits`) are exactly LHA `huf.c`/`maketbl.c`.
- The exact parameters (below) are the canonical LHA formulas with
  **`DICBIT = 12`** (4 KB dictionary) — i.e. method **`-lh4-`**. (Standard
  `-lh5-` is `DICBIT = 13`; `-lh6-` = 15; `-lh7-` = 16. Everything else is
  identical across the family.)

So: to reimplement, take any `-lh5-` decoder (`slide.c` + `huf.c` +
`maketbl.c`) and set `DICBIT = 12`, `NP = 13`, `PBIT = 4`. Nothing else changes.

---

## 2. Parameters (all verified against the disassembly)

| LHA name | Value | Where seen in `.asm` |
|---|---|---|
| `DICBIT` | **12** | window index masked `& 0FFFh` in `decode_` (`dword_1AB020`) |
| `DICSIZ` | **4096** | `cx_malloc(1000h)` in `CXBUF2BUFEXPAND`; chunk size cap |
| `THRESHOLD` | **3** | `decode_`: `sub eax, 0FDh` (0xFD = 256 − 3); min match = 3 |
| `MAXMATCH` | **256** | `NC` = 510 ⇒ 256 literals + 254 length codes; 254 = 256−3+1 |
| `NC` | **510** (`1FEh`) | `decode_c`: `cmp edx, 1FEh`; `_c_len db 1FEh dup`; `_c_table dw 1000h dup` |
| `CBIT` | **9** | `read_c_len`: first `getbits(9)`; degenerate-tree fill `getbits(9)` |
| `NP` | **13** (`0Dh`) | `decode_p`: `cmp ecx, 0Dh`; `read_pt_len` called with `nn = 0Dh` |
| `PBIT` | **4** | `decode_c`: 2nd `read_pt_len` called with `nbit = 4` |
| `NT` | **19** (`13h`) | `decode_c`: 1st `read_pt_len` called with `nn = 13h` |
| `TBIT` | **5** | `decode_c`: 1st `read_pt_len` called with `nbit = 5` |
| block-length field | **16 bits** | `decode_c`: `getbits(10h)` when `blocksize == 0` |
| bit order | **MSB-first** | all peeks are `bitbuf >> (32 − n)` |
| bit buffer width | **32-bit** | `_cx_nBit_Buffer` is a dword; `fillbuf`/`getbits` shift by `32 − n` |
| CRC | **CRC-32**, poly `0xEDB88320`, init `0xFFFFFFFF`, final `~` | `_cx_lCRC32Polynomial`; `cx_BuildCRCTable`; `CX_COMPUTECRC32` |

The 32-bit bit buffer is the only deviation from the canonical 16-bit
reference `huf.c`; it's a common later-port widening and does **not** change
the bitstream (still MSB-first, same code lengths).

---

## 3. Top-level entry — `CXBUF2BUFEXPAND`

Signature (from the `gxVirtualDecompress` call site, args pushed right-to-left):

```c
int CXBUF2BUFEXPAND(uint32 srcLen,   // arg_0  compressed byte count
                    uint32 dstLen,   // arg_4  decompressed byte count (= pic width * height)
                    void  *dst,      // arg_8  output buffer
                    const void *src);// arg_C  compressed input
```

Earlier note had the arg order as `(dstSize, srcSize, srcPtr, dstPtr)` — the
corrected order is **`(srcLen, dstLen, dst, src)`**.

Return: `0` success; `-3` (`0xFFFFFFFD`) work-buffer alloc failed; `-1`
(`0xFFFFFFFF`) produced-size ≠ `dstLen` (truncated / corrupt stream).

Body:

```c
work = cx_malloc(4096);                 // the 4 KB sliding window / staging buffer
if (!work) return -3;
_cx_InBuf          = src;
dword_1A9F28       = srcLen;             // input bytes remaining -> drives fillbuf EOF
_cx_lOriginal_Size = 0;                  // produced-so-far counter
decode_start();                         // = huf_decode_start(): init_getbits() + blocksize=0 + decode_j=0
for (rem = dstLen; rem != 0; ) {
    n = min(rem, 4096);
    decode_(n, work);                   // fill work[0..n-1]
    memcpy(dst, work, n);               // flush chunk  (done via segmented repne movs)
    dst  += n;
    _cx_lOriginal_Size += n;
    rem  -= n;
}
cx_free(work);
_cx_lCRC = ~_cx_lCRC;                    // finalized, but NOT checked here
if (_cx_Progress_Proc) _cx_Progress_Proc(0, 7, &sp);
return (_cx_lOriginal_Size == dstLen) ? 0 : -1;
```

Notes:
- **CRC is computed but ignored** on this path — pass/fail is purely the
  produced-size check. (`CXBUF2BUFINIT`, called from `gxSetMode`, seeds
  `_cx_lCRC = 0xFFFFFFFF` and builds the table; the archive path uses it,
  buf-to-buf expand does not.)
- `_cx_bBuf2Buf_Initialized` must already be set (it is — `CXBUF2BUFINIT`
  runs during `gxSetMode`). `read_byte`/`write_byte` no-op if it isn't.
- `_cx_InLen` is decremented by `read_byte` but never checked; the real
  input-exhausted guard is `dword_1A9F28`. When it hits 0, `fillbuf` stops
  calling `read_byte` and feeds zero bits (`_cx_subbitbuf` left as-is).

---

## 4. Bit reader — `fillbuf` / `getbits`

State globals: `_cx_nBit_Buffer` (u32 `bitbuf`, MSB-aligned), `_cx_subbitbuf`
(u32, current partial byte in low 8 bits), `_cx_bitcount` (bits left unused
in `subbitbuf`).

```c
void fillbuf(int n) {          // consume n bits, refill from the byte stream
    bitbuf <<= n;
    while (n > bitcount) {
        n -= bitcount;
        bitbuf |= subbitbuf << n;
        if (dword_1A9F28 != 0) {              // input not exhausted
            dword_1A9F28--;
            subbitbuf = read_byte() & 0xFF;   // next compressed byte
            _cx_lCRC = CX_COMPUTECRC32(subbitbuf, _cx_lCRC);
        }
        // else: subbitbuf keeps its value (feeds trailing garbage/zero bits)
        bitcount = 8;
    }
    bitcount -= n;
    bitbuf |= subbitbuf >> bitcount;
}

uint32 getbits(int n) {        // peek+consume n bits (n in 1..32)
    if (n == 0) return 0;
    uint32 v = bitbuf >> (32 - n);
    fillbuf(n);
    return v;
}

void init_getbits(void) {      // fillbuf(32) with everything zeroed
    bitbuf = subbitbuf = bitcount = 0;
    fillbuf(32);
}
```

`read_byte()`: `al = *_cx_InBuf++; _cx_InLen--; return al;` (guarded by
`_cx_bBuf2Buf_Initialized`).

Peeking without consuming (used in `decode_c`/`decode_p`/`read_c_len`):
`bitbuf >> (32 - n)` for the top `n` bits.

---

## 5. Sliding-window expansion — `decode_(count, buffer)`

`buffer` is the 4096-byte `work` buffer; it doubles as the dictionary.
`dword_1AB020` = LHA's static `i` (copy-source cursor); `_cx_lBytesLeft?` =
LHA's static `decode_j` (bytes still owed from an in-progress match, carried
across calls). Both persist between `decode_` calls; `decode_start` zeroes
`decode_j` only.

```c
// statics: uint i = dword_1AB020;  int decode_j = _cx_lBytesLeft?;
void decode_(uint count, uint8 *buffer) {
    uint r = 0;
    // 1. finish any match left over from the previous chunk
    while (--decode_j >= 0) {
        buffer[r] = buffer[i & 0xFFF];       // NB: i already an absolute index into buffer
        i = (i + 1) & 0xFFF;
        if (++r == count) { /* save i, decode_j */ return; }
    }
    // 2. main loop
    for (;;) {
        c = decode_c();                       // 0..255 literal, 256..509 = match
        if (c <= 0xFF) {
            buffer[r] = c;
            if (++r == count) return;
        } else {
            decode_j = c - 0xFD;              // c - (256 - THRESHOLD) = match length (3..256)
            uint p  = decode_p();             // match distance - 1  (0..4094)
            i = (r - p - 1) & 0xFFF;
            while (--decode_j >= 0) {
                buffer[r] = buffer[i];
                i = (i + 1) & 0xFFF;
                if (++r == count) return;
            }
        }
    }
}
```

Because `count` is always 4096 (except possibly the last chunk) and the
buffer is exactly `DICSIZ`, back-references wrap into the previous chunk's
still-resident bytes — standard LHA chunked `decode()`. A whole-buffer
reimplementation can equivalently keep one 4096-byte ring and `memcpy` each
full ring out to `dst`.

---

## 6. Literal/length symbol — `decode_c()`

```c
uint decode_c(void) {
    if (blocksize == 0) {                     // _cx_blocksize
        blocksize = getbits(16);              // symbols in this block
        read_pt_len(NT=19, TBIT=5,  3);       // -> pt_len[0..18], pt_table[256]  (code-length alphabet)
        read_c_len();                         // -> c_len[0..509],  c_table[4096]
        read_pt_len(NP=13, PBIT=4, -1);       // -> pt_len[0..12],  pt_table[256]  (distance alphabet)
    }
    blocksize--;
    uint c = c_table[bitbuf >> (32 - 12)];    // 12-bit fast lookup
    if (c >= 510) {                           // long code: walk the tree
        uint mask = 1u << (32 - 12 - 1);      // 0x80000
        do { c = (bitbuf & mask) ? cx_right[c] : cx_left[c]; mask >>= 1; } while (c >= 510);
    }
    fillbuf(c_len[c]);
    return c;
}
```

`cx_left[]` / `cx_right[]` are the binary-tree node arrays (`dw 3FBh dup`
each = 1019 = 2·NC − 1) built by `make_table` for codes longer than
`tablebits`.

---

## 7. Distance symbol — `decode_p()`

```c
uint decode_p(void) {
    uint c = pt_table[bitbuf >> (32 - 8)];    // 8-bit fast lookup
    if (c >= 13) {                            // NP
        uint mask = 1u << (32 - 8 - 1);       // 0x800000
        do { c = (bitbuf & mask) ? cx_right[c] : cx_left[c]; mask >>= 1; } while (c >= 13);
    }
    fillbuf(pt_len[c]);
    if (c != 0)
        c = (1u << (c - 1)) + getbits(c - 1); // c-th distance bucket
    return c;                                 // 0..4094  (decode_ adds +1)
}
```

---

## 8. Code-length table readers

### `read_pt_len(int nn, int nbit, int i_special)`  — for `pt_len` / `pt_table`

```c
uint n = getbits(nbit);
if (n == 0) {
    uint c = getbits(nbit);
    for (i = 0; i < nn;  i++) pt_len[i]  = 0;
    for (i = 0; i < 256; i++) pt_table[i] = c;   // degenerate: single symbol
} else {
    i = 0;
    while (i < n) {
        uint c = bitbuf >> (32 - 3);             // 3-bit length
        if (c == 7) {                            // 111 + unary tail
            uint mask = 1u << (32 - 4);
            while (bitbuf & mask) { mask >>= 1; c++; }
        }
        fillbuf(c < 7 ? 3 : c - 3);
        pt_len[i++] = c;
        if (i == i_special) {                    // run of zeros, only for the NT table
            uint z = getbits(2);
            while (z-- > 0) pt_len[i++] = 0;
        }
    }
    while (i < nn) pt_len[i++] = 0;
    make_table(nn, pt_len, 8, pt_table);         // tablebits = 8
}
```

### `read_c_len()`  — for `c_len` / `c_table`, decoded through the just-built `pt` tree

```c
uint n = getbits(9);                             // CBIT
if (n == 0) {
    uint c = getbits(9);
    for (i = 0; i < 510;  i++) c_len[i]  = 0;
    for (i = 0; i < 4096; i++) c_table[i] = c;
} else {
    i = 0;
    while (i < n) {
        uint c = pt_table[bitbuf >> (32 - 8)];
        if (c >= 19) {                           // NT: walk pt tree
            uint mask = 1u << (32 - 8 - 1);
            do { c = (bitbuf & mask) ? cx_right[c] : cx_left[c]; mask >>= 1; } while (c >= 19);
        }
        fillbuf(pt_len[c]);
        if (c <= 2) {
            if      (c == 0) c = 1;
            else if (c == 1) c = getbits(4) + 3;
            else             c = getbits(9) + 20;   // CBIT
            while (c-- > 0)  c_len[i++] = 0;
        } else {
            c_len[i++] = c - 2;
        }
    }
    while (i < 510) c_len[i++] = 0;
    make_table(510, c_len, 12, c_table);          // tablebits = 12
}
```

---

## 9. `make_table(nchar, bitlen[], tablebits, table[])`  = `sub_17DCC0`

Stock LHA `maketbl.c` canonical-Huffman table builder. Fills `table[]` (the
`1 << tablebits` fast-lookup array) directly for codes ≤ `tablebits`, and
extends into `cx_left[]` / `cx_right[]` (allocating node indices from
`nchar` upward) for longer codes. No behavioural surprises; a reimplementation
can drop in any reference `make_table` unchanged.

`sub_17DCC0` was listed as "not yet characterized" in
[`drawing_primitives.md`](drawing_primitives.md) §"remaining unnamed" — it is
**not** a CRT/graphics-lib internal, it's this codec's `make_table`.

---

## 10. Symbol map (for renaming the IDB)

| Current | Should be | Kind |
|---|---|---|
| `CXBUF2BUFEXPAND` | *(keep)* — fix the comment/prototype | fn |
| `decode_` | `lzh_decode` | fn |
| `decode_start` | `lzh_decode_start` | fn |
| `decode_c` | `lzh_decode_c` | fn |
| `decode_p` | `lzh_decode_p` | fn |
| `read_c_len` | *(keep)* | fn |
| `read_pt_len` | *(keep)* | fn |
| `sub_17DCC0` | `make_table` | fn |
| `huf_decode_start` | *(keep)* | fn |
| `fillbuf` / `getbits` / `init_getbits` | *(keep)* | fn |
| `read_byte` / `write_byte` | *(keep)* | fn |
| `_cx_lBytesLeft?` | `_lzh_decode_j` | data (int, static match-run remainder) |
| `dword_1AB020` | `_lzh_decode_i` | data (uint, window cursor) |
| `dword_1A9F28` | `_cx_InBytesLeft` | data (uint, compressed bytes remaining) |
| `_cx_nBit_Buffer` | `_cx_bitbuf` | data (u32) |
| `_cx_blocksize` | *(keep)* | data (int) |
| `_c_table` / `_pt_table` / `_c_len` / `_pt_len` | *(keep)* | data |
| `_cx_left` / `_cx_right` | `_lzh_left` / `_lzh_right` | data (Huffman node arrays) |

See `reversing/scripts/apply_lzh_findings.py`.

---

## 11. What's left

- **Encoder** (`CXBUF2BUFCOMPRESS` / `ENCODE_BUF` / `huf_encode_*` /
  `send_block` / `make_tree` / `count_len` / `make_len` / `_p_freq` /
  `_cx_heap` / `_pt_code`) is fully linked in and is the matching LHA
  `-lh4-` *encoder*. Not needed to read game data; only relevant if the
  reimplementation ever needs to *write* the format. Not traced.
- The `Pic` on-disk container around the compressed blob (how `srcLen` /
  `w*h` reach `gxVirtualDecompress`, the 17-byte gx header, animation frame
  arrays) is covered in `engine_overview.md` §3–4 and is unaffected by this
  pass.
- `sub_17DC6C` (the other "uncharacterized" fn from `drawing_primitives.md`)
  is still unidentified — it is *not* part of this codec.
