"""
Headless IDA script: discover functions in a 16-bit DOS binary and dump an
inventory as JSON, for auditing the u5d decompilation against the original.

Used for the raw-binary overlays (*.OVL, *.DRV, MOSLO.COM) which load as a
single 16-bit code segment at offset 0, and for ULTIMA.EXE.

USAGE:
    idat.exe -A -c -T"Binary file" -pmetapc -b0 ^
        -S"export_functions.py <out.json> [raw]" <file>.OVL
    idat.exe -A -c -S"export_functions.py <out.json>" ULTIMA.EXE

ARGV[1] = output json path, ARGV[2] = "raw" for the binary-loader case (forces
16-bit segment and drops IDA's initial 32-bit disassembly).

Function discovery: every MSC 5.1 function starts with 'push bp; mov bp,sp'
(55 8B EC) unless it was hand-written assembly, so we seed on that pattern
and then iterate on near call targets until no new functions appear.
"""
import json
import idc
import ida_auto
import ida_bytes
import ida_funcs
import ida_ida
import ida_segment
import ida_ua
import idautils

PROLOGUE = b"\x55\x8b\xec"


def log(msg):
    with open(idc.ARGV[1] + ".log", "a") as fh:
        fh.write(msg + "\n")


def code_segments():
    for s in idautils.Segments():
        seg = ida_segment.getseg(s)
        if ida_segment.get_segm_class(seg) == "CODE" or len(list(idautils.Segments())) == 1:
            yield seg


def seed_prologues(seg):
    n = 0
    data = ida_bytes.get_bytes(seg.start_ea, seg.end_ea - seg.start_ea) or b""
    i = data.find(PROLOGUE)
    while i >= 0:
        ea = seg.start_ea + i
        if not ida_funcs.get_func(ea):
            ida_ua.create_insn(ea)
            if ida_funcs.add_func(ea):
                n += 1
        i = data.find(PROLOGUE, i + 1)
    return n


def seed_extra(seg, path):
    """Offsets known from the C sources to be functions (e.g. frameless leaf functions with no push bp)."""
    n = 0
    for off in json.load(open(path)):
        ea = seg.start_ea + off
        if seg.start_ea <= ea < seg.end_ea and not ida_funcs.get_func(ea):
            ida_ua.create_insn(ea)
            if ida_funcs.add_func(ea):
                n += 1
    return n


def sweep_function_bodies(seg):
    """Decode bytes inside function ranges that IDA left undefined (switch case bodies reached only through an
    unresolved jump table).  Jump-table words that follow the last 'ret' may decode as junk; the audit only trusts
    call targets that land on known function starts."""
    n = 0
    for fn in list(idautils.Functions(seg.start_ea, seg.end_ea)):
        f = ida_funcs.get_func(fn)
        ea = fn
        while ea < f.end_ea:
            flags = ida_bytes.get_flags(ea)
            if ida_bytes.is_code(flags):
                ea += max(1, ida_bytes.get_item_size(ea))
            elif ida_bytes.is_unknown(flags):
                sz = ida_ua.create_insn(ea)
                if sz:
                    n += 1
                    ea += sz
                else:
                    ea += 1
            else:
                ea += max(1, ida_bytes.get_item_size(ea))
    return n


def seed_call_targets(seg):
    n = 0
    for fn in list(idautils.Functions(seg.start_ea, seg.end_ea)):
        for item in idautils.FuncItems(fn):
            if idc.print_insn_mnem(item) == "call" and idc.get_operand_type(item, 0) == idc.o_near:
                tgt = idc.get_operand_value(item, 0)
                if seg.start_ea <= tgt < seg.end_ea and not ida_funcs.get_func(tgt):
                    ida_ua.create_insn(tgt)
                    if ida_funcs.add_func(tgt):
                        n += 1
    return n


