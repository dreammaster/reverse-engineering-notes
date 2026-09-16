"""
Read-only: dumps raw g_pictureDir entries 8-25 (regardless of the
plausibility filter enumerate_pictures.py used) since sub_1EA6E's code
references word_2E530=0xA (10) / word_2E532=0x80 together for one
DrawPicture call, and those two don't obviously agree under the
"word_2E532 = id*0x10" assumption from before (0x80/0x10=8, not 10) --
checking the raw bytes directly to resolve which assumption is wrong.

    .\run_ida_script.ps1 dump_more_pics.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
TABLE = DS_BASE + 0x782E

for i in range(6, 26):
    ea = TABLE + i * 0x10
    raw = ida_bytes.get_bytes(ea, 0x10)
    w = ida_bytes.get_word(ea + 8)
    h = ida_bytes.get_word(ea + 0xA)
    off = ida_bytes.get_word(ea + 0xC) | (ida_bytes.get_word(ea + 0xE) << 16)
    hexs = " ".join(f"{b:02x}" for b in raw)
    print(f"#{i:3d}  ea={ea:#x}  {hexs}  w={w} h={h} off={off:#x}")
