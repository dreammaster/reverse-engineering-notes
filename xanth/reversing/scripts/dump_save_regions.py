"""
IDAPython: dumps the savegame region table of Companions of Xanth (the global variables persisted by serialize_state) to JSON.

Table: save_region_count (word_85336) entries of 6 bytes at (segment word seg_8AA38)*16 + 0x116 = {u16 offset; u16 segment; u16 size}.

    idat.exe -A -S"dump_save_regions.py out.json" copy.idb
"""
import json
import idc
import ida_kernwin

out = idc.ARGV[1] if len(idc.ARGV) > 1 else "save_regions.json"
w = idc.get_wide_word
ea = idc.get_name_ea_simple
n = w(ea("save_region_count")) if ea("save_region_count") != idc.BADADDR else w(ea("word_85336"))
base = w(ea("seg_8AA38") if ea("seg_8AA38") != idc.BADADDR else ea("save_regions_seg")) * 16 + 0x116
regs = []
for i in range(n):
    e = base + 6 * i
    addr = w(e + 2) * 16 + w(e)
    regs.append(dict(index=i, addr="%x" % addr, size=w(e + 4), name=idc.get_name(addr)))
json.dump(regs, open(out, "w"))
print("regions", n)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
