"""
Read-only: dumps the parameters that fix the world map's location and shape
in WORLD.DAT, for docs23/file-formats.md and src23/worldmap.c.

  1. Table 0xCDEF (read by the generic PrepareWorldDatRead stub): the 32-bit
     WORLD.DAT base offset RefreshDungeonMapWindow reads map rows from.
     0 in the real data -- the map starts at the very beginning of the file.
  2. The named globals at the WORLD.DAT FileEntry's own field offsets
     (base 0x9043), confirming which globals RefreshDungeonMapWindow uses as
     the per-row block index (+8: word_368AB) versus CURGAME's own (+8:
     g_groundItemSlotRecord) -- these turned out to be the same names
     already used elsewhere for unrelated things (ground-item slots), which
     caused real confusion when first tracing this function; this script is
     what resolved it.

_blockSize3 (row size = 4*_blockSize3 = 3200 bytes) isn't dumped here: it's
a runtime-only global (0 in the static .idb image), so it has to be read
from InitGlobals' own "mov _blockSize3, 320h" instruction in the disassembly
instead (yendor2.asm and yendor3.asm both set it to 0x320).

    .\run_ida_script.ps1 dump_map_layout.py -NoExport
"""
import ida_bytes
import ida_name

DS_BASE = 0x2D860

print("map base-offset table @DS:0xCDEF =", hex(ida_bytes.get_wide_dword(DS_BASE + 0xCDEF)))

print("\nFileEntry field names (this confirms which named global is which struct field):")
for base_label, base in (("WORLD.DAT FileEntry (0x9043)", 0x9043), ("CURGAME FileEntry (0x8FFB)", 0x8FFB)):
    print(" ", base_label)
    for off in (0, 2, 4, 6, 8, 0xA, 0xC, 0xE):
        ea = DS_BASE + base + off
        print("    +%#x  name=%r" % (off, ida_name.get_name(ea)))
