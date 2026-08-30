"""
IDA Pro script: name LEGLIB.EXE's DGROUP (runtime-internal) variables.

LEGLIB is the shared BASIC runtime + engine, so its DGROUP holds runtime
internals (the value stack, the screen/output state, graphics pointers,
re-entrancy counters) rather than game state. DGROUP is the zero-filled
area at `seg000` (0x10000); the runtime's `ds` points there and offsets
run up past 0x0E00 (into `seg001`), so the base is `seg000.start_ea`.

`ida_scripts/dsvars.py` (run over seg003..seg008) finds ~740 DGROUP words
touched. This names the ones whose role is clear from the instruction
patterns and the surrounding function cluster; the many single-function
scratch slots and the not-yet-understood `rtm_*` clusters are left.

Rough DGROUP layout that emerges:
  0x0002..0x0140  BASIC runtime control block (dgroupSeg, nestLevel,
                  stopFlag, exit state, I/O channel)
  0x0216..0x0302  interior tile-graphics engine (rtm_60/61, sub_1A2xx)
  0x0346..0x03D4  text/console output + PRINT-USING formatter
  0x0410..0x0527  string heap / string routines
  0x0C62..0x0C64  value-stack work registers
  0x0E46..0x0EFA  screen geometry / mode (mostly read-only)
  0x111C..0x1164  value stack + FF operand cluster
  0x14FA..0x1FEE  the bm* / FE graphics layer (refresh rect, gfx cursor)

Names data addresses + repeatable comments only.

    .\run_ida_script.ps1 -Idb leglib -ScriptName apply_dsvars_leglib.py
"""

import idc
import idautils
import ida_bytes
import ida_segment

DRY_RUN = False

