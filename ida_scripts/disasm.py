"""
Read-only helper: disassembles address ranges listed in disasm_args.txt ("<start hex> <end hex>" per line, linear
addresses) into disasm_out.txt, turning undefined bytes into instructions in memory only (the database is not saved).
Run with -NoExport.
"""
import os
import idc
import ida_bytes
import ida_ua
import idautils

here = os.path.dirname(os.path.abspath(__file__))
out = open(os.path.join(here, "disasm_out.txt"), "w")
for line in open(os.path.join(here, "disasm_args.txt")):
    parts = line.split()
    if len(parts) < 2:
        continue
    start, end = int(parts[0], 16), int(parts[1], 16)
    ea = start
    while ea < end:
        if not ida_bytes.is_code(ida_bytes.get_flags(ea)):
            ida_bytes.del_items(ea, 0, 1)
            ida_ua.create_insn(ea)
        text = idc.GetDisasm(ea)
        out.write("%05X  %s\n" % (ea, text))
        size = idc.get_item_size(ea)
        ea += size if size > 0 else 1
out.close()
