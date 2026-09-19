"""
Read-only: prints the WORLD.DAT offset and record size of each item-catalog
table from Chapter 3's record-setup stubs (the counterpart of what
yendor2/ida_scripts/extract_resource_stubs.py reports for Chapter 2).

The two item-table stubs (WorldDat_setBlock1/2) and the four secondary-table
stubs (PrepareWorldDat1Block1-4Read) each load a 32-bit base offset from a
static table via `mov si, imm`. This database has no ds segment register, so
the data segment base is seg133's paragraph-aligned start.

    .\run_ida_script.ps1 dump_catalog_layout.py -NoExport
"""
import idc
import ida_bytes
import ida_funcs
import ida_segment

DS_BASE = ida_segment.get_segm_by_name("seg133").start_ea & ~0xF

STUBS = [
    "WorldDat_setBlock1",
    "WorldDat_setBlock2",
    "PrepareWorldDat1Block1Read",
    "PrepareWorldDat1Block2Read",
    "PrepareWorldDat1Block3Read",
    "PrepareWorldDat1Block4Read",
]

for name in STUBS:
    func = ida_funcs.get_func(idc.get_name_ea_simple(name))
    si = size = None
    cur = func.start_ea
    while cur < func.end_ea:
        mnemonic = idc.print_insn_mnem(cur)
        if mnemonic == "mov" and idc.print_operand(cur, 0) == "si":
            si = idc.get_operand_value(cur, 1) & 0xFFFF
        if mnemonic == "mov" and "+6" in idc.print_operand(cur, 0):
            size = idc.get_operand_value(cur, 1)
        cur = idc.next_head(cur, func.end_ea)
    offset = ida_bytes.get_wide_dword(DS_BASE + si)
    print("%s: si=%#x offset=%#x (%d) recsize=%s" % (name, si, offset, offset, size))
