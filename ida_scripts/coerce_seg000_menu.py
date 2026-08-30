"""
menu.idb structural pass: turn seg000 (the compiled-BASIC root program,
~12 KB, left entirely as raw `db` by auto-analysis) into code.

menu.exe is one compiled Microsoft BASIC 6.0 module. IDA's MZ loader
never queued seg000 because nothing references it as code -- the entry
point is in seg002 and seg000 is only ever reached through the int-3Fh
handler patching CALL FAR sites at run time (see resolve_rtm_leglib.py).

seg000's body is ~pure code. Two `call far` forms appear:
  * `9A <off> <seg001>`         -- 830 of them, into seg001's int-3Fh
                                   thunk table (run-time calls)
  * `9A <off> 0000`             -- into seg000 itself (menu's own compiled
                                   BASIC SUB/FUNCTION procedures; segment
                                   word 0 = image-base para)
each preceded by a short run of argument set-up (`mov ax,imm` / `push` /
`push [mem]`) and separated by the occasional `jmp`/`jcc`/`retf`. ~830
calls x ~15 bytes/statement fills essentially the whole 12,640-byte
segment, so:

  1. both far-call forms are certain instruction boundaries -- anchor
     them first; the seg000-targeted ones also seed function entries;
  2. every gap *between* two consecutive anchors is code (arg set-up for
     the next call) -- linear-sweep each gap;
  3. also sweep the head (entry code, after the ~0x31-byte "bmMENU"/BSS
     header) and the tail (after the last call);
  4. let IDA flow-analysis fan out, then carve functions.

String / DATA constants live in DGROUP (seg003), not here, so blind
sweeping of the gaps is safe. Any genuine embedded data (e.g. a SELECT
CASE jump table) shows up as a bad insn for a later manual pass.

Re-runnable.

    .\run_ida_script.ps1 -Idb menu -ScriptName coerce_seg000_menu.py
"""

import idc
import idautils
import ida_bytes
import ida_funcs
import ida_auto
import ida_segment
import ida_xref

DRY_RUN = False
HEADER_END = 0x31          # "bmMENU    \0" + BSS descriptor; entry code starts here

s0 = ida_segment.get_segm_by_name("seg000")
s1 = ida_segment.get_segm_by_name("seg001")
S0, S0E = s0.start_ea, s0.end_ea
S1, S1E = s1.start_ea, s1.end_ea

THUNKS = {}   # ea -> size (3 or 4)
ea = S1
while ea < S1E - 2:
    if ida_bytes.get_byte(ea) == 0xCD and ida_bytes.get_byte(ea + 1) == 0x3F:
        b2 = ida_bytes.get_byte(ea + 2)
        sz = 4 if b2 in (0xFE, 0xFF) else 3
        THUNKS[ea] = sz
        ea += sz
    else:
        ea += 1

SEG1_SELECTORS = {S1 >> 4, (S1 >> 4) - 0x1000}
SEG0_SELECTORS = {S0 >> 4, (S0 >> 4) - 0x1000}   # 0x1000 relocated / 0x0000 raw


def is_code(ea):
    return ida_bytes.is_code(ida_bytes.get_full_flags(ea))


def is_undef(ea):
    return ida_bytes.is_unknown(ida_bytes.get_full_flags(ea))


def coverage():
    """bytes that belong to an instruction (head or tail) vs data vs raw."""
    insn = data = undef = 0
    for a in range(S0, S0E):
        f = ida_bytes.get_full_flags(a)
        if ida_bytes.is_code(f) or ida_bytes.is_tail(f) and ida_bytes.is_code(
                ida_bytes.get_full_flags(ida_bytes.get_item_head(a))):
            insn += 1
        elif ida_bytes.is_data(f) or ida_bytes.is_tail(f):
            data += 1
        else:
            undef += 1
    return insn, data, undef


def sweep(lo, hi):
    """Linear-disassemble [lo, hi); return count of insns made. Within a
    known code gap, anything IDA left as data (mis-typed operand bytes) is
    also forced to code."""
    n = 0
    a = lo
    while a < hi:
        if is_code(a):
            a = max(ida_bytes.get_item_end(a), a + 1)
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


