"""
Generic structural pass: turn a Legacy of the Ancients client module's
compiled-BASIC code segment into disassembled, function-carved code.

IDA's MZ loader leaves it as raw `db` -- nothing references it as code
(the entry is in the RTM-loader segment and real entry only happens via
the int-3Fh self-patch at run time, see resolve_rtm_leglib.py). The body
is ~pure code: a dense stream of `call far` trampolines with short
argument set-up between them. Two `call far` forms:

  * `9A <off> <thunk-selector>`  -> the module's int-3Fh thunk table
  * `9A <off> 0000`              -> the module's own BASIC SUB/FUNCTIONs
                                    (segment word 0 = image base)

Everything is auto-detected (thunk table location, selectors, code
segment, header size) so this works for every module regardless of how
its segments are split -- menu.exe keeps the thunk table in its own
segment; out.exe embeds it mid-code in seg000.

Algorithm: anchor both `call far` forms, linear-sweep the gaps between
consecutive anchors (skipping the thunk-table hole), make each called
thunk a returning function so flow continues past every call, wipe IDA's
fragmented auto-functions and re-carve at real entry points, then merge
the int-3Fh-induced fragmentation. The cosmetic "fall-through cref past
every call far" pass lives in apply_renames_<module>.py (it must run
after resolve_thunks.py's final reanalysis). Pipeline order:
  resolve_thunks -> coerce_code -> apply_renames_<m> -> resolve_thunks

    .\run_ida_script.ps1 -Idb out -ScriptName coerce_code.py
"""

import idc
import idautils
import ida_bytes
import ida_funcs
import ida_auto
import ida_segment

DRY_RUN = False


def is_code(ea):
    return ida_bytes.is_code(ida_bytes.get_full_flags(ea))


def is_undef(ea):
    return ida_bytes.is_unknown(ida_bytes.get_full_flags(ea))


def find_thunks():
    best = []
    for i in range(ida_segment.get_segm_qty()):
        seg = ida_segment.getnseg(i)
        run, gap, ea = [], 0, seg.start_ea
        while ea < seg.end_ea - 3:
            if ida_bytes.get_byte(ea) == 0xCD and ida_bytes.get_byte(ea + 1) == 0x3F:
                b2 = ida_bytes.get_byte(ea + 2)
                sz = 4 if b2 in (0xFE, 0xFF) else 3
                run.append((ea, sz))
                ea += sz
                gap = 0
            else:
                ea += 1
                gap += 1
                if gap > 16 and run:
                    if len(run) > len(best):
                        best = run
                    run = []
        if len(run) > len(best):
            best = run
    return dict(best)


THUNKS = find_thunks()
TH_LO = min(THUNKS)
TH_HI = max(THUNKS) + THUNKS[max(THUNKS)]


def resolve_far(off, sel):
    """linear target of `9A off sel`, trying image-base 0 and 0x1000."""
    for base in (0x1000, 0x0):
        yield ((sel + base) << 4) + off


def pick_code_seg():
    """the segment whose `9A` far calls hit the thunk table -- for
    menu.exe that's seg000 (thunks are in seg001); for out.exe it's the
    same seg000 the thunks are embedded in."""
    best, best_n = None, -1
    for i in range(ida_segment.get_segm_qty()):
        seg = ida_segment.getnseg(i)
        n = 0
        for a in range(seg.start_ea, seg.end_ea - 4):
            if ida_bytes.get_byte(a) != 0x9A:
                continue
            off, sel = ida_bytes.get_word(a + 1), ida_bytes.get_word(a + 3)
            if any(TH_LO <= lin < TH_HI for lin in resolve_far(off, sel)):
                n += 1
        if n > best_n:
            best, best_n = seg, n
    return best


CODE_SEG = pick_code_seg()
S0, S0E = CODE_SEG.start_ea, CODE_SEG.end_ea
SEGNAME = ida_segment.get_segm_name(CODE_SEG)


def header_end():
    for a in range(S0 + 0x18, S0 + 0x48):
        b = ida_bytes.get_byte(a)
        if b in (0x33, 0xB8, 0x55, 0x9A, 0x2E, 0xFC, 0xE8, 0xE9):
            return a - S0
    return 0x31


HEADER_END = header_end()

