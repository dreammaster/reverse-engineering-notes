# Resource compression is LZHUF (LZSS + adaptive Huffman), not RLE

> **Superseded (deep-dive complete):** see
> [`lzh_decode_spec.md`](lzh_decode_spec.md) for the full line-by-line decode
> spec (exact parameters, every routine as C). Two things this page got
> slightly wrong, corrected there:
> 1. It's **LHA `-lh4-`** (per-block *static* canonical Huffman, `DICBIT=12`),
>    not Okumura's 1988 *adaptive* `lzhuf.c`. The routine names
>    (`decode_c`/`decode_p`/`read_c_len`/`read_pt_len`/`make_table`) are LHA
>    `huf.c`, not `lzhuf.c`.
> 2. `CXBUF2BUFEXPAND`'s real arg order is `(srcLen, dstLen, dst, src)`, not
>    `(dstSize, srcSize, srcPtr, dstPtr)`.
> The call-graph / "this IS lzh" identification below still stands.

**Correction to `engine_overview.md`**, which describes `Pic.isCompressed`
as gating "RLE decompression" via `gxVirtualDecompress`. That's wrong (or at
least incomplete) — the actual decompressor statically linked into the
binary, `CXBUF2BUFEXPAND` (.asm line 35448), is a byte-for-byte match, by function
naming, for **Haruyasu "Yoshi" Yoshizaki's LZHUF algorithm** (the LZSS+
adaptive-Huffman coder behind the classic Japanese `LHA`/`.lzh` archiver and
its widely-reused public-domain `huf.c`/`lzhuf.c` reference source from the
late 1980s). This is VERY HIGH confidence — not a guess from behavior, but
an exact match of the source's own internal function names, still present
as symbols in this binary:

| Symbol in `Shannara Demo.asm` | Line # in the .asm export | Role in the canonical LZHUF source |
|---|---|---|
| `fillbuf` | 59941 | refills the bit-input buffer |
| `getbits` | 60027 | reads N bits from the bit-input stream |
| `decode_start` | 60611 | resets/inits the decoder's Huffman tables |
| `cx_malloc` / `cx_free` | 60318 / 60329 | the decompressor's private allocator wrappers |
| `decode_` | 60626 | per-symbol decode dispatcher |
| `read_pt_len` | 74621 | reads the "position" (distance) Huffman code-length table |
| `read_c_len` | 74748 | reads the "character" (literal/length) Huffman code-length table |
| `decode_c` | 74892 | decodes one literal-or-length symbol |
| `decode_p` | 74958 | decodes one match-position (distance) symbol |
| `CXBUF2BUFEXPAND` | 35448 | top-level entry point, called from `gxVirtualDecompress` |

(Line numbers are from this specific `.asm` export and will shift on
re-export — use the symbol names to re-locate via grep. Real hex addresses
weren't pulled for this table; grep each name in the .asm for its `loc_`/xref
context to get them if needed.)

(`decode_` — with the trailing underscore — is the umbrella per-symbol
decode dispatcher that `CXBUF2BUFEXPAND` calls in a loop; it in turn calls
`decode_c`/`decode_p`.)

## Call path

```
load_pic -> gxVirtualDecompress(gxHeaderPtr, di_flag)
              -> [if Pic.isCompressed] CXBUF2BUFEXPAND(dstSize, srcSize=w*h, srcPtr, dstPtr)
                   -> decode_start()               reset Huffman tables
                   -> loop: decode_()               -> decode_c() / decode_p()
                                                        (reading bits via getbits/fillbuf)
```

`gxVirtualDecompress` itself (.asm line ~12002) is otherwise just `gx*`
buffer-management glue (`gxDestroyVirtual`/`gxCreateVirtual`/lock-unlock
handle dance around the actual pixel buffer) — the real compressed-data
codec is entirely inside `CXBUF2BUFEXPAND` and its `decode_*`/`read_*`/
`getbits`/`fillbuf` helpers. This is the **same decompressor used for any
`Pic` with `isCompressed` set** (i.e. essentially all real in-game art —
the demo's own splash art is presumably compressed this way too), and is
architecturally separate from the in-house block-based "Q" video codec
(`QHeader`/`q_expand`/`sub_1652E0`) used for full-motion cutscenes.

## Why this matters for reimplementation

LZHUF/LHA is public, extremely well-documented, and has many existing clean
reference implementations (the original Yoshizaki/Okumura `lzhuf.c`, and
countless later ports). Reimplementing Shannara's picture decompression
almost certainly means **porting/adapting a standard LZHUF decoder** rather
than reverse-engineering a bespoke bit format from scratch — a substantial
shortcut for the eventual reimplementation. The main things still needed to
nail the exact variant in use:

- Confirm the specific LZHUF parameters (window size, `NC`/`NP`/`NT`
  Huffman table sizes, min-match length) by reading `CXBUF2BUFEXPAND`,
  `decode_start`, and `decode_` in full — not done yet this pass beyond
  identifying the call graph and confirming this IS lzhuf.
- Confirm whether this is stock LZHUF or a modified/renamed variant (Legend
  may have tweaked constants) by diffing against a reference `lzhuf.c`.
- `CXBUF2BUFEXPAND`'s signature (from its call site in
  `gxVirtualDecompress`): `CXBUF2BUFEXPAND(dstSize, srcSize, srcPtr, dstPtr)`
  (stack args, right-to-left per the pushes) — a standard
  `decompress(dst, dstLen, src, srcLen)`-shaped entry point, good news for
  porting since it's not deeply entangled with other engine state beyond
  its own static Huffman-table workspace.

## Suggested follow-up

This is a strong candidate for the **next** deep-dive session: fully trace
`CXBUF2BUFEXPAND` + `decode_`/`decode_c`/`decode_p`/`read_c_len`/
`read_pt_len` (roughly 6 functions, a few hundred lines total) and write out
the exact bitstream format, then cross-check against a reference `lzhuf.c`
to confirm/adapt constants. That would fully unlock picture-data decoding
for the reimplementation, independent of understanding any more game logic.