# (offset, name, is_byte, comment)
VARS = [
    # --- BASIC runtime control block ---
    (0x0101, "dgroupSeg", False,
     "the runtime's own DS / DGROUP segment value -- stashed at startup "
     "(sub_12B21 / rtm_12: `mov ds:101h, ds`) and reloaded into ES/DS by "
     "routines that trash the segment regs (rtm_92 / rtm_A0 / rtm_FE20 "
     "/ rtm_02 / rtm_03)."),
    (0x0118, "nestLevel", False,
     "runtime call-nesting / re-entrancy counter -- rtm_63 inc's on "
     "entry and dec's on exit, rtm_FF02 checks `== 1`, basProcLeave "
     "resets it to 0."),
    (0x0136, "stopFlag", True,
     "end-of-program / abort byte (0xFF = stop). basProcExit2 checks "
     "`== 0`; rtm_F2 / rtm_FB / rtm_FF07 test it; sub_1D5F2 sets it."),
    (0x0116, "procFlags", False,
     "proc-frame state word paired with nestLevel (basProcLeave, "
     "rtm_16, rtm_FF02). TENTATIVE."),
    (0x0738, "savedStackTop", False,
     "saved stack pointer / frame top for the proc unwind (basProcLeave "
     "/ rtm_16 / rtm_EF read it). TENTATIVE."),
    (0x011E, "ioChannel", True,
     "current I/O channel / mode byte (rtm_5A sets 8, sub_19A5E checks "
     "3, sub_1A176 sets 2). TENTATIVE."),
    (0x0874, "chainCmdPtr", False,
     "startup pointer stashed alongside dgroupSeg (sub_12B21: `mov "
     "ds:874h, di`); rtm_FF08 reads it and sub_172FE pushes it for the "
     "chain/exec -- looks like the command-line / environment pointer. "
     "TENTATIVE."),

    # --- value stack ---
    (0x111C, "valueStackPtr", False,
     "the BASIC value / expression stack pointer. The FF-cluster "
     "routines advance/retreat it via BX/DI; rtm_FF4A / rtm_FF4B are "
     "the push / pop. (Init value in the DB: 0xFAC.)"),
    (0x0C62, "vsWorkA", False,
     "value-stack work register -- the primary CX spill for the FF "
     "arithmetic / string cluster (sub_2072E..sub_20948). TENTATIVE."),
    (0x0C64, "vsWorkB", False,
     "second value-stack work register / accumulator (`add cx, "
     "ds:0C64h`). TENTATIVE."),

    # --- screen / console output ---
    (0x0EFA, "screenFlags", True,
     "display / console output status bits -- the seg003 text-output "
     "cluster tests and sets individual bits (0x01 / 0x03 / 0x06 / "
     "0x08 / 0x20 / 0x40 / 0x80)."),
    (0x0E68, "screenCols", True,
     "screen width in columns -- the text-output routines compare the "
     "output column / char in AL against it. TENTATIVE."),
    (0x0E6B, "screenRows", True,
     "screen height in rows (paired with screenCols in the same "
     "output routines). TENTATIVE."),
    (0x03BD, "keyModifiers", True,
     "keyboard shift / ctrl / alt state bits (sub_15018 / sub_15F9F "
     "test 0x04 / 0x08 / 0x10 / 0x20). TENTATIVE."),

    # --- the bm* / FE graphics layer ---
    (0x0876, "videoSegment", False,
     "the graphics framebuffer segment. Every bm* blitter (rtm_FE2A / "
     "FE2D / FE3A / FE3B / FE46 / FE47 / FE5E / FE5F / ...) does "
     "`mov es, ds:876h` (or `mov ds, ...`) before touching pixels. "
     "(Init value in the DB: 0xB800.)"),
    (0x1E86, "dirtyRectA", False,
     "one corner of the pending screen-refresh rectangle -- rtm_FE42 / "
     "rtm_FE43 mark it, screenRefresh flushes it."),
    (0x1E88, "dirtyRectB", False, "the other corner of the refresh rect (see dirtyRectA)."),
    (0x1FEC, "pagerLineCount", False,
     "line counter for the rtm_FE54 \"press a key to continue\" pager -- "
     "inc'd per line, compared against the page height."),
    (0x1FEA, "textPageFlag", False,
     "text-page mode flag for rtm_FE53 / rtm_FE00 (0 or 0x14). "
     "TENTATIVE."),
    (0x02C4, "viewOriginX", False,
     "interior-graphics view origin X -- the tile routines compute "
     "`screenX = worldX - ds:2C4h` (sub_1A3E9 etc). TENTATIVE."),
    (0x02C8, "viewOriginY", False, "interior-graphics view origin Y (paired with viewOriginX). TENTATIVE."),
    (0x0250, "interiorDrawX", False,
     "current interior-graphics draw column -- rtm_60 / rtm_61 and the "
     "sub_1A2xx..sub_1C0xx cluster xchg / pop it around draw ops. "
     "TENTATIVE."),
    (0x0252, "interiorDrawY", False, "current interior-graphics draw row (paired with interiorDrawX). TENTATIVE."),
    (0x15FE, "interiorViewBase", False,
     "interior-view scroll offset / base pointer -- the rtm_FE1x "
     "routines add to it (`add bx, ds:15FEh`). TENTATIVE."),

    # --- misc (kept from the first pass) ---
    (0x0137, "fmtBufPos", False,
     "a running buffer / parse position in the string-format cluster "
     "(sub_149xx..sub_153xx) -- cleared to 0, used as SI, `add "
     "es:137h, ax`. TENTATIVE."),
    (0x0E46, "textAttr", True,
     "current text attribute / colour byte (rtm_FF66 / sub_142B3 / "
     "sub_14334). TENTATIVE."),
]


def name_data(ea, name, is_byte, cmt):
    n = 1 if is_byte else 2
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, n)
    ida_bytes.create_data(ea, ida_bytes.FF_BYTE if is_byte else ida_bytes.FF_WORD,
                          n, idc.BADADDR)
    if idc.set_name(ea, name, idc.SN_NOWARN | idc.SN_CHECK):
        if cmt:
            idc.set_cmt(ea, cmt, 1)
        return True
    return False


def main():
    base = ida_segment.get_segm_by_name("seg000").start_ea
    done = skip = 0
    seen = set()
    for off, name, is_byte, cmt in VARS:
        if off in seen:
            continue
        seen.add(off)
        ea = base + off
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  ds:{off:04X}: {cur!r} -> {name!r}")
            continue
        if cur == name:
            skip += 1
            continue
        if name_data(ea, name, is_byte, cmt or None):
            done += 1
        else:
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
    print(f"\nnamed {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))


main()
