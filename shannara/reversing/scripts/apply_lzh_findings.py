"""
Applies the LZH/-lh4- codec deep-dive findings back into the IDA database.
Source of truth: reversing/analysis/lzh_decode_spec.md

Run INSIDE IDA (File > Script file..., Alt+F7) against `Shannara Demo.idb`,
or headlessly (only when the GUI does NOT have the .idb open):

    idat64 -A -S"reversing\\scripts\\apply_lzh_findings.py" "Shannara Demo.idb"

Idempotent - renames/comments just get overwritten with the same values.
Skips anything it can't resolve by name rather than guessing.
"""
import idc

RESULTS = []


def resolve(name_or_ea):
    if isinstance(name_or_ea, int):
        return name_or_ea
    return idc.get_name_ea_simple(name_or_ea)


def rename(old_name, new_name):
    ea = resolve(old_name)
    if ea == idc.BADADDR:
        RESULTS.append(f"SKIP  {old_name}: not found")
        return
    ok = idc.set_name(ea, new_name, idc.SN_NOWARN)
    RESULTS.append(f"{'OK  ' if ok else 'WARN'}  {old_name} -> {new_name} (0x{ea:X})")


def rename_and_comment(old_name, new_name, comment):
    ea = resolve(old_name)
    if ea == idc.BADADDR:
        RESULTS.append(f"SKIP  {old_name}: not found")
        return
    ok = idc.set_name(ea, new_name, idc.SN_NOWARN)
    idc.set_func_cmt(ea, comment, 0)
    RESULTS.append(f"{'OK  ' if ok else 'WARN'}  {old_name} -> {new_name} (0x{ea:X})")


def comment_only(name, comment):
    ea = resolve(name)
    if ea == idc.BADADDR:
        RESULTS.append(f"SKIP  {name}: not found")
        return
    idc.set_func_cmt(ea, comment, 0)
    RESULTS.append(f"OK    {name}: comment set (0x{ea:X})")


def set_type(name, decl):
    ea = resolve(name)
    if ea == idc.BADADDR:
        RESULTS.append(f"SKIP  {name}: not found (SetType)")
        return
    ok = idc.SetType(ea, decl)
    RESULTS.append(f"{'OK  ' if ok else 'WARN'}  {name}: SetType {'applied' if ok else 'REJECTED'}")


# --- functions --------------------------------------------------------------
FUNC_RENAMES = [
    ("decode_", "lzh_decode",
     "LHA -lh4- sliding-window expansion, chunked decode(count, buffer) form. "
     "buffer (the 4 KB work buf from CXBUF2BUFEXPAND) doubles as the "
     "dictionary. _lzh_decode_i (was dword_1AB020) = static copy cursor; "
     "_lzh_decode_j (was _cx_lBytesLeft?) = bytes still owed from an "
     "in-progress match, carried across calls. See "
     "reversing/analysis/lzh_decode_spec.md sec.5"),
    ("decode_start", "lzh_decode_start",
     "Resets the LZH decoder: huf_decode_start() (init_getbits + blocksize=0) "
     "then _lzh_decode_j = 0. See reversing/analysis/lzh_decode_spec.md"),
    ("decode_c", "lzh_decode_c",
     "LHA huf.c decode_c(): returns 0..255 literal or 256..509 length code. "
     "On block boundary (blocksize==0) reads blocksize=getbits(16) then the 3 "
     "code-length tables: read_pt_len(19,5,3), read_c_len(), "
     "read_pt_len(13,4,-1). 12-bit fast lookup via _c_table, overflow via "
     "_lzh_left/_lzh_right. See reversing/analysis/lzh_decode_spec.md sec.6"),
    ("decode_p", "lzh_decode_p",
     "LHA huf.c decode_p(): returns match distance-1 (0..4094). 8-bit fast "
     "lookup via _pt_table; if code c != 0 then result = (1<<(c-1)) + "
     "getbits(c-1). NP=13. See reversing/analysis/lzh_decode_spec.md sec.7"),
    ("sub_17DCC0", "make_table",
     "LHA maketbl.c make_table(nchar, bitlen, tablebits, table): canonical-"
     "Huffman fast-lookup table builder; extends into _lzh_left/_lzh_right "
     "for codes longer than tablebits. Called as make_table(nn,pt_len,8,"
     "pt_table) and make_table(510,c_len,12,c_table). NOT a CRT/graphics-lib "
     "internal (corrects drawing_primitives.md). See "
     "reversing/analysis/lzh_decode_spec.md sec.9"),
]