def reset_seg000():
    """Undo any prior pass: delete every seg000 function and un-type the
    whole body past the header, back to raw bytes. Keeps this script
    idempotent without a full -B rebuild."""
    for f in list(idautils.Functions(S0, S0E)):
        ida_funcs.del_func(f)
    ida_bytes.del_items(S0 + HEADER_END, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC,
                        S0E - (S0 + HEADER_END))
    ida_auto.auto_wait()


def main():
    c0 = coverage()
    print(f"seg000 {S0:#x}-{S0E:#x} ({S0E - S0} bytes)")
    print(f"before: code={c0[0]} data={c0[1]} undef={c0[2]}")
    if not DRY_RUN and c0[0] > 0:
        reset_seg000()
        cr = coverage()
        print(f"reset:  code={cr[0]} data={cr[1]} undef={cr[2]}")

    callsites = []       # every anchor (both forms)
    proc_targets = set()  # seg000-local far-call targets -> function seeds
    rt_used = set()       # seg001 thunk EAs actually called
    n_rt = n_proc = 0
    for a in range(S0 + HEADER_END, S0E - 4):
        if ida_bytes.get_byte(a) != 0x9A:
            continue
        off = ida_bytes.get_word(a + 1)
        sel = ida_bytes.get_word(a + 3)
        if sel in SEG1_SELECTORS and S1 + off in THUNKS:
            callsites.append(a)
            rt_used.add(S1 + off)
            n_rt += 1
        elif sel in SEG0_SELECTORS and HEADER_END <= off < (S0E - S0):
            callsites.append(a)
            proc_targets.add(S0 + off)
            n_proc += 1
    callsites.sort()
    print(f"far-call sites: {len(callsites)}  ({n_rt} -> seg001 thunks, "
          f"{n_proc} -> seg000 procs / {len(proc_targets)} distinct targets)")

    if DRY_RUN:
        gaps = [callsites[i + 1] - (callsites[i] + 5) for i in range(len(callsites) - 1)]
        big = sorted(((g, callsites[i] + 5) for i, g in enumerate(gaps)), reverse=True)[:10]
        print(f"gap sizes: min={min(gaps)} max={max(gaps)} avg={sum(gaps)/len(gaps):.1f}")
        print("largest gaps (size @ start):  " + ", ".join(f"{g}@{a:#x}" for g, a in big))
        print(f"proc targets: {', '.join(f'{t:#x}' for t in sorted(proc_targets)[:20])}")
        print("[DRY_RUN] stopping before edits")
        return

    # 1. make each called seg001 thunk a tiny returning function, so IDA
    #    flows *past* every `call far` instead of treating it as a dead
    #    end. (A bare `int 3Fh` with a data byte after it otherwise reads
    #    as a no-return stub, which fragments seg000 into ~250 one-line
    #    "functions" and stops disassembly after each call.)
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
    print(f"seg001 thunks made into returning funcs: {fixed}/{len(rt_used)}")

    # 2. anchor the far-call sites themselves
    made = 0
    for a in callsites:
        ida_bytes.del_items(a, ida_bytes.DELIT_SIMPLE, 5)
        if idc.create_insn(a) == 5:
            made += 1
    ida_auto.auto_wait()
    print(f"anchored {made}/{len(callsites)} call-far sites")

    # 2. sweep the gaps between consecutive anchors, + head + tail
    total = sweep(S0 + HEADER_END, callsites[0])
    for i in range(len(callsites) - 1):
        total += sweep(callsites[i] + 5, callsites[i + 1])
    total += sweep(callsites[-1] + 5, S0E)
    ida_auto.auto_wait()
    print(f"gap sweep: +{total} insns")

    # 3. a couple more settle passes over anything still undefined between
    #    the first and last anchor (flow analysis may have exposed more)
    for p in range(3):
        n = sweep(S0 + HEADER_END, callsites[-1] + 5)
        ida_auto.auto_wait()
        cc = coverage()
        print(f"  settle pass {p + 1}: +{n} insns, code={cc[0]} undef={cc[2]}")
        if n == 0:
            break

    # 4. functions. IDA's flow analysis during the sweep carves a mess of
    #    hundreds of one-statement "functions" (compiled BASIC has no
    #    prologues and the run-time calls confuse boundary detection).
    #    Wipe them and re-create only at real entry points:
    #      * program entry (menu_main)
    #      * far-call targets into seg000  = BASIC SUB/FUNCTION procs
    #      * near-call targets within seg000
    #    then one orphan sweep for whatever's left unreachable.
    for f in list(idautils.Functions(S0, S0E)):
        ida_funcs.del_func(f)
    ida_auto.auto_wait()

    entry = next((a for a in range(S0 + HEADER_END, S0 + 0x60) if is_code(a)), None)

    seeds = set(proc_targets)
    if entry is not None:
        seeds.add(entry)
    for a in idautils.Heads(S0, S0E):
        if is_code(a) and idc.print_insn_mnem(a) == "call" \
           and idc.get_operand_type(a, 0) == idc.o_near:
            t = idc.get_operand_value(a, 0)
            if S0 <= t < S0E:
                seeds.add(t)

    n_seed = 0
    for t in sorted(seeds):
        if ida_funcs.add_func(t):
            n_seed += 1
    ida_auto.auto_wait()
    if entry is not None:
        idc.set_name(entry, "menu_main", idc.SN_NOWARN)
    print(f"seed functions (entry + {len(proc_targets)} procs + near-calls): "
          f"{n_seed}/{len(seeds)}")

    swept = 0
    a = S0 + HEADER_END
    while a < S0E:
        if is_code(a) and not ida_funcs.get_func(a):
            if ida_funcs.add_func(a):
                swept += 1
            nf = ida_funcs.get_func(a)
            a = nf.end_ea if nf else a + 1
        else:
            a += 1
    ida_auto.auto_wait()
    print(f"orphan-code functions: {swept}")

    # 5. IDA's `int 3Fh` = "overlay-manager" special-casing makes it treat
    #    every `call far <thunk>` as non-returning, so seg000 comes out as
    #    ~500 one-statement functions. Merge any function that ends in a
    #    `call far` into its successor, UNLESS the successor is a real
    #    SUB/FUNCTION entry (a seg000 far-call target).
    boundaries = set(proc_targets)
    if entry is not None:
        boundaries.add(entry)
    merges = 1
    rounds = 0
    while merges and rounds < 40:
        merges = 0
        rounds += 1
        for f in list(idautils.Functions(S0, S0E)):
            fn = ida_funcs.get_func(f)
            if not fn:
                continue
            last = idc.prev_head(fn.end_ea, fn.start_ea)
            if idc.print_insn_mnem(last) != "call":
                continue
            succ = ida_funcs.get_func(fn.end_ea)
            if not succ or succ.start_ea in boundaries:
                continue
            new_end = succ.end_ea
            ida_funcs.del_func(succ.start_ea)
            ida_funcs.set_func_end(fn.start_ea, new_end)
            merges += 1
        if merges:
            ida_auto.auto_wait()
    print(f"call-far function merges: {rounds} rounds")

    # NB: the cosmetic "add a fall-through cref past every call far" pass
    # (to stop IDA's overlay special-casing chopping a block break after
    # each one) lives in apply_renames_menu.py -- it has to run *after*
    # resolve_thunks_menu.py's auto_wait, which would otherwise wipe the
    # crefs. Pipeline order: resolve_thunks -> coerce -> apply_renames.

    c1 = coverage()
    print(f"\nafter:  code={c1[0]} data={c1[1]} undef={c1[2]}  "
          f"({100.0 * c1[0] / (S0E - S0):.1f}% code)")
    print(f"delta:  code {c1[0] - c0[0]:+d}, undef {c1[2] - c0[2]:+d}")
    print(f"functions in seg000: {sum(1 for _ in idautils.Functions(S0, S0E))}")

    bad = [a for a in idautils.Heads(S0, S0E)
           if is_code(a) and idc.print_insn_mnem(a) == ""]
    undef_runs = []
    a = S0 + HEADER_END
    while a < S0E:
        if is_undef(a):
            st = a
            while a < S0E and is_undef(a):
                a += 1
            undef_runs.append((a - st, st))
        else:
            a += 1
    undef_runs.sort(reverse=True)
    print(f"bad insns: {len(bad)}  {', '.join(f'{x:#x}' for x in bad[:15])}")
    print(f"largest undefined runs left: "
          + ", ".join(f"{n}@{a:#x}" for n, a in undef_runs[:10]))


main()
