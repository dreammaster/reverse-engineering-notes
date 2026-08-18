"""
IDA Pro script: undefines the mis-disassembled garbage in seg002
between the real `start` bootstrap proc (0x18AC0-0x18AE0) and the
confirmed-uninitialized `byte_18FE0` tail, replacing garbled fake x86
instructions with a plain byte array.

Background (2026-08-18): seg002 (0x18AC0-0x19FE0) turned out to start
at the EXE's actual DOS entry point (header: "Entry Point: 18AC:0").
The first 32 bytes are a genuine, clean bootstrap routine (`start`):
computes SS relative to DS (set by the loader), zeroes all registers,
far-jumps into the game's own `start_`. Everything after that (0x18AE0
through 0x18FE0, 1280 bytes) is currently disassembled as nonsense x86
(random FPU instructions, a jump to a nonsensical far pointer) -- not
real code, confirmed by reading it. It's NOT simple zero padding
either (only 45% zero bytes) -- it has real semi-structured content
(a repeating pattern, no readable text) whose actual meaning/format
isn't understood. `byte_18FE0` onward (1000h bytes) is separately
already correctly marked "Segment type: Uninitialized" -- exactly
matching the EXE header's "Loaded length: 8FE0h" (0x10000+0x8FE0 =
0x18FE0), i.e. that tail was never part of the file at all.

This script only fixes the misleading "looks like code" presentation
-- it does NOT claim to explain what the bytes represent. Flagged as
an open question in docs/roadmap.md.

USAGE: structural/graph-surgery script, dry-run first per this
project's convention (same category as fix_inline_strings.py).
"""

import ida_bytes
import ida_funcs
import ida_name
import idc

DRY_RUN = False


def main():
    start_ea = idc.get_name_ea_simple("start")
    uninit_ea = idc.get_name_ea_simple("byte_18FE0")
    if start_ea == idc.BADADDR or uninit_ea == idc.BADADDR:
        print("[!] couldn't resolve start/byte_18FE0 -- aborting")
        return

    pfn = ida_funcs.get_func(start_ea)
    if pfn is None:
        print("[!] no function object at start_ea -- aborting")
        return

    gap_start = pfn.end_ea
    gap_len = uninit_ea - gap_start
    print(f"Gap: {gap_start:X} .. {uninit_ea:X} ({gap_len:#x} = {gap_len} bytes)")

    if DRY_RUN:
        print("[dry] would del_items + create a byte array here, "
              "named seg002_unexplained_data. Set DRY_RUN = False to apply.")
        return

    if not ida_bytes.del_items(gap_start, ida_bytes.DELIT_SIMPLE, gap_len):
        print("[!] del_items reported failure (may be harmless)")
    if not ida_bytes.create_data(gap_start, ida_bytes.FF_BYTE, gap_len, idc.BADADDR):
        print("[!] create_data FAILED")
        return
    if not ida_name.set_name(gap_start, "seg002_unexplained_data", ida_name.SN_NOWARN):
        print("[!] set_name FAILED")
    else:
        print(f"Done: {gap_start:X} now a {gap_len}-byte array, "
              "named seg002_unexplained_data.")
    idc.set_cmt(gap_start,
                "Not code (confirmed) -- structure/meaning not understood. "
                "45% zero bytes, some repeating pattern, no readable text. "
                "See docs/roadmap.md.", 0)


if __name__ == "__main__":
    main()
