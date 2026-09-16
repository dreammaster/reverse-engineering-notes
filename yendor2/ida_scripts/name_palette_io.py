"""
Names the direct-port-I/O VGA DAC palette read/write pair, found while
looking for how PICTURES.VGA's true colors get set up (extracted images
so far are grayscale index visualizations only):

- sub_25A3B: waits for vertical retrace (port 0x3DAh bit 3) if a flag is
  set, then writes bl to port 0x3C8 (DAC write-address register) and cx
  RGB triples (from ds:si) to port 0x3C9 (DAC data register). This is
  the real palette-set routine (bypassing the BIOS int10h/ax=1012h
  entirely, straight to the VGA DAC). -> SetPaletteRange
- sub_25A5B: BIOS int10h/ax=1017h (read block of DAC registers) reading
  all 256 into es:dx. -> GetPalette

Run via:
    .\run_ida_script.ps1 name_palette_io.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x25A3B: "SetPaletteRange",
    0x25A5B: "GetPalette",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x25A3B,
    "Direct VGA DAC I/O (bypasses BIOS): optionally waits for vertical "
    "retrace (port 0x3DA bit 3), writes bl to port 0x3C8 (DAC write "
    "address), then cx RGB triples from ds:si to port 0x3C9 (DAC data). "
    "The real palette-set primitive.",
    False,
)
ida_bytes.set_cmt(
    0x25A5B,
    "BIOS INT 10h/AX=1017h: reads all 256 DAC palette registers into "
    "es:dx (768-byte RGB-triple buffer).",
    False,
)
