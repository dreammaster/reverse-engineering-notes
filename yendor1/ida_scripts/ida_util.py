"""
Helpers shared by the yendor1 IDA scripts.
"""
import idc
import idautils
import ida_xref

STOP = {"call", "retn", "retf", "ret", "jmp", "iret", "int"}


def reg(ea, n):
    return idc.print_operand(ea, n).strip().lower()


def find_ah(ea):  # noqa: E302
    """Return (ah or None, why, shared)."""
    cur = ea
    shared = False
    for _ in range(24):
        cur = idc.prev_head(cur)
        if cur == idc.BADADDR:
            return None, "start of segment", shared
        # a label with an incoming jump between here and the int
        nxt = idc.next_head(cur)
        for xr in idautils.XrefsTo(nxt, ida_xref.XREF_FAR):
            if xr.type in (ida_xref.fl_JN, ida_xref.fl_JF):
                shared = True
        mn = idc.print_insn_mnem(cur).lower()
        if mn in STOP:
            # a `jmp` or `call` before us means the load is on another path
            return None, f"hit {mn} at {cur:#x}", shared
        if mn not in ("mov", "xor", "sub", "lea", "pop", "inc", "dec", "add", "and", "or", "shl",
                      "shr", "xchg", "cbw", "cwd", "lodsb", "lodsw", "in", "neg", "not", "adc", "sbb", "imul", "mul", "div"):
            continue
        d = reg(cur, 0)
        if mn in ("cbw",):
            return None, f"cbw at {cur:#x}", shared
        if d not in ("ah", "ax", "eax"):
            if mn in ("xchg",) and reg(cur, 1) in ("ah", "ax"):
                return None, f"xchg at {cur:#x}", shared
            if mn in ("lodsb", "lodsw", "mul", "div", "imul", "in"):
                return None, f"{mn} at {cur:#x}", shared
            continue
        if mn == "mov" and idc.get_operand_type(cur, 1) == idc.o_imm:
            v = idc.get_operand_value(cur, 1)
            return (v if d == "ah" else (v >> 8) & 0xFF), "", shared
        if mn in ("xor", "sub") and reg(cur, 1) == d:
            return 0, "", shared
        return None, f"{mn} {d} at {cur:#x}", shared
    return None, "no load found in 24 insns", shared


