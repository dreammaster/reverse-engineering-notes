"""
Names sub_21CC2: MarkCellExplored's reveal action, and it turns out to
be about persistence, not rendering. Configures a resource read
(sub_27E20, one of the WORLD.DAT/CURGAME resource-stub family) with the
cell's (x, y) as parameters, reads it from CURGAME (FileEntry bx=0x8FFB),
computes a bit position (x/8 byte offset + word_3685F base, x%8 bit
within the byte via `0x80 >> (x%8)`), ORs the bit into that byte
(setting it), then writes the whole record back to CURGAME. This is the
automap's "explored" bitmap being persisted into the savegame itself --
not a rendering step. -> PersistExploredCell

Run via:
    .\run_ida_script.ps1 name_persist_explored.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x21CC2
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PersistExploredCell", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PersistExploredCell': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Persists one cell's explored bit into CURGAME: reads a record "
    "(sub_27E20, params = cell x/y) then sets bit (x%8) of byte "
    "(x/8 + word_3685F) and writes the record back. The automap's "
    "explored bitmap is saved in the savegame itself, not just kept in "
    "memory. Called by MarkCellExplored on newly-discovered cells.",
    False,
)
