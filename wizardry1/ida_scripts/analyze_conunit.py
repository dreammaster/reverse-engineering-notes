"""
Disassemble the CONUNIT (unit 13) console driver inside SYSTEM.INTERP.

`RANDOM` and the graphics I/O (`*.CHARSET` / `*.MONSTERS` / `*.TITLE` load,
the offscreen blits, the 3D-view scanline pokes) all funnel through
`UNITREAD/UNITWRITE(unit 13, buf, len, blocknumber, ...)`.  The generic CSP
handlers `loc_2CCF` (URD) / `loc_2DB9` (UWR) dispatch on the *unit number* to
per-unit natives via tables at 0x2CF7 / 0x2DDE; unit 13 -> 0x144B (URD) /
0x145B (UWR).  Those re-dispatch on the *blocknumber* via tables at 0x134E
(URD) / 0x1392 (UWR).  Key blocknumbers:
    URD 10 -> 0x221E   RANDOM
    URD 11 -> 0x22CD   URD 12 -> 0x22FD   URD 16 -> 0x23E4
    UWR 14 -> 0x146B   load *.CHARSET
    UWR 18 -> 0x1496   load *.MONSTERS      <-- the compressed-portrait path
    UWR  3 -> 0x1577   present / flip
    UWR 13 -> 0x1958   UWR 17 -> 0x1950   blit offscreen -> WINDOW1
    UWR 19 -> 0x2305   scanline poke
    UWR  1 -> 0x1AE6   UWR  2 -> 0x153E   UWR  4 -> 0x14C4
    UWR  5 -> 0x18C1   UWR 15 -> 0x1B59

Run: ida_scripts\\run_ida_script.ps1 -Idb wiz1_interp -ScriptName ida_scripts\\analyze_conunit.py
"""
import ida_bytes
import ida_auto
import ida_funcs
import ida_name
import idc

CODE_LO, CODE_HI = 0x144B, 0x1B80          # the unit-13 driver body (data below 0x144B)

# blocknumber dispatch tables (word arrays; keep them OUT of the code sweep)
URD_TBL = (0x134E, 34)                      # 0x134E .. 0x1391
UWR_TBL = (0x1392, 36)                      # 0x1392 .. 0x13D7

HANDLERS = [
    0x144B, 0x145B,                         # unit-13 URD / UWR entries
    0x146B, 0x1496, 0x14A2, 0x14C4, 0x153E, 0x1577,
    0x18C1, 0x1950, 0x1958, 0x1AE6, 0x1B59,
    0x221E, 0x22CD, 0x22FD, 0x2305, 0x23E4, 0x2215,
]

NAMES = {
    0x144B: "conunit_urd",
    0x145B: "conunit_uwr",
    0x146B: "conunit_load_charset",
    0x1496: "conunit_load_monsters",
    0x1577: "conunit_present",
    0x1950: "conunit_blit17",
    0x1958: "conunit_blit13",
    0x2305: "conunit_scanline19",
    0x221E: "conunit_random",
    0x2215: "conunit_noop",
}


def wtable(base, n):
    ida_bytes.del_items(base, ida_bytes.DELIT_SIMPLE, n * 2)
    for i in range(n):
        idc.create_word(base + 2 * i)


def main():
    # 1. tables as words
    wtable(*URD_TBL)
    wtable(*UWR_TBL)

    # 2. wipe + re-disassemble the driver body
    ida_bytes.del_items(CODE_LO, ida_bytes.DELIT_SIMPLE, CODE_HI - CODE_LO)
    for ea in HANDLERS:
        ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 1)
        idc.create_insn(ea)
    ida_auto.plan_and_wait(CODE_LO, CODE_HI)
    ida_auto.plan_and_wait(0x2200, 0x2400)

    for ea in HANDLERS:
        if idc.is_code(idc.get_full_flags(ea)):
            ida_funcs.add_func(ea)
    for ea, nm in NAMES.items():
        ida_name.set_name(ea, nm, ida_name.SN_FORCE | ida_name.SN_NOWARN)

    # 3. dump
    def dump(lo, hi, title):
        print(f"\n===== {title}  {lo:#06x}..{hi:#06x} =====")
        ea = lo
        while ea < hi:
            f = idc.get_full_flags(ea)
            if idc.is_code(f):
                print(f"{ea:04X}  {idc.GetDisasm(ea)}")
            else:
                print(f"{ea:04X}  db {' '.join(f'{b:02x}' for b in idc.get_bytes(ea, min(8, hi-ea)) or b'')}")
            nxt = idc.next_head(ea, hi)
            ea = nxt if nxt > ea else ea + 1

    dump(0x144B, 0x1600, "unit-13 driver: entries + charset/monsters load")
    dump(0x1600, 0x1B80, "unit-13 driver: blits / present / scanline")
    dump(0x2200, 0x2410, "unit-13 URD block handlers (10=RANDOM, 11, 12, 16)")


main()
