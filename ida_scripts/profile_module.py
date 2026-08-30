"""
Read-only: profile the coerced BASIC code of a client module to support
naming. Auto-detects the code segment (the one whose `9A` far calls hit
the int-3Fh thunk table) and the DGROUP data segment. For every function:
callers, module-local callees, the rtm_* run-time routines it invokes
(top by count), and any immediate that points at a string in DGROUP or
the code-segment tail (resolved to the text).

    .\run_ida_script.ps1 -Idb out -ScriptName profile_module.py -NoExport
"""

import idc
import idautils
import ida_bytes
import ida_segment
import ida_funcs


def find_thunk_span():
    best = (0, 0, 0)
    for i in range(ida_segment.get_segm_qty()):
        seg = ida_segment.getnseg(i)
        run0 = run1 = None
        gap = 0
        ea = seg.start_ea
        while ea < seg.end_ea - 3:
            if ida_bytes.get_byte(ea) == 0xCD and ida_bytes.get_byte(ea + 1) == 0x3F:
                b2 = ida_bytes.get_byte(ea + 2)
                sz = 4 if b2 in (0xFE, 0xFF) else 3
                if run0 is None:
                    run0 = ea
                run1 = ea + sz
                ea += sz
                gap = 0
            else:
                ea += 1
                gap += 1
                if gap > 16 and run0 is not None:
                    if run1 - run0 > best[1] - best[0]:
                        best = (run0, run1)
                    run0 = None
        if run0 is not None and run1 - run0 > best[1] - best[0]:
            best = (run0, run1)
    return best


TH_LO, TH_HI = find_thunk_span()


def pick_segs():
    code = data = None
    best_n = -1
    for i in range(ida_segment.get_segm_qty()):
        seg = ida_segment.getnseg(i)
        n = printable = 0
        for a in range(seg.start_ea, seg.end_ea - 4, 1):
            if ida_bytes.get_byte(a) == 0x9A:
                off = ida_bytes.get_word(a + 1)
                sel = ida_bytes.get_word(a + 3)
                for base in (0x1000, 0):
                    if TH_LO <= ((sel + base) << 4) + off < TH_HI:
                        n += 1
            b = ida_bytes.get_byte(a)
            if 0x20 <= b <= 0x7e:
                printable += 1
        size = seg.end_ea - seg.start_ea
        if n > best_n:
            code, best_n = seg, n
        if size > 1024 and printable > size * 0.45 and \
           (data is None or size > data.end_ea - data.start_ea):
            data = seg
    return code, data


CODE_SEG, DATA_SEG = pick_segs()
S0, S0E = CODE_SEG.start_ea, CODE_SEG.end_ea
DS = DATA_SEG.start_ea if DATA_SEG else None
DE = DATA_SEG.end_ea if DATA_SEG else None


def str_near(ea):
    start = ea
    while start > ea - 96:
        b = ida_bytes.get_byte(start - 1)
        if b in (0, 0x24) or b < 0x20 or b > 0x7e:
            break
        start -= 1
    raw = bytearray()
    p = start
    while p < start + 120:
        b = ida_bytes.get_byte(p)
        if b in (0, 0x24):
            break
        raw.append(b)
        p += 1
    if len(raw) < 3:
        return None
    txt = "".join(chr(b) if 0x20 <= b <= 0x7e else "." for b in raw[:60])
    d = ea - start
    return f'"{txt}"' + (f" (+{d})" if d else "")


def resolve_imm(v):
    """v as a near pointer into DGROUP or the code-seg string tail."""
    for base, lo, hi in ((DS, 0, (DE - DS) if DS else 0),
                         (S0, 0, S0E - S0)):
        if base is None:
            continue
        if lo <= v < hi:
            s = str_near(base + v)
            if s:
                return s
    return None


def main():
    funcs = sorted(f for f in idautils.Functions(S0, S0E))
    print(f"code {ida_segment.get_segm_name(CODE_SEG)} {S0:#x}-{S0E:#x}; "
          f"thunks {TH_LO:#x}-{TH_HI:#x}; "
          f"dgroup {ida_segment.get_segm_name(DATA_SEG) if DATA_SEG else '?'}")
    print(f"{len(funcs)} functions\n")
    for f in funcs:
        end = idc.get_func_attr(f, idc.FUNCATTR_END)
        name = idc.get_func_name(f)
        callers = sorted({idc.get_func_name(x) or f"{x:#x}"
                          for x in idautils.CodeRefsTo(f, 0)})
        rtm, callees, texts = {}, set(), []
        for h in idautils.Heads(f, end):
            if not ida_bytes.is_code(ida_bytes.get_full_flags(h)):
                continue
            if idc.print_insn_mnem(h) == "call":
                t = idc.get_operand_value(h, 0)
                tn = idc.get_name(t) or f"{t:#x}"
                if tn.startswith("rt_"):
                    rtm[tn] = rtm.get(tn, 0) + 1
                elif S0 <= t < S0E:
                    callees.add(idc.get_func_name(t) or f"{t:#x}")
            for opn in (0, 1):
                if idc.get_operand_type(h, opn) == idc.o_imm:
                    v = idc.get_operand_value(h, opn)
                    s = resolve_imm(v) if 0x40 <= v < 0x10000 else None
                    if s:
                        texts.append((h, v, s))
        print(f"=== {name}  {f:#x}-{end:#x} ({end - f}b)")
        print(f"    callers: {', '.join(callers) or '-'}")
        if callees:
            print(f"    -> {', '.join(sorted(callees))}")
        top = sorted(rtm.items(), key=lambda kv: -kv[1])[:8]
        print(f"    rtm: {', '.join(f'{k}x{v}' for k, v in top) or '-'}")
        seen = set()
        for h, v, s in texts:
            if v in seen:
                continue
            seen.add(v)
            print(f"      {v:#06x} -> {s}")
        print()


main()
