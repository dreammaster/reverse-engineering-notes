"""
Read-only: recover a module's screen-string constants and cross-reference
them to the code.

Screen strings live in a pool of `[dw length][dw ptr][data..length]`
records (see docs/file-formats.md). Two arrangements:

  * menu.exe -- the pool is the DGROUP segment itself, `ptr` is
    self-relative (points at the 4 bytes right after the descriptor);
    code does `mov ax,<descaddr> / push / call basStrAssign`.
  * out.exe (and the other EXEPACK'd modules) -- the pool sits in the
    code segment's tail as a DGROUP *initialiser* list; `ptr` is the
    destination offset in the (BSS) DGROUP segment that the startup code
    copies each record to. Code immediates are those DGROUP offsets.

This walks every segment for the record pattern, builds
`ptr -> text`, and lists which `mov reg,imm` sites in the code reference
each. `%` in the text = newline; leading `%`s position vertically;
trailing `! # $ &` are drawString paragraph/page directives.

    .\run_ida_script.ps1 -Idb out -ScriptName dump_strings.py -NoExport
"""

import idc
import idautils
import ida_bytes
import ida_segment

ANNOTATE = True    # write the decoded text as a comment at each ref


def printable_ratio(b):
    if not b:
        return 0.0
    return sum(1 for c in b if 0x20 <= c <= 0x7e or c in (0, 0x0d, 0x0a)) / len(b)


def find_records():
    """[(desc_ea, length, ptr, text)] across all segments."""
    out = []
    for i in range(ida_segment.get_segm_qty()):
        seg = ida_segment.getnseg(i)
        ea = seg.start_ea
        while ea < seg.end_ea - 5:
            ln = ida_bytes.get_word(ea)
            ptr = ida_bytes.get_word(ea + 2)
            if 1 <= ln <= 200:
                data = ida_bytes.get_bytes(ea + 4, ln) or b""
                alpha = sum(1 for c in data if 0x41 <= c <= 0x5a or 0x61 <= c <= 0x7a)
                if len(data) == ln and printable_ratio(data) >= 0.85 and alpha >= 2:
                    out.append((ea, ln, ptr, data))
                    ea += 4 + ln
                    while ea < seg.end_ea and ida_bytes.get_byte(ea) in (0, 0xF4):
                        ea += 1
                    continue
            ea += 1
    return out


def main():
    recs = find_records()
    print(f"{len(recs)} string records\n")

    # Code pushes the DESCRIPTOR address, which sits 4 bytes before the
    # string data the record populates (`mov ax,2548h` -> data at 254Ch).
    # Match on ptr-4 (and the raw ptr / descriptor ea as fallbacks).
    by_ref = {}
    for ea, ln, ptr, data in recs:
        for k in (ptr - 4, ptr, ea):
            by_ref.setdefault(k, (ea, ln, ptr, data))

    rec_refs = {}   # record desc_ea -> [code ea]
    for seg_i in range(ida_segment.get_segm_qty()):
        seg = ida_segment.getnseg(seg_i)
        for h in idautils.Heads(seg.start_ea, seg.end_ea):
            if not ida_bytes.is_code(ida_bytes.get_full_flags(h)):
                continue
            for opn in (0, 1):
                if idc.get_operand_type(h, opn) == idc.o_imm:
                    hit = by_ref.get(idc.get_operand_value(h, opn))
                    if hit:
                        rec_refs.setdefault(hit[0], []).append(h)

    def render(data):
        # `%` = newline; show it as \n so the layout is legible
        return "".join("\\n" if c == 0x25 else chr(c) if 0x20 <= c <= 0x7e
                       else {0: "", 0x0d: "", 0x0a: ""}.get(c, f"\\x{c:02x}")
                       for c in data)

    n_ref = 0
    for ea, ln, ptr, data in recs:
        refs = rec_refs.get(ea, [])
        if refs:
            n_ref += 1
        rs = ("  <- " + ", ".join(f"{r:#x}" for r in refs[:8])) if refs else ""
        print(f"  {ea:#07x} -> dgrp {ptr:#06x} [{ln:3d}]  {render(data)!r}{rs}")
        if ANNOTATE:
            for r in refs:
                idc.set_cmt(r, render(data)[:72], 0)

    print(f"\n{len(recs)} records, {n_ref} referenced from code")


main()
