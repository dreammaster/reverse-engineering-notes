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
# attribute descriptors: 4 bytes per attribute id at 0x33A1C + 4*id = {u8 bitOffset; u8 pad; u16 typeMask}; the bit lives at record + 4 + bitOffset/8, bit bitOffset%8
attr = [dict(id=a, bit=b(0x33A1C + 4 * a), mask=w(0x33A1E + 4 * a)) for a in range(1, 25)]
json.dump(dict(count=count, objects=objs, attrs=attr), open(out, "w"))
print("objects", count)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
