"""
IDA Pro script: name what can be identified of LEGLIB.EXE's DGROUP
(runtime-internal) variables.

LEGLIB is the shared BASIC runtime + engine, not a compiled-BASIC client,
so its DGROUP holds runtime internals (the value stack, the screen/output
state, graphics pointers, re-entrancy counters) rather than game state.
DGROUP is the zero-filled area at `seg000` (0x10000); the runtime's `ds`
points there and offsets run up past 0x0E00 (into `seg001`), so the base
is `seg000.start_ea` and there is no useful upper-segment bound.

`ida_scripts/dsvars.py` (run over seg003 + seg004) finds ~460 DGROUP
words touched. Most belong to still-unnamed `rtm_*` / `sub_` clusters and
can't be pinned yet. The entries below are the ones whose role is clear
from the instruction patterns; the rest are left for when the
surrounding routines get real `B$…` names.

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
    (0x0876, "videoSegment", False,
     "the graphics framebuffer segment. Every bm* blitter (rtm_FE2A / "
     "FE2D / FE3A / FE3B / FE46 / FE47 / FE5E / FE5F / ...) does "
     "`mov es, ds:876h` (or `mov ds, ...`) before touching pixels."),
    (0x111C, "valueStackPtr", False,
     "the BASIC value / expression stack pointer. The FF-cluster "
     "routines advance/retreat it via BX/DI; rtm_FF4A / rtm_FF4B are "
     "the push / pop."),
    (0x0EFA, "screenFlags", True,
     "display / console output status bits -- the seg003 text-output "
     "cluster tests and sets individual bits (0x01 / 0x03 / 0x06 / "
     "0x08 / 0x20 / 0x40 / 0x80)."),
    (0x0118, "nestLevel", False,
     "runtime call-nesting / re-entrancy counter -- rtm_63 inc's on "
     "entry and dec's on exit, rtm_FF02 checks `== 1`, basProcLeave "
     "resets it to 0."),

    # --- tentative ---
    (0x0C62, "vsScratchA", False,
     "value-stack scratch cell (holds CX across a sub-operation in "
     "sub_2072E / sub_2075B / sub_20788). TENTATIVE."),
    (0x0C64, "vsScratchB", False,
     "value-stack accumulator (sub_207BB / sub_20840 -- `add cx, "
     "ds:0C64h`). TENTATIVE."),
    (0x0E68, "textAttr", True,
     "current text attribute / colour byte, compared against the "
     "character in AL by the text-output routines. TENTATIVE."),
    (0x011E, "ioChannel", True,
     "current I/O channel / mode byte (rtm_5A sets 8, sub_19A5E checks "
     "3, sub_1A176 sets 2). TENTATIVE."),
    (0x0250, "gfxTempA", False,
     "interior-graphics swap cell -- rtm_60 / rtm_61 and the "
     "sub_1A2xx..sub_1C0xx cluster `xchg` / `pop` it around draw ops. "
     "TENTATIVE."),
    (0x0252, "gfxTempB", False, "paired with gfxTempA (0250). TENTATIVE."),
    (0x0137, "fmtBufPos", False,
     "a running buffer / parse position in the string-format cluster "
     "(sub_149xx..sub_153xx) -- cleared to 0, used as SI, `add "
     "es:137h, ax`. TENTATIVE."),
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
    for off, name, is_byte, cmt in VARS:
        ea = base + off
        cur = idc.get_name(ea)
        if DRY_RUN:
            print(f"  ds:{off:04X}: {cur!r} -> {name!r}")
            continue
        if cur == name:
            skip += 1
            continue
        if name_data(ea, name, is_byte, cmt):
            done += 1
        else:
            print(f"  [!] set_name failed at {ea:#x} -> {name}")
    print(f"\nnamed {done}, already-named {skip}" + ("  [DRY_RUN]" if DRY_RUN else ""))


main()
