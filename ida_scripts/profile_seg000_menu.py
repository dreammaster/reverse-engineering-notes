"""
Read-only: profile each seg000 function in menu.idb to support naming.
For every function lists: callers/callees, the run-time (rtm_*) routines
it invokes, and any immediate that points into the seg003 text block
(resolved to the string at/just before that offset).

    .\run_ida_script.ps1 -Idb menu -ScriptName profile_seg000_menu.py -NoExport
"""

import idc
import idautils
import ida_bytes
import ida_segment
import ida_funcs

s0 = ida_segment.get_segm_by_name("seg000")
s3 = ida_segment.get_segm_by_name("seg003")
S0, S0E = s0.start_ea, s0.end_ea
S3 = s3.start_ea


def str_at(off):
    """text at seg003:off, else the string that contains off."""
    ea = S3 + off
    best = None
    p = ea
    # scan back up to 64 bytes for the start of a printable run
    start = ea
    while start > ea - 80:
        b = ida_bytes.get_byte(start - 1)
        if b == 0 or b < 0x20 or b > 0x7e:
            break
        start -= 1
    raw = bytearray()
    p = start
    while p < start + 120:
        b = ida_bytes.get_byte(p)
        if b == 0:
            break
        raw.append(b)
        p += 1
    txt = raw.decode("latin1", "replace")
    delta = ea - start
    return f'"{txt[:50]}"' + (f" (+{delta})" if delta else "")


def main():
    funcs = sorted(idautils.Functions(S0, S0E))
    print(f"{len(funcs)} functions in seg000\n")
    for f in funcs:
        end = idc.get_func_attr(f, idc.FUNCATTR_END)
        name = idc.get_func_name(f)
        callers = sorted({idc.get_func_name(x) or f"{x:#x}"
                          for x in idautils.CodeRefsTo(f, 0)})
        rtm = {}
        callees = set()
        texts = []
        for h in idautils.Heads(f, end):
            if not ida_bytes.is_code(ida_bytes.get_full_flags(h)):
                continue
            m = idc.print_insn_mnem(h)
            if m == "call":
                t = idc.get_operand_value(h, 0)
                tn = idc.get_name(t) or (idc.get_func_name(t) or f"{t:#x}")
                if tn.startswith("rt_"):
                    rtm[tn] = rtm.get(tn, 0) + 1
                elif S0 <= t < S0E:
                    callees.add(idc.get_func_name(t) or f"{t:#x}")
            # immediates that look like seg003 text offsets
            for opn in (0, 1):
                if idc.get_operand_type(h, opn) == idc.o_imm:
                    v = idc.get_operand_value(h, opn)
                    if 0x2100 <= v <= 0x3200:
                        texts.append((h, v))
        size = end - f
        print(f"=== {name}  {f:#x}-{end:#x} ({size} bytes)")
        print(f"    callers: {', '.join(callers) or '-'}")
        if callees:
            print(f"    calls seg000: {', '.join(sorted(callees))}")
        top = sorted(rtm.items(), key=lambda kv: -kv[1])
        print(f"    rtm: {', '.join(f'{k}x{v}' for k, v in top) or '-'}")
        seen = set()
        for h, v in texts:
            if v in seen:
                continue
            seen.add(v)
            print(f"      {h:#x}  push {v:#06x}  -> {str_at(v)}")
        print()


main()