def main():
    raw = len(idc.ARGV) > 2 and idc.ARGV[2] == "raw"
    out = idc.ARGV[1]
    open(out + ".log", "w").close()
    ida_auto.auto_wait()
    segs = list(code_segments())
    if raw:
        # binary loader gives a 32-bit segment; redo as 16-bit
        for seg in segs:
            idc.set_segm_attr(seg.start_ea, idc.SEGATTR_BITNESS, 0)
            idc.del_items(seg.start_ea, idc.DELIT_SIMPLE, seg.end_ea - seg.start_ea)
        ida_auto.auto_wait()
    total = 0
    seeds = idc.ARGV[3] if len(idc.ARGV) > 3 else None
    for seg in segs:
        total += seed_prologues(seg)
        ida_auto.auto_wait()
        if seeds:
            total += seed_extra(seg, seeds)
            ida_auto.auto_wait()
        for _ in range(20):
            k = seed_call_targets(seg)
            ida_auto.auto_wait()
            total += k
            if not k:
                break
    for seg in segs:
        sweep_function_bodies(seg)
    ida_auto.auto_wait()
    log("seeded %d" % total)

    result = {"input": idc.get_input_file_path(), "segments": [], "functions": []}
    for seg in segs:
        result["segments"].append({"name": ida_segment.get_segm_name(seg), "start": seg.start_ea, "end": seg.end_ea,
                                   "bitness": seg.bitness})
        for fn in idautils.Functions(seg.start_ea, seg.end_ea):
            f = ida_funcs.get_func(fn)
            calls, fcalls, imms, mems = [], [], [], []
            ninsn = njcc = nret = 0
            items = []
            ea = fn
            while ea < f.end_ea:
                if ida_bytes.is_code(ida_bytes.get_flags(ea)):
                    items.append(ea)
                ea += max(1, ida_bytes.get_item_size(ea))
            for item in items:
                m = idc.print_insn_mnem(item)
                ninsn += 1
                if m.startswith("j") and m != "jmp":
                    njcc += 1
                if m in ("retn", "retf", "ret"):
                    nret += 1
                insn = ida_ua.insn_t()
                if ida_ua.decode_insn(insn, item):
                    for op in insn.ops:
                        if op.type == ida_ua.o_void:
                            break
                        if op.type == ida_ua.o_mem:
                            mems.append([item, op.addr & 0xffff])
                        elif op.type == ida_ua.o_displ:
                            if op.phrase not in (2, 3, 6):  # bp-based => stack local/arg
                                mems.append([item, op.addr & 0xffff])
                        elif op.type == ida_ua.o_imm:
                            imms.append([item, op.value & 0xffff])
                if m == "call":
                    t = idc.get_operand_type(item, 0)
                    if t == idc.o_near:
                        calls.append(idc.get_operand_value(item, 0))
                    elif t == idc.o_far:
                        fcalls.append([idc.get_operand_value(item, 0), idc.get_operand_value(item, 0)])
                    else:
                        calls.append(-1)  # indirect
            result["functions"].append({
                "start": fn, "end": f.end_ea, "size": f.end_ea - fn,
                "prologue": ida_bytes.get_bytes(fn, 3) == PROLOGUE,
                "calls": calls, "farcalls": fcalls, "name": idc.get_func_name(fn),
                "insns": ninsn, "jcc": njcc, "rets": nret, "mems": mems, "imms": imms,
            })
    # bytes in segment not covered by any function
    gaps = []
    for seg in segs:
        cur = seg.start_ea
        for fn in idautils.Functions(seg.start_ea, seg.end_ea):
            f = ida_funcs.get_func(fn)
            if fn > cur:
                gaps.append([cur, fn])
            cur = max(cur, f.end_ea)
        if cur < seg.end_ea:
            gaps.append([cur, seg.end_ea])
    result["gaps"] = gaps
    with open(out, "w") as fh:
        json.dump(result, fh)
    log("done: %d functions, %d gaps" % (len(result["functions"]), len(gaps)))
    idc.qexit(0)


try:
    main()
except Exception:
    import traceback
    log(traceback.format_exc())
    idc.qexit(1)
