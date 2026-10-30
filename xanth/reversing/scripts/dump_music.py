"""
IDAPython: dumps the room -> music table of Companions of Xanth (set_room_music, Xanth 0x1FA9x): 0x42 entries of 6 bytes at unk_33880 = {u16 room; u16 musicId; u8 volume; u8 loop}.
musicId = (XANTH_nn.MUS file number << 8) | tune number (1-based; tune t = directory records 2t-2 / 2t-1, see xanth_mus.py).  Rooms 0x22, 0x40 and 0x51 are special-cased:
0x307 at volume 0x7F (looped), and the fallback is 0x302 / 0x307 depending on the sword flag.

    idat.exe -A -S"dump_music.py" copy.idb      (writes music.json next to the database)
"""
import idc

w, b = idc.get_wide_word, idc.get_wide_byte
a = idc.get_name_ea_simple("unk_33880")
rows = []
for i in range(0x42):
    e = a + 6 * i
    rows.append(dict(room=w(e), music=w(e + 2), vol=b(e + 4), loop=b(e + 5)))
import json
json.dump(rows, open("music.json", "w"))
idc.qexit(0)
