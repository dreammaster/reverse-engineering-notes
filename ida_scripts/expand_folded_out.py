"""Un-fold the collapsed functions in out.idb so they export fully.

`creatureAttack` (0x490 bytes, the overworld monster-attack turn) and
possibly a few others export as
    ; [00000490 BYTES: COLLAPSED FUNCTION creatureAttack. PRESS NUMPAD+ ...]
because their listing is folded (FUNC_HIDDEN), not because the body is
un-analysed -- IDA has instruction-level xrefs into it
(creatureAttack+457 etc).

This clears the fold flag on every folded function and un-hides any
hidden ranges, so gen_file(OFILE_ASM) emits the real body.  Read-only
w.r.t. analysis -- it only touches display flags.

    .\run_ida_script.ps1 -Idb out -ScriptName expand_folded_out.py
"""
import idc
import idautils
import ida_funcs
import ida_bytes
import ida_range

FUNC_HIDDEN = getattr(ida_funcs, "FUNC_HIDDEN", 0x10)


def main():
    unfolded = []
    for ea in idautils.Functions():
        f = ida_funcs.get_func(ea)
        if not f:
            continue
        if f.flags & FUNC_HIDDEN:
            f.flags &= ~FUNC_HIDDEN
            ida_funcs.update_func(f)
            unfolded.append((ea, idc.get_func_name(ea),
                             idc.get_func_attr(ea, idc.FUNCATTR_END) - ea))

    # also walk hidden (collapsed) ranges and expand any that are folded
    hr_expanded = 0
    try:
        n = ida_bytes.get_hidden_range_qty()
        for i in range(n):
            hr = ida_bytes.getn_hidden_range(i)
            if hr is None:
                continue
            if hr.visible == 0:
                hr.visible = 1
                ida_bytes.update_hidden_range(hr)
                hr_expanded += 1
    except Exception as e:
        print("hidden-range pass skipped:", e)

    print("unfolded functions:")
    for ea, name, size in unfolded:
        print("  %#08x  %-28s  %d bytes" % (ea, name, size))
    print("hidden ranges expanded:", hr_expanded)

    ca = idc.get_name_ea_simple("creatureAttack")
    if ca != idc.BADADDR:
        end = idc.get_func_attr(ca, idc.FUNCATTR_END)
        print("creatureAttack: %#x .. %#x  (%d bytes)" % (ca, end, end - ca))
        f = ida_funcs.get_func(ca)
        print("  flags now: %#x  (FUNC_HIDDEN %s)" %
              (f.flags, bool(f.flags & FUNC_HIDDEN)))


main()
