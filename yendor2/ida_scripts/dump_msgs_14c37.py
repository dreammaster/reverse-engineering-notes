"""
Dump the two message strings referenced by sub_14C37 (bx=0x8EF7, 0x8EFE),
plus surrounding context bytes since the direct null-terminated read looked
truncated/mid-string. Read-only.

Run via:
    .\\run_ida_script.ps1 dump_msgs_14c37.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=200):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

for off in (0x8EF7, 0x8EFE):
    ea = off + 0x2D860
    print(f"{off:#x} -> {ea:#x}: {read_str(ea)!r}")
    ctx = bytes(ida_bytes.get_byte(ea - 20 + i) for i in range(60))
    print(f"  context [-20..+40]: {ctx!r}")