FUNC_COMMENTS = [
    ("CXBUF2BUFEXPAND",
     "LHA -lh4- buffer-to-buffer decompressor (per-block STATIC canonical "
     "Huffman + 4 KB LZSS window; NOT adaptive lzhuf.c, NOT RLE). "
     "Signature (args pushed R-to-L at the gxVirtualDecompress call site): "
     "int CXBUF2BUFEXPAND(u32 srcLen, u32 dstLen, void *dst, const void *src). "
     "srcLen -> dword_1A9F28 (input EOF countdown); dstLen = pic w*h. "
     "Returns 0 ok / -3 alloc fail / -1 produced-size mismatch. CRC is "
     "computed but NOT checked on this path. Full spec: "
     "reversing/analysis/lzh_decode_spec.md"),
    ("read_pt_len",
     "LHA huf.c read_pt_len(nn=eax, nbit=edx, i_special=ebx). Builds pt_len[] "
     "+ pt_table[256]. Called with (19,5,3) for the NT code-length alphabet "
     "and (13,4,-1) for the NP distance alphabet. See lzh_decode_spec.md sec.8"),
    ("read_c_len",
     "LHA huf.c read_c_len(): builds c_len[510] + c_table[4096], decoding "
     "each length through the pt tree just built by read_pt_len(19,5,3). "
     "CBIT=9. See reversing/analysis/lzh_decode_spec.md sec.8"),
    ("fillbuf",
     "LHA huf.c fillbuf(n): consume n bits from the 32-bit MSB-first bitbuf "
     "(_cx_bitbuf / _cx_nBit_Buffer), refilling a byte at a time from "
     "read_byte() while dword_1A9F28 (compressed bytes remaining) > 0, "
     "CRC-32'ing each byte. See reversing/analysis/lzh_decode_spec.md sec.4"),
    ("getbits",
     "LHA huf.c getbits(n): v = bitbuf >> (32-n); fillbuf(n); return v. "
     "n in 1..32. See reversing/analysis/lzh_decode_spec.md sec.4"),
    ("huf_decode_start",
     "LHA huf.c decode_start helper: init_getbits() + _cx_blocksize = 0. "
     "See reversing/analysis/lzh_decode_spec.md"),
]

# --- data -----------------------------------------------------------------
DATA_RENAMES = [
    ("_cx_lBytesLeft?", "_lzh_decode_j"),      # int, static match-run remainder
    ("dword_1AB020",    "_lzh_decode_i"),      # uint, sliding-window cursor
    ("dword_1A9F28",    "_cx_InBytesLeft"),    # uint, compressed bytes remaining
    ("_cx_nBit_Buffer", "_cx_bitbuf"),         # u32 MSB-first bit buffer
    ("_cx_left",        "_lzh_left"),           # Huffman node array (2*NC-1 words)
    ("_cx_right",       "_lzh_right"),
]


def main():
    for old, new, cmt in FUNC_RENAMES:
        rename_and_comment(old, new, cmt)
    for name, cmt in FUNC_COMMENTS:
        comment_only(name, cmt)
    for old, new in DATA_RENAMES:
        rename(old, new)

    # Best-effort prototype. __cdecl, 4 stack dwords.
    set_type("CXBUF2BUFEXPAND",
             "int __cdecl CXBUF2BUFEXPAND(unsigned int srcLen, unsigned int dstLen, "
             "void *dst, const void *src);")

    print("\n".join(RESULTS))
    print(f"\n{sum(1 for r in RESULTS if r.startswith('OK'))} applied, "
          f"{sum(1 for r in RESULTS if r.startswith('SKIP'))} skipped, "
          f"{sum(1 for r in RESULTS if r.startswith('WARN'))} warnings")

    idc.save_database(idc.get_idb_path(), 0)
    print("Database saved.")


main()
