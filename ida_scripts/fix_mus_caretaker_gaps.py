"""Coerce the raw `db` runs inside MUS.EXE's caretaker-reward subs.

sub_12AF4 and sub_12CAC (both called from caretakerOffer's accept path --
the museum-caretaker "level up" flow) each carry a big undecoded byte
blob starting right after their first `call far` -- IDA's int-3Fh
overlay-manager special-casing chops the block there instead of falling
through. Same pattern as ultima1/dun's fix_dun_coerce_gaps.py: del the
function, wipe it, re-decode linearly with a forced fall-through cref
past every `call far` (opcode 0x9A), re-add the function.

Goal: find where MUS actually writes ds:1AE0 (character level) and/or
sets questFlagWord bit 0x2000 -- currently believed to be hiding in one
of these blobs, since no `mov ds:1AE0h,...` appears anywhere in the
CURRENT (partially-coerced) mus.asm export.

    .\run_ida_script.ps1 -Idb mus -ScriptName fix_mus_caretaker_gaps.py
"""
import idc
import idautils
import ida_bytes
import ida_funcs
import ida_auto
import ida_xref

TARGETS = ["sub_12AF4", "sub_12CAC", "caretakerPraise", "sub_12C67", "sub_12DA7"]


def coerce(lo, hi):
    ida_bytes.del_items(lo, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC,
                        hi - lo)
    a, n = lo, 0
    while a < hi:
        ln = idc.create_insn(a)
        if ln <= 0:
            ida_bytes.del_items(a, ida_bytes.DELIT_SIMPLE, 1)
            ln = idc.create_insn(a)
        if ln <= 0:
            a += 1
            continue
        n += 1
        if ida_bytes.get_byte(a) == 0x9A:          # call far ptr
            ida_xref.add_cref(a, a + ln, ida_xref.fl_F | ida_xref.XREF_USER)
        a += ln
    return n


def body_stats(lo, hi):
    ins = und = dat = 0
    for a in range(lo, hi):
        fl = ida_bytes.get_full_flags(a)
        if ida_bytes.is_code(fl):
            ins += 1
        elif ida_bytes.is_unknown(fl):
            und += 1
        else:
            dat += 1
    return ins, und, dat


def main():
    for name in TARGETS:
        ea = idc.get_name_ea_simple(name)
        if ea == idc.BADADDR:
            print(f"  {name}: not found, skipping")
            continue
        f = ida_funcs.get_func(ea)
        lo, hi = f.start_ea, f.end_ea
        i0, u0, d0 = body_stats(lo, hi)
        ida_funcs.del_func(lo)
        n = coerce(lo, hi)
        ida_auto.auto_wait()
        ida_funcs.add_func(lo, hi)
        idc.set_name(lo, name, idc.SN_NOWARN | idc.SN_CHECK)
        i1, u1, d1 = body_stats(lo, hi)
        print(f"  {name:12} {lo:#08x}-{hi:#08x}  "
              f"insn {i0}->{i1}  undef {u0}->{u1}  (+{n} decoded)")
    ida_auto.auto_wait()


main()
