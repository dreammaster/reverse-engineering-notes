"""
Names sub_1770C: a FOURTH fixed FileEntry discovered this round, at
bx=0x8FFB -- filename "CURGAME" (the active savegame file, per
docs/file-formats.md; distinct from the SAVGAMEX FileEntry at 0x902C
used for the numbered save slots). Reads a record via FileEntry_Read
(configured by the resource stub sub_27DE5), maps it into EMS
(MapUnmapPages), computes an offset (word_32DBC*4 + 0x1A*_val9 --
plausibly a per-character row, stride 0x1A=26, within a small table),
copies 2 words out of the mapped page, then splits word_32DD0 by 100
(quotient/remainder) -- a shape very typical of splitting a currency or
time value into two denominations, not confirmed which.

-> LoadCurgameRecord (moderate confidence on the exact semantics, high
confidence it's reading player/party state from the active save)

Run via:
    .\run_ida_script.ps1 name_curgame_reader.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1770C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "LoadCurgameRecord", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'LoadCurgameRecord': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Reads a record from CURGAME (the active savegame, FileEntry "
    "bx=0x8FFB) via EMS paging, indexed by word_32DBC*4 + 0x1A*_val9 "
    "(plausibly a per-character row). Splits word_32DD0 by 100 into "
    "word_32DC0 (quotient)/word_32DC2 (remainder) -- typical of a "
    "currency or time value split into two denominations, not "
    "confirmed which.",
    False,
)
