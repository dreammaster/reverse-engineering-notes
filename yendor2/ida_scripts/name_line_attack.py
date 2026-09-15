"""
Names sub_233D0 and sub_1DA2C -- a multi-cell "straight line" attack
mechanism, tying the dungeon-viewport scratch buffer directly into
combat.

sub_233D0 (-> GetMonsterAtViewportRow): looks up the dungeon-viewport
scratch buffer (0x6D60 + word_3292C*8 -- the exact same buffer
RenderDungeonViewRow/DrawDungeonCellWallTexture read) for the current
depth row. If the cell's [+6] flags have bit 0x400 set (the same
"monster present" flag DrawDungeonCellWallTexture checks to draw its
overlay picture 5), looks up that monster's type ([+4]) via
FindMonsterTypeInLevelPool and returns its record (si) with ZF
reflecting found/not-found; otherwise returns si=0/not-found.

sub_1DA2C (-> ApplyDamageAlongCorridorLine): calls
GetMonsterAtViewportRow for 3 consecutive depth rows (incrementing
word_3292C each time), applying ApplyResolvedDamageWithResistance to
whichever monster (if any) is found at each -- a straight-line/
multi-target attack, matching the "IN A STRAIGHT LINE" targeting text
already dumped from ShowClueBookSpellDetail's message table.

Run via:
    .\run_ida_script.ps1 name_line_attack.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x233D0: "GetMonsterAtViewportRow",
    0x1DA2C: "ApplyDamageAlongCorridorLine",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x233D0,
    "Looks up the dungeon-viewport scratch buffer (0x6D60 + "
    "word_3292C*8) for a monster at the current depth row -- if the "
    "cell's [+6] bit 0x400 'monster present' flag is set, resolves it "
    "via FindMonsterTypeInLevelPool. Called from "
    "ApplyDamageAlongCorridorLine and sub_2C0FE.",
    False,
)
ida_bytes.set_cmt(
    0x1DA2C,
    "Straight-line multi-target attack: calls GetMonsterAtViewportRow "
    "for 3 consecutive depth rows (word_3292C incrementing), applying "
    "ApplyResolvedDamageWithResistance to whatever monster is found at "
    "each. Matches the 'IN A STRAIGHT LINE' targeting text from "
    "ShowClueBookSpellDetail's message table. Called from sub_1DA60.",
    False,
)
