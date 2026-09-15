"""
Names sub_212EB, called from RenderDungeonVanishingPoint when
word_328CA bit 0x1000 is set (plausibly "in combat"). Iterates the 3
g_monsterSlots records (base 0x51C0, confirmed in file-formats.md as
"up to 3 simultaneous active monsters") and calls sub_20E54 (not
traced) for each occupied slot ([+0] != 0) -- reads as drawing the
active combat monsters' sprites into the dungeon viewport.

-> RenderActiveMonsterSprites

Run via:
    .\run_ida_script.ps1 name_render_active_monsters.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x212EB
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RenderActiveMonsterSprites", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RenderActiveMonsterSprites': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Iterates the 3 g_monsterSlots records, calling sub_20E54 (not "
    "traced) for each occupied one -- draws the active combat "
    "monsters' sprites into the dungeon viewport. Called from "
    "RenderDungeonVanishingPoint when word_328CA bit 0x1000 is set.",
    False,
)
