"""
CORRECTION: DrawThreeThresholdStats's comment guessed its 3 fields
(+0x4C/+0x4E/+0x50) were "plausibly 3 primary attributes" -- wrong.
RollCharacterAttributes' own pre-existing comment already documents
the 6 core attributes at a *different* set of offsets: +0x3C/+0x7C,
+0x3E/+0x7E, +0x40/+0x80, +0x42/+0x82, +0x44/+0x84, +0x46/+0x86 (6
fields, not 3, and a different range entirely). So +0x4C/+0x4E/+0x50
are NOT the primary attributes -- they're something else, drawn
alongside the attributes on the post-roll display, plausibly derived
combat stats (not identified).

Run via:
    .\run_ida_script.ps1 fix_threshold_stats_comment.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x24CAD,
    "Draws 3 threshold-highlighted stat values from the current party "
    "record: [+0x4C]/[+0x8C], [+0x4E]/[+0x8E], [+0x50]/[+0x90]. "
    "CORRECTION: NOT the 6 primary attributes -- those are confirmed "
    "at a different offset range (+0x3C/+0x7C..+0x46/+0x86, per "
    "RollCharacterAttributes' own comment). These 3 fields' identity "
    "is not confirmed; drawn alongside the attributes on the "
    "post-attribute-roll display in ShowCharacterSkills. Called from "
    "sub_23C18 and ShowCharacterSkills.",
    False,
)
print("comment corrected")
