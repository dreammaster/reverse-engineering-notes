"""
leglib.idb structural script: resolve every int-3Fh run-time entry point
that client modules (menu/out/dun/...) call, turn each target into a
named function, and dump the (prefix,ordinal) -> address map to
ida_scripts/rtm_map.py for the client-side thunk scripts to consume.

HOW THE DISPATCH WORKS (reverse-engineered 2026-08-30)
-----------------------------------------------------
Compiled-BASIC client modules call the shared run-time (this file,
LEGLIB.EXE) through a thunk segment full of 3-/4-byte trampolines:

    CD 3F nn        -> bare ordinal nn         (nn = 0x00..0xFD)
    CD 3F FF nn     -> FF-prefixed ordinal nn  (nn = 0x00..0x67)
    CD 3F FE nn     -> FE-prefixed ordinal nn  (nn = 0x00..0x6D)

The int-3Fh handler LEGLIB installs (seg003:7383h, set via DOS
int 21h/AX=253Fh from ~seg003:734Bh) does, on first hit of each call
site:
  * bare nn : target = seg003 : word[seg003:73F6h + 2*nn]
  * FF   nn : target = base   : word[seg003:75F2h + 2*nn]
              base = seg004 when 0x19 <= nn < 0x62, else seg003
  * FE   nn : target = seg:off from the 4-byte entry at
              seg003:15Ch + 4*nn   (a real, already-relocated far ptr;
              lands in seg004 / seg007 / seg008 -- the bitmap/graphics
              segments)
  * then it rewrites the *caller's* `CALL FAR` operands in place and
    retf's, so every call site self-patches to a direct far call after
    its first execution. (Nothing in LEGLIB's own image is patched.)

seg003's paragraph in IDA is 0x12A9 (= file value 0x2A9 + image base
0x1000); seg004 is 0x1F9C. The bare/FF tables store offsets only; the
handler supplies the segment as the constant 0x2A9 / 0xF9C (file
convention), i.e. seg003 / seg004.

    .\run_ida_script.ps1 -Idb leglib -ScriptName resolve_rtm_leglib.py
"""

import os
import idc
import ida_bytes
import ida_segment
import ida_funcs
import ida_auto

DRY_RUN = False

MAP_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "rtm_map.py")

seg3 = ida_segment.get_segm_by_name("seg003")
seg4 = ida_segment.get_segm_by_name("seg004")
S3 = seg3.start_ea
S4 = seg4.start_ea

BARE_TAB = S3 + 0x73F6
FF_TAB = S3 + 0x75F2
FE_TAB = S3 + 0x15C

BARE_N = 0xFE     # ordinals 0x00..0xFD
FF_N = 0x68       # 0x00..0x67
FE_N = 0x6E       # 0x00..0x6D


def w(ea):
    return ida_bytes.get_word(ea)


def seg_of(ea):
    s = ida_segment.getseg(ea)
    return ida_segment.get_segm_name(s) if s else "?"


def key_str(prefix, ordinal):
    return f"{ordinal:02X}" if prefix is None else f"{prefix:02X}{ordinal:02X}"


def resolve():
    out = []  # (prefix, ordinal, target_ea)
    for n in range(BARE_N):
        out.append((None, n, S3 + w(BARE_TAB + 2 * n)))
    for n in range(FF_N):
        base = S4 if 0x19 <= n < 0x62 else S3
        out.append((0xFF, n, base + w(FF_TAB + 2 * n)))
    for n in range(FE_N):
        off = w(FE_TAB + 4 * n)
        para = w(FE_TAB + 4 * n + 2)          # already relocated by IDA loader
        out.append((0xFE, n, (para << 4) + off))
    return out


def main():
    entries = resolve()

    made_code = made_func = named = 0
    stats = {"FUNC_HEAD": 0, "mid-func": 0, "code": 0, "made": 0, "bad": 0}
    rows = []

    for prefix, ordinal, ea in entries:
        seg = ida_segment.getseg(ea)
        if seg is None:
            stats["bad"] += 1
            rows.append((prefix, ordinal, ea, "NO-SEGMENT"))
            continue

        fn = ida_funcs.get_func(ea)
        if fn and fn.start_ea == ea:
            state = "FUNC_HEAD"
        elif fn:
            state = "mid-func"
        else:
            flags = ida_bytes.get_full_flags(ea)
            if ida_bytes.is_code(flags):
                state = "code"
            elif not ida_bytes.is_data(flags):
                state = "undef"
            else:
                state = "data"

        if not DRY_RUN and state in ("code", "undef"):
            if state == "undef":
                ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 8)
                if idc.create_insn(ea):
                    made_code += 1
            if ida_funcs.add_func(ea):
                made_func += 1
                state = "made"

        want = "rtm_" + key_str(prefix, ordinal)
        cur = idc.get_name(ea)
        if not DRY_RUN and state != "data":
            # set_name works at any ea (renames the func at a head, else a
            # plain label); only overwrite auto-generated names.
            if not cur or cur.startswith(("sub_", "loc_", "unk_", "byte_", "word_", "off_", "dword_")):
                if idc.set_name(ea, want, idc.SN_NOWARN):
                    named += 1
            idc.set_cmt(ea, f"int 3Fh run-time entry {key_str(prefix, ordinal)}"
                        + ("  [mid-func: verify]" if state == "mid-func" else ""), 1)

        stats[state] = stats.get(state, 0) + 1
        rows.append((prefix, ordinal, ea, state))

    if not DRY_RUN:
        ida_auto.auto_wait()

    # console summary
    print(f"entries: {len(entries)}  (bare {BARE_N}, FF {FF_N}, FE {FE_N})")
    print(f"target state: {stats}")
    print(f"{'made code' if not DRY_RUN else 'would make code'}: "
          f"{made_code}, functions: {made_func}, names: {named}"
          + ("   [DRY_RUN]" if DRY_RUN else ""))

    print("\nhot ordinals (menu.idb call counts, for reference): "
          "C2=171 D1=149 FE26=98 FE3F=29 AF=26 FE45=24 F0=21 F4=21")
    for prefix, ordinal, ea in [(None, 0xC2, 0), (None, 0xD1, 0), (0xFE, 0x26, 0),
                                (None, 0xAF, 0), (None, 0xF4, 0)]:
        pass

    # write the map module (always -- it's derived, not an edit to the idb)
    with open(MAP_PATH, "w") as fh:
        fh.write('"""\n')
        fh.write("Auto-generated by ida_scripts/resolve_rtm_leglib.py -- do not edit by hand.\n\n")
        fh.write("(prefix, ordinal) -> dict(leglib linear ea, leglib segment, name).\n")
        fh.write("prefix is None for a bare `CD 3F nn` thunk, else 0xFE / 0xFF.\n")
        fh.write("Used by the client-side thunk scripts (resolve_thunks_<module>.py)\n")
        fh.write("to label each client thunk with the run-time routine it reaches.\n")
        fh.write('"""\n\n')
        fh.write("RTM_MAP = {\n")
        for prefix, ordinal, ea, state in rows:
            nm = "rtm_" + key_str(prefix, ordinal)
            real = idc.get_name(ea)
            if real and not real.startswith(("sub_", "loc_", "unk_")):
                nm = real
            fh.write(f"    ({prefix!r}, {ordinal:#04x}): "
                     f"dict(ea={ea:#07x}, seg={seg_of(ea)!r}, name={nm!r}, state={state!r}),\n")
        fh.write("}\n")
    print(f"\nwrote {MAP_PATH}")


main()
