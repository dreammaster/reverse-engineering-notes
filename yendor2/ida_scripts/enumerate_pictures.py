"""
Read-only: scans g_pictureDir for plausible entries (width/height in a
sane range, file offset within PICTURES.VGA's size) to build a fuller
picture catalog, and finds the table's end (first run of clearly-invalid
entries) to bound its size.

    .\run_ida_script.ps1 enumerate_pictures.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
TABLE = DS_BASE + 0x782E  # == g_pictureDir, 0x3508E
VGA_SIZE = 12550618
MAX_ENTRIES = 2000

entries = []
bad_streak = 0
for i in range(MAX_ENTRIES):
    ea = TABLE + i * 0x10
    w = ida_bytes.get_word(ea + 8)
    h = ida_bytes.get_word(ea + 0xA)
    off = ida_bytes.get_word(ea + 0xC) | (ida_bytes.get_word(ea + 0xE) << 16)
    plausible = (1 <= w <= 640) and (1 <= h <= 480) and (off + w * h <= VGA_SIZE)
    if plausible:
        bad_streak = 0
        entries.append((i, ea, w, h, off))
    else:
        bad_streak += 1
        if bad_streak > 20 and entries:
            print(f"stopping scan at index {i} after {bad_streak} implausible entries in a row")
            break

print(f"\n{len(entries)} plausible entries found (of {i+1} scanned)\n")
for idx, ea, w, h, off in entries:
    print(f"  #{idx:3d}  ea={ea:#x}  {w:4d}x{h:<4d}  offset={off:#x}  end={off+w*h:#x}")