# Code span(s) to sweep. When the thunk table lives in its own segment
# (menu.exe) the whole code segment is BASIC code. When it's embedded
# mid-segment (out.exe) only the part *before* it is BASIC code -- the
# part after is the RTM-loader stub + its `$`-terminated DOS message
# strings, which must not be blind-swept. All module SUB/FUNCTION
# targets are confirmed to sit before the table, so dropping the tail
# loses no procedures.
if TH_LO <= S0 or TH_HI >= S0E:      # table not inside this segment
    SPANS = [(S0 + HEADER_END, S0E)]
else:
    SPANS = [(S0 + HEADER_END, TH_LO)]


def in_span(ea):
    return any(lo <= ea < hi for lo, hi in SPANS)


def coverage():
    insn = data = undef = 0
    for lo, hi in SPANS:
        for a in range(lo, hi):
            f = ida_bytes.get_full_flags(a)
            if ida_bytes.is_code(f):
                insn += 1
            elif ida_bytes.is_tail(f):
                if ida_bytes.is_code(ida_bytes.get_full_flags(ida_bytes.get_item_head(a))):
                    insn += 1
                else:
                    data += 1
            elif ida_bytes.is_data(f):
                data += 1
            else:
                undef += 1
    return insn, data, undef


def span_bytes():
    return sum(hi - lo for lo, hi in SPANS)


def looks_like_string(a, hi):
    """>=8 printable bytes (most of them letters/space/'$') then a NUL ->
    a C string. The RTM-loader messages and chained-EXE names ("Error in
    loading RTM: ", "LEGLIB.EXE - $", "MUS.EXE") sit inside the code seg;
    without this the sweep eats them as garbage instructions. Tight enough
    not to fire on `push`-run opcode bytes."""
    p, texty = a, 0
    while p < hi and p < a + 128:
        b = ida_bytes.get_byte(p)
        if b == 0:
            return p - a >= 8 and texty >= (p - a) * 3 // 4
        if b < 0x20 or b > 0x7e:
            return False
        if b == 0x20 or b == 0x24 or 0x41 <= b <= 0x5a or 0x61 <= b <= 0x7a:
            texty += 1
        p += 1
    return False


def sweep(lo, hi):
    n, a = 0, lo
    while a < hi:
        if is_code(a):
            a = max(ida_bytes.get_item_end(a), a + 1)
            continue
        if looks_like_string(a, hi):
            end = a
            while ida_bytes.get_byte(end) != 0:
                end += 1
            ida_bytes.del_items(a, ida_bytes.DELIT_SIMPLE, end + 1 - a)
            idc.create_strlit(a, end + 1)
            a = end + 1
            continue
        if not is_undef(a):
            ida_bytes.del_items(a, ida_bytes.DELIT_SIMPLE, 1)
        ln = idc.create_insn(a)
        if ln > 0:
            n += 1
            a += ln
        else:
            a += 1
    return n


def reset():
    for f in list(idautils.Functions(S0, S0E)):
        if in_span(f):
            ida_funcs.del_func(f)
    for lo, hi in SPANS:
        ida_bytes.del_items(lo, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC, hi - lo)
    ida_auto.auto_wait()


