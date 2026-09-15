"""
Dump msg 0x795D, the shared prompt for sub_20523/sub_20570 (wall/floor
type-number entry fields in RunMapEditorScreen).

Run via:
    .\\run_ida_script.ps1 dump_msg_795d.py -NoExport
"""
import ida_bytes

def read_str(ea, maxlen=40):
    out = bytearray()
    for i in range(maxlen):
        b = ida_bytes.get_byte(ea + i)
        if b == 0:
            break
        out.append(b)
    return bytes(out)

ea = 0x795D + 0x2D860
print(f"0x795d -> {ea:#x}: {read_str(ea)!r}")
