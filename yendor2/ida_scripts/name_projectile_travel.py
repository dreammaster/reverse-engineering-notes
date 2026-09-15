"""
Names sub_2BAA0 and sub_11236 -- the projectile-travel animation and
per-row obstacle/target classification used by sub_1D4B8 (the
combat-round driver, not yet fully named) to animate a ranged attack
or spell traveling down the corridor and detect what it hits.

sub_2BAA0 (-> AnimateProjectileStep): draws a sprite via
DrawViewportSprite at z-layer 5 (a new layer value alongside the
0/1/2/3/4/6/7/8 layers already identified -- a projectile/effect
frame), redraws the mouse cursor, calls sub_2BC16 (not traced,
plausibly a whoosh/travel sound), waits 2 ticks, resets errorCode.
One animation step of a projectile advancing one depth row.

sub_11236 (-> ClassifyObstacleAtViewportRow): looks up the dungeon-
viewport scratch buffer (the same 0x6D60 buffer GetMonsterAtViewportRow
reads) at the current depth row and classifies what's there into
errorCode: 0 = clear, 1 = a wall (cell type in a blocking range), 2 =
a door/side-feature in a blocking range, 3 = a [+6] bit 0x800 feature,
4 = a monster (found via FindMonsterTypeInLevelPool). Called
repeatedly by sub_1D4B8, each time preceded by AnimateProjectileStep --
together they animate a projectile traveling down the corridor one
row at a time until it hits something.

Run via:
    .\run_ida_script.ps1 name_projectile_travel.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2BAA0: "AnimateProjectileStep",
    0x11236: "ClassifyObstacleAtViewportRow",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2BAA0,
    "One animation step of a projectile/effect traveling down the "
    "corridor: draws it via DrawViewportSprite (z-layer 5), redraws "
    "the cursor, plays a sound (sub_2BC16, not traced), waits 2 ticks. "
    "Called repeatedly from sub_1D4B8, each time followed by "
    "ClassifyObstacleAtViewportRow to check what's at the next row.",
    False,
)
ida_bytes.set_cmt(
    0x11236,
    "Classifies what's at the current depth row in the dungeon-"
    "viewport scratch buffer into errorCode: 0=clear, 1=wall, "
    "2=door/side-feature, 3=a [+6] bit 0x800 feature, "
    "4=monster (FindMonsterTypeInLevelPool). Called repeatedly by "
    "sub_1D4B8 as a projectile travels down the corridor.",
    False,
)
