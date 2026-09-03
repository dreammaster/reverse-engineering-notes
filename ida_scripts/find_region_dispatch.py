"""Discover how the OUT.EXE overworld region-difficulty triples are selected.

Five orphaned code fragments sit inside out_entry (jumped over by
`jmp loc_10181`).  Each is a near-`retn` stub that stores a constant
triple into ds:208E / ds:2092 / ds:2096 -- e.g.

    push single [246E]=0.51 ; pop -> [208E]
    push single [2472]=0.22 ; pop -> [2092]
    push single [2476]=0.40 ; pop -> [2096]
    retn

2092/2096 are later read by beginEncounterView as cumulative monster-tier
probability gates.  This looks like `ON regionId GOSUB rgn0..rgn4`.

This script locates the five fragments, finds every xref into them, and
dumps any nearby word table (the GOSUB target list) plus the code that
indexes it, so we can name the selector variable.

Read-only.  Run with:
    .\run_ida_script.ps1 -Idb out -ScriptName find_region_dispatch.py -NoExport
"""
import idc
import idautils
import ida_bytes
import ida_ua

LOG = r"C:\dev\lota\ida_scripts\region_dispatch.txt"
out = []


def w(s=""):
    out.append(str(s))


def insns(ea, n):
    r = []
    for _ in range(n):
        r.append((ea, idc.generate_disasm_line(ea, 0)))
        ea = idc.next_head(ea)
    return r


def find_fragments():
    """Every ea where: mov bx,<K> ; call FF4B ; mov bx,208Eh ; call FF50."""
    frags = []
    for seg in idautils.Segments():
        for ea in idautils.Heads(idc.get_segm_start(seg), idc.get_segm_end(seg)):
            if idc.print_insn_mnem(ea) != "mov":
                continue
            if idc.print_operand(ea, 0) != "bx":
                continue
            nxt = idc.next_head(ea)
            n2 = idc.next_head(nxt)
            n3 = idc.next_head(n2)
            if "FF4B" not in idc.generate_disasm_line(nxt, 0):
                continue
            if idc.print_operand(n2, 0) == "bx" and idc.get_operand_value(n2, 1) == 0x208E:
                if "FF50" in idc.generate_disasm_line(n3, 0):
                    frags.append(ea)
    return frags


def main():
    frags = find_fragments()
    w("=== fragment starts (mov bx,K / FF4B / mov bx,208E / FF50) ===")
    for f in frags:
        k = idc.get_operand_value(f, 1)
        w("  %#08x   K=%#06x  %s" % (f, k, idc.generate_disasm_line(f, 0)))
    w()

    # widen: the ON..GOSUB body group usually starts a few bytes before the
    # first 208E store (there may be a `mov bx,<x>`/FF4B pair that pushes the
    # value for 208E from a *non-constant*).  Walk back to the nearest retn.
    for f in frags:
        start = f
        p = idc.prev_head(start)
        for _ in range(6):
            m = idc.print_insn_mnem(p)
            if m in ("retn", "jmp"):
                start = idc.next_head(p)
                break
            p = idc.prev_head(p)
        w("--- fragment %#08x (body from %#08x) ---" % (f, start))
        ea = start
        for _ in range(20):
            w("  %#08x  %s" % (ea, idc.generate_disasm_line(ea, 0)))
            if idc.print_insn_mnem(ea) == "retn":
                break
            ea = idc.next_head(ea)
        # xrefs into this fragment (any address within its first 24 bytes)
        w("  xrefs into fragment:")
        seen = set()
        for probe in range(start, start + 28):
            for xr in idautils.XrefsTo(probe, 0):
                if xr.frm in seen:
                    continue
                seen.add(xr.frm)
                w("    from %#08x  %s   (type %s)" %
                  (xr.frm, idc.generate_disasm_line(xr.frm, 0), xr.type))
        w()

    # dump word tables near the fragments: look 0x40 before the first frag
    if frags:
        lo = min(frags) - 0x60
        hi = min(frags)
        w("=== bytes/words in [%#x, %#x) (candidate GOSUB table) ===" % (lo, hi))
        ea = lo
        while ea < hi:
            ln = idc.generate_disasm_line(ea, 0)
            w("  %#08x  %s" % (ea, ln))
            ea = idc.next_head(ea)
        w()

    # who references loadOverworldData / what sets ds:2192 right before it
    lod = idc.get_name_ea_simple("loadOverworldData")
    w("=== loadOverworldData = %#x ; callers ===" % lod)
    for xr in idautils.XrefsTo(lod, 0):
        w("  called from %#08x  (%s)" % (xr.frm, idc.get_func_name(xr.frm)))
    w()

    # every store to ds:2192h, ds:2092h, ds:2096h with the containing func
    for tgt in (0x2192, 0x2092, 0x2096, 0x208E):
        w("=== writes touching ds:%04X ===" % tgt)
        for seg in idautils.Segments():
            for ea in idautils.Heads(idc.get_segm_start(seg), idc.get_segm_end(seg)):
                dl = idc.generate_disasm_line(ea, 0)
                if ("%04Xh" % tgt) in dl and ("mov" == idc.print_insn_mnem(ea)) and idc.print_operand(ea, 0).startswith("ds:") and idc.print_operand(ea,0).rstrip('h').endswith("%04X" % tgt):
                    w("  %#08x  %-40s  [%s]" % (ea, dl, idc.get_func_name(ea)))
        w()

    open(LOG, "w").write("\n".join(out))
    print("wrote", LOG)


main()
