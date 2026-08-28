"""
Applies this reversing pass's findings back into the IDA database:
renames + explanatory comments for functions resolved in
reversing/analysis/entry_and_init_flow.md, drawing_primitives.md, and
compression_lzhuf.md.

Run this INSIDE IDA (File > Script file..., Alt+F7) against
`Shannara Demo.idb` while it's open in the GUI, or headlessly via:

    idat64 -A -S"apply_re_findings.py" "Shannara Demo.idb"

Idempotent - safe to run more than once (renames/comments just get
overwritten with the same values). Only touches functions that already
exist; skips anything it can't resolve by name rather than guessing.
"""
import idc

RESULTS = []


def resolve(name_or_ea):
    if isinstance(name_or_ea, int):
        return name_or_ea
    ea = idc.get_name_ea_simple(name_or_ea)
    return ea


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


# --- drawing_primitives.md ---------------------------------------------
RENAMES = [
    ("sub_167CE8", "set_gr_op",
     "Sets the global _gr_op raster-op/write-mode selector (0-4). Sibling "
     "setter to set_gr_color. Identified in an RE pass - see "
     "reversing/analysis/drawing_primitives.md"),
    ("sub_167CF4", "get_gr_color",
     "Returns the current _gr_color pen color. Called from gxVirtualVirtual "
     "to save/restore pen color around a blit. See "
     "reversing/analysis/drawing_primitives.md"),
    ("sub_167D08", "get_pixel",
     "get_pixel(x=ax, y=dx): clip-tests against _clipx1/_clipx2/_clipy1/"
     "_clipy2, then gxVideoAddr + byte read. Returns 0 if outside the clip "
     "rect. See reversing/analysis/drawing_primitives.md"),
    ("sub_167D54", "put_pixel",
     "put_pixel(x=ax, y=cx, color=bl): clip-tested; dispatches on _gr_op "
     "(0=direct write, 3=XOR, 1/2/4=no pixel write but still calls "
     "gxDirtyDisplay). See reversing/analysis/drawing_primitives.md"),
    ("sub_167334", "draw_line",
     "Clipped line-draw primitive. Distinguishes _gr_op==0 (direct write "
     "w/ _gr_color) vs !=0 (XOR). Axis-aligned case fully traced; the "
     "diagonal/Bresenham branch (past loc_167424) was NOT fully traced this "
     "pass. See reversing/analysis/drawing_primitives.md"),
    ("sub_1672D0", "line_to",
     "LineTo-style primitive: draws from the current pen position "
     "(_cpx/_cpy) to (eax,edx) via draw_line, then updates _cpx/_cpy to the "
     "new point. Called from _draw_bevel and draw_rect_. See "
     "reversing/analysis/drawing_primitives.md"),
    # --- entry_and_init_flow.md ---
    ("sub_1574EC", "music_to_sound_automatch",
     "Not really a function - a hardcoded jump-table correlation table "
     "mapping a chosen _music[] index (eax) to a suggested _sound[] index: "
     "0/1/2 -> 0, 3..9 -> idx-1, else -1 (no auto-match). Called from "
     "setup_demo. See reversing/analysis/entry_and_init_flow.md"),
]

COMMENTS_ONLY = [
    ("sub_15E930",
     "STUB / dead code: `xor eax,eax; mov eax,eax; ret` - always returns 0. "
     "The incoming filename-pointer parameter (eax) is completely unused. "
     "Called twice from init_dib_file. Possibly a compiled-out DIB-cache "
     "lookup hook, or an unconditionally-false check in this build - worth "
     "checking whether the full-game binary has real logic at the "
     "equivalent location. See reversing/analysis/drawing_primitives.md"),
    ("q_init",
     "Phase-2 auto-signature undercounts params (reported as a1@edx only - "
     "a documented instance of the 'undercounts params read after the "
     "function's first branch' limitation). Real signature: "
     "q_init(edx=yOrigin, ebx=palette ptr). The incoming ecx (main passes "
     "the LOOPING flag here) is pushed/popped unused, and incoming eax is "
     "never read - neither is a real parameter, despite superficially "
     "looking like ones at the call site. See "
     "reversing/analysis/entry_and_init_flow.md"),
    # --- compression_lzhuf.md ---
    ("CXBUF2BUFEXPAND",
     "LZHUF (LZSS + adaptive Huffman) decompressor - matches Yoshizaki's "
     "classic lzhuf.c/LHA algorithm by exact internal function names "
     "(fillbuf/getbits/decode_start/read_pt_len/read_c_len/decode_c/"
     "decode_p, all present in this binary). Called from "
     "gxVirtualDecompress whenever Pic.isCompressed is set - this is NOT "
     "simple RLE, correcting an earlier analysis pass. Signature (from the "
     "call site): CXBUF2BUFEXPAND(dstSize, srcSize, srcPtr, dstPtr). See "
     "reversing/analysis/compression_lzhuf.md"),
    ("gxVirtualDecompress",
     "Decompression path is LZHUF via CXBUF2BUFEXPAND, NOT RLE (correction "
     "to an earlier analysis pass which assumed RLE). See "
     "reversing/analysis/compression_lzhuf.md"),
]


def main():
    for old_name, new_name, comment in RENAMES:
        rename_and_comment(old_name, new_name, comment)
    for name, comment in COMMENTS_ONLY:
        comment_only(name, comment)

    print("\n".join(RESULTS))
    print(f"\n{sum(1 for r in RESULTS if r.startswith('OK'))} applied, "
          f"{sum(1 for r in RESULTS if r.startswith('SKIP'))} skipped, "
          f"{sum(1 for r in RESULTS if r.startswith('WARN'))} warnings")

    idc.save_database(idc.get_idb_path(), 0)
    print("Database saved.")


main()