def main():
    print(f"code seg {SEGNAME} {S0:#x}-{S0E:#x}; thunk table {TH_LO:#x}-{TH_HI:#x} "
          f"({len(THUNKS)} thunks); header {HEADER_END:#x}")
    print(f"sweep spans: {[f'{lo:#x}-{hi:#x}' for lo, hi in SPANS]} "
          f"({span_bytes()} bytes)")

    c0 = coverage()
    print(f"before: insn={c0[0]} data={c0[1]} undef={c0[2]}")
    if not DRY_RUN and c0[0] > 0:
        reset()
        print(f"reset:  {coverage()}")

    callsites, proc_targets, rt_used = [], set(), set()
    call_tgt = {}        # callsite ea -> resolved linear target
    n_rt = n_proc = 0
    for lo, hi in SPANS:
        for a in range(lo, hi - 4):
            if ida_bytes.get_byte(a) != 0x9A:
                continue
            off, sel = ida_bytes.get_word(a + 1), ida_bytes.get_word(a + 3)
            hit = None
            for lin in resolve_far(off, sel):
                if TH_LO <= lin < TH_HI and lin in THUNKS:
                    hit = ("rt", lin)
                    break
                if S0 <= lin < S0E and in_span(lin):
                    hit = ("proc", lin)
            if hit and hit[0] == "rt":
                callsites.append(a)
                rt_used.add(hit[1])
                call_tgt[a] = hit[1]
                n_rt += 1
            elif hit and hit[0] == "proc":
                callsites.append(a)
                proc_targets.add(hit[1])
                call_tgt[a] = hit[1]
                n_proc += 1
    callsites.sort()
    print(f"far-call sites: {len(callsites)}  ({n_rt} -> thunks, "
          f"{n_proc} -> module procs / {len(proc_targets)} distinct)")

    if DRY_RUN:
        gaps = [callsites[i + 1] - (callsites[i] + 5) for i in range(len(callsites) - 1)
                if callsites[i + 1] > callsites[i] + 5]
        if gaps:
            print(f"gaps: min={min(gaps)} max={max(gaps)} avg={sum(gaps)/len(gaps):.1f}")
        print("[DRY_RUN]")
        return

    # 1. called thunks -> tiny returning functions
    fixed = 0
    for rt in sorted(rt_used):
        sz = THUNKS[rt]
        ida_bytes.del_items(rt, ida_bytes.DELIT_SIMPLE, sz)
        idc.create_insn(rt)
        ida_bytes.create_data(rt + 2, ida_bytes.FF_BYTE, sz - 2, idc.BADADDR)
        if ida_funcs.add_func(rt, rt + sz):
            fixed += 1
        fn = ida_funcs.get_func(rt)
        if fn:
            fn.flags &= ~ida_funcs.FUNC_NORET
            ida_funcs.update_func(fn)
    ida_auto.auto_wait()
    print(f"thunks -> returning funcs: {fixed}/{len(rt_used)}")

    # 2. anchor the call-far sites; add an explicit far-call xref to the
    #    target so `call far ptr rt_<key>` renders (the thunk "segment"
    #    of an out.exe-style embedded table is not a real IDA segment, so
    #    without this the operand shows as `call far ptr 67Eh:472h`).
    import ida_xref
    made = 0
    for a in callsites:
        ida_bytes.del_items(a, ida_bytes.DELIT_SIMPLE, 5)
        if idc.create_insn(a) == 5:
            made += 1
        ida_xref.add_cref(a, call_tgt[a], ida_xref.fl_CF)
    ida_auto.auto_wait()
    print(f"anchored {made}/{len(callsites)}")

    # 3. sweep the gaps
    total = 0
    for lo, hi in SPANS:
        pts = [lo] + [a + 5 for a in callsites if lo <= a < hi] + [hi]
        for i in range(len(pts) - 1):
            total += sweep(pts[i], pts[i + 1])
    ida_auto.auto_wait()
    for p in range(3):
        n = sum(sweep(lo, hi) for lo, hi in SPANS)
        ida_auto.auto_wait()
        if n == 0:
            break
        total += n
    print(f"gap sweep: +{total} insns")

    # 4. re-carve functions at real entry points only
    for f in list(idautils.Functions(S0, S0E)):
        if in_span(f):
            ida_funcs.del_func(f)
    ida_auto.auto_wait()

    entry = next((a for lo, hi in SPANS for a in range(lo, min(lo + 0x40, hi))
                  if is_code(a)), None)
    seeds = set(proc_targets)
    if entry is not None:
        seeds.add(entry)
    for a in idautils.Heads(S0, S0E):
        if in_span(a) and is_code(a) and idc.print_insn_mnem(a) == "call" \
           and idc.get_operand_type(a, 0) == idc.o_near:
            t = idc.get_operand_value(a, 0)
            if in_span(t):
                seeds.add(t)
    n_seed = sum(1 for t in sorted(seeds) if ida_funcs.add_func(t))
    ida_auto.auto_wait()
    if entry is not None and idc.get_name(entry).startswith(("sub_", "loc_", "")):
        idc.set_name(entry, SEGNAME + "_entry", idc.SN_NOWARN)
    print(f"seed functions: {n_seed}/{len(seeds)}  (entry {entry:#x})")

    swept = 0
    for lo, hi in SPANS:
        a = lo
        while a < hi:
            if is_code(a) and not ida_funcs.get_func(a):
                if ida_funcs.add_func(a):
                    swept += 1
                nf = ida_funcs.get_func(a)
                a = nf.end_ea if nf else a + 1
            else:
                a += 1
    ida_auto.auto_wait()
    print(f"orphan-code functions: {swept}")

    # 5. merge the int-3Fh-induced fragmentation
    boundaries = set(proc_targets) | ({entry} if entry else set())
    rounds = 0
    merges = 1
    while merges and rounds < 40:
        merges, rounds = 0, rounds + 1
        for f in list(idautils.Functions(S0, S0E)):
            if not in_span(f):
                continue
            fn = ida_funcs.get_func(f)
            if not fn:
                continue
            last = idc.prev_head(fn.end_ea, fn.start_ea)
            if idc.print_insn_mnem(last) != "call":
                continue
            # a complete proc that merely ends in a call is not a fragment
            # -- only merge if the whole function has no return in it.
            if any(idc.print_insn_mnem(h) in ("retn", "retf")
                   for h in idautils.Heads(fn.start_ea, fn.end_ea)):
                continue
            succ = ida_funcs.get_func(fn.end_ea)
            if not succ or succ.start_ea != fn.end_ea \
               or succ.start_ea in boundaries or not in_span(succ.start_ea):
                continue
            new_end = succ.end_ea
            ida_funcs.del_func(succ.start_ea)
            ida_funcs.set_func_end(fn.start_ea, new_end)
            merges += 1
        if merges:
            ida_auto.auto_wait()
    print(f"call-far merges: {rounds} rounds")

    # 5b. re-sweep any code the merge / earlier passes left unowned
    #     (a call-far fragment whose successor wasn't adjacent gets
    #     orphaned; those addresses then show as `<prevfunc>+big_offset`).
    resweep = 0
    for lo, hi in SPANS:
        a = lo
        while a < hi:
            fn = ida_funcs.get_func(a)
            if fn:
                a = fn.end_ea
                continue
            if is_code(a):
                if ida_funcs.add_func(a):
                    resweep += 1
                nf = ida_funcs.get_func(a)
                a = nf.end_ea if nf else a + 1
            else:
                a += 1
    if resweep:
        ida_auto.auto_wait()
    print(f"re-swept orphan functions: {resweep}")

    # 6. fold the 3-byte `jmp <thunk>` procedure-epilogue stubs (IDA
    #    daisy-chain-names them j_j_..._rt_XX) into the function before.
    stubs = 0
    for f in list(idautils.Functions(S0, S0E)):
        fn = ida_funcs.get_func(f)
        if not fn or not in_span(f) or (fn.end_ea - fn.start_ea) > 5:
            continue
        if idc.print_insn_mnem(idc.prev_head(fn.end_ea, fn.start_ea)) != "jmp":
            continue
        prev = ida_funcs.get_func(fn.start_ea - 1)
        if prev and in_span(prev.start_ea):
            e = fn.end_ea
            ida_funcs.del_func(fn.start_ea)
            ida_funcs.set_func_end(prev.start_ea, e)
            stubs += 1
    ida_auto.auto_wait()

    # 7. cosmetic, and the LAST structural touch: add an explicit
    #    fall-through cref past every `call far` so the listing reads
    #    continuously (IDA's int-3Fh overlay special-casing otherwise
    #    chops a block break + fresh label after each). No auto_wait
    #    after -- a reanalysis reconsiders these away. resolve_thunks.py
    #    run afterwards is non-destructive and leaves them intact.
    import ida_xref
    crefs = 0
    for lo, hi in SPANS:
        for a in idautils.Heads(lo, hi):
            if is_code(a) and idc.print_insn_mnem(a) == "call" \
               and idc.get_operand_type(a, 0) == idc.o_far:
                nxt = idc.get_item_end(a)
                if nxt < hi and is_code(nxt):
                    ida_xref.add_cref(a, nxt, ida_xref.fl_F)
                    crefs += 1
    print(f"epilogue stubs folded: {stubs};  fall-through crefs: {crefs}")

    c1 = coverage()
    print(f"\nafter:  insn={c1[0]} data={c1[1]} undef={c1[2]}  "
          f"({100.0 * c1[0] / span_bytes():.1f}% covered)")
    nf = sum(1 for f in idautils.Functions(S0, S0E) if in_span(f))
    print(f"functions in code spans: {nf}")
    bad = [a for lo, hi in SPANS for a in idautils.Heads(lo, hi)
           if is_code(a) and idc.print_insn_mnem(a) == ""]
    print(f"bad insns: {len(bad)}  {', '.join(f'{x:#x}' for x in bad[:12])}")


main()
