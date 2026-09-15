"""
Correction: AnimateProjectileStep's comment guessed sub_2BC16 was
"plausibly a whoosh/travel sound" -- wrong, it's
RestoreCorridorBackgroundFromEMS, a graphics blit, not audio.

Run via:
    .\run_ida_script.ps1 fix_animate_projectile_comment.py
"""
import ida_bytes

ea = 0x2BAA0
ida_bytes.set_cmt(
    ea,
    "One animation step of a projectile/effect traveling down the "
    "corridor: draws it via DrawViewportSprite (z-layer 5), redraws "
    "the cursor, restores the background via "
    "RestoreCorridorBackgroundFromEMS (CORRECTION: not a sound effect "
    "as first guessed -- it's an EMS-backed graphics blit), waits 2 "
    "ticks. Called repeatedly from sub_1D4B8, each time followed by "
    "ClassifyObstacleAtViewportRow to check what's at the next row.",
    False,
)
print("comment corrected")
