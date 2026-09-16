"""
Names sub_22315, called from HandleMovementInput and
ProcessLevelMonsters -- draws a small status icon at a fixed position
and caches it into the EMS portrait/dungeon page.

Picks picture variant 3 or 1 (word_2E530) based on word_328CA bit
0x1000 (the same "in combat" style flag seen elsewhere), draws it at
a fixed position (0xEF, 0x43) into the offscreen buffer via
DrawPicture, then maps EMS page 0x55D8 (the confirmed portrait/
dungeon-screen cluster page) and copies a small 16-row region
(8 words/row) from the offscreen buffer into the EMS page frame at
the matching offset -- caching the just-drawn icon for later
restoration, the same convention documented for the portrait cache.
-> DrawAndCacheStatusIcon

Run via:
    .\run_ida_script.ps1 name_draw_and_cache_status_icon.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x22315
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawAndCacheStatusIcon", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawAndCacheStatusIcon': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws picture variant 1/3 (gated on word_328CA bit 0x1000) at "
    "fixed position (0xEF,0x43), then caches the drawn region into "
    "EMS page 0x55D8 (the portrait/dungeon cluster page). Called from "
    "HandleMovementInput and ProcessLevelMonsters.",
    False,
)
