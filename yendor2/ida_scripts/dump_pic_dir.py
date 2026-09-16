"""
Read-only: dumps raw bytes of the candidate picture-directory table at
DS-relative 0x782E (seen used as `bx=0x782E+word_2E532` then reading
[bx+4]/[bx+0xC]/[bx+0xE] before a PICTURES.VGA FileEntry_Read in
sub_23874), to work out its per-entry layout.

    .\run_ida_script.ps1 dump_pic_dir.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
TABLE = DS_BASE + 0x782E

data = ida_bytes.get_bytes(TABLE, 0x200)
for row in range(0, len(data), 16):
    chunk = data[row:row+16]
    hexs = " ".join(f"{b:02x}" for b in chunk)
    print(f"{TABLE+row:#x}  {hexs}")
