"""
Names sub_2BC16 and sub_2BC72 -- EMS-backed background-restore
blitters for the dungeon viewport, and corrects a wrong guess in
AnimateProjectileStep's comment (it called sub_2BC16 "plausibly a
whoosh/travel sound" -- wrong, it's graphics, not audio).

sub_2BC16 (-> RestoreCorridorBackgroundFromEMS): maps EMS page 0x55FE
if not already cached (word_2E500), then blits a fixed-size region
(136 rows x 112 words) from the EMS page frame into the video buffer
at a fixed offset (0xA08) -- restores the corridor viewport background
after a sprite (e.g. a projectile) was drawn over it. Called from
HandleRangedOrCombatAction and AnimateProjectileStep.

sub_2BC72 (-> ScrollCorridorBackgroundFromEMS): the parameterized
sibling -- same EMS source/video-buffer blit, but ax/bx/cx adjust the
region's offsets and dimensions to scroll/shift it directionally
(each axis independently, by cx units). Called from sub_2C0FE (an
unnamed dispatcher, multiple sites).

Run via:
    .\run_ida_script.ps1 name_ems_background_restore.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2BC16: "RestoreCorridorBackgroundFromEMS",
    0x2BC72: "ScrollCorridorBackgroundFromEMS",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2BC16,
    "Maps EMS page 0x55FE if not cached, blits a fixed 136x112-word "
    "region from the EMS page frame into the video buffer at offset "
    "0xA08 -- restores the corridor viewport background over a drawn "
    "sprite. Called from HandleRangedOrCombatAction and "
    "AnimateProjectileStep (NOT a sound effect, despite an earlier "
    "comment's guess).",
    False,
)
ida_bytes.set_cmt(
    0x2BC72,
    "Parameterized sibling of RestoreCorridorBackgroundFromEMS: "
    "ax/bx/cx shift the blitted region's offsets/dimensions to "
    "scroll it directionally. Called from sub_2C0FE.",
    False,
)
