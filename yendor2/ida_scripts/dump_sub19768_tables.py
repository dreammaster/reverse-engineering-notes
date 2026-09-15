"""
Dumps the 3 string tables referenced by sub_19768 (stride 0xB=11 bytes),
to figure out what [si+0xE] actually selects (class name? location
category? profession?).

Run via:
    .\\run_ida_script.ps1 dump_sub19768_tables.py -NoExport
"""
import ida_bytes

def dump_table(base, count, stride=0xB):
    print(f"--- table at {base:#x} ---")
    for i in range(count):
        ea = base + i * stride
        chars = []
        for j in range(stride):
            b = ida_bytes.get_byte(ea + j)
            if b == 0:
                break
            chars.append(chr(b) if 32 <= b < 127 else f"\\x{b:02x}")
        print(f"  [{i}] {ea:#x}: {''.join(chars)!r}")

dump_table(0x351E2 - 0xB, 14)
