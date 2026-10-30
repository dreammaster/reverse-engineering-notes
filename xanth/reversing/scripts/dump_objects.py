"""
IDAPython: dumps the Companions of Xanth object table (the initial state of every object) from the IDB to JSON.

Object table: 6-byte entries at (segment word seg_8A9E4)*16 + 0x66 + id*6 = {u8 type (1..5); u8 pad; u16 recordOffset; u16 recordSegment}, ids 1..object_count.
Record sizes by type 1..5 = 18, 28, 32, 14, 32 bytes (the spacing of the records in the data segment; every accessor offset fits). Field offsets by type: analysis/xanth_overview.md section 8.

    idat.exe -A -S"dump_objects.py out.json" copy.idb
"""
import json
import idc
import ida_kernwin

out = idc.ARGV[1] if len(idc.ARGV) > 1 else "objects.json"
w, b = idc.get_wide_word, idc.get_wide_byte
ea = idc.get_name_ea_simple
count = w(ea("object_count"))
tab = w(ea("seg_8A9E4")) * 16 + 0x66
SIZES = {1: 18, 2: 28, 3: 32, 4: 14, 5: 32}
objs = []
for i in range(1, count + 1):
    e = tab + i * 6
    t = b(e)
    rec = w(e + 4) * 16 + w(e + 2)
    size = SIZES[t]
    objs.append(dict(id=i, type=t, rec="%x" % rec, size=size, data=[b(rec + k) for k in range(size)]))
json.dump(dict(count=count, objects=objs), open(out, "w"))
print("objects", count)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
