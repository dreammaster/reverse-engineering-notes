"""
Read-only: FileEntry_OpenFile reads the filename from bx+0xE (right
after the 14-byte struct). Checks what string sits there for each known
fixed FileEntry instance (0x9043, 0x902C), plus reports the linear
address so we can find which FileEntry (if any) points at
PICTURES.VGA's string (0x3687F).

    .\run_ida_script.ps1 check_fileentry_names.py -NoExport
"""
import idc
import ida_bytes

DS_BASE = 0x2D860

for bx in (0x9043, 0x902C):
    ea = DS_BASE + bx + 0xE
    s = idc.get_strlit_contents(ea)
    print(f"FileEntry bx={bx:#x}: filename ea={ea:#x}  contents={s!r}")

print()
print("PICTURES.VGA string ea = 0x3687F")
print("implied bx (if this were a FileEntry+0xE):", hex(0x3687F - DS_BASE - 0xE))
