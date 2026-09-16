"""
Read-only: dumps raw bytes of the candidate palette table at DS-relative
0x475A (linear 0x31FBA), the target buffer FadePaletteStep/its
duplicate use for palette interpolation, to check whether it looks like
sensible 6-bit-per-channel VGA DAC RGB triples (each byte 0-63) and find
its per-picture stride.

    .\run_ida_script.ps1 dump_palette_table.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
TABLE = DS_BASE + 0x475A

data = ida_bytes.get_bytes(TABLE, 768 + 32)
max_byte = max(data)
print(f"max byte value in first {len(data)} bytes: {max_byte:#x} ({max_byte})")
for row in range(0, 96, 16):
    chunk = data[row:row+16]
    hexs = " ".join(f"{b:02x}" for b in chunk)
    print(f"{TABLE+row:#x}  {hexs}")

print("\nword_322BA name/value check:")
import idc
print("word_322BA ea:", hex(idc.get_name_ea_simple("word_322BA")))
