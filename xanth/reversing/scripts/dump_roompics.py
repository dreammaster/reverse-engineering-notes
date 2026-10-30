"""
IDAPython: dumps the room -> picture table of Companions of Xanth (sub_1DB4E, called by describe_room): 68 entries of 4 bytes at seg044:0x7F8 = {u16 room; u16 picNum},
picNum = (XANTH_nn.PIC file number << 8) | entry.  Room 0x1F0 is special-cased in code (0x123E / 0x123F depending on whether object 0x8B is held).

    idat.exe -A -S"dump_roompics.py" copy.idb      (writes roompics.json next to the database)
"""
import idc
import json

w = idc.get_wide_word
base = w(idc.get_name_ea_simple("seg_8AA04")) * 16 + 0x7F8
rows = [dict(room=w(base + 4 * i), pic=w(base + 4 * i + 2)) for i in range((0x908 - 0x7F8) // 4)]
json.dump(rows, open("roompics.json", "w"))
idc.qexit(0)
