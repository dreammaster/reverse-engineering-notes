"""
Names the first-person dungeon corridor viewport renderer and its
per-cell encounter check, tracing SpawnMonsterInFacingDirection's
caller chain one level further.

sub_20FB7 (-> RenderDungeonViewport): called from sub_20C1E and
sub_20C46 (both unnamed). Resets word_3292C to 0, then calls sub_21015
six times with decreasing cell counts (0x11, 0x11, 5, 3, 3, 3) and a
different row-data pointer each time (word_328E6/E8/EA/EC/EE/F0),
followed once by a 7th, different function (sub_21217, word_328F2) --
the classic "render each depth row of the visible corridor, near to
far" shape of a first-person dungeon crawler's 3D-perspective view.

sub_21015 (-> RenderDungeonViewRow): renders one row: iterates a
sequence of 8-byte cell records (forward then backward from a
midpoint, matching a "draw left wall then right wall" or similar
split), drawing each cell's picture from a 12-byte-stride lookup table
at 0xE551 via sub_29B0F when a visibility-ish flag ([+6] bit 0) is
clear, and calling sub_212B8 once per cell -- incrementing/
decrementing word_3292C as it goes. Confirms word_3292C is a per-frame
row-depth counter, not a global value.

sub_212B8 (-> TryTriggerMonsterEncounterAtCell): gates on
word_3292C >= 0x11 -- since RenderDungeonViewport only passes cx=0x11
for its first two (farthest) rows, monsters can only spawn in the
farthest visible cells, not right next to the party -- and a flag bit
on the cell record ([di+6] bit 0x400), then a probability check
(sub_22B78) before calling SpawnMonsterInFacingDirection.

Confidently the dungeon rendering + random-encounter-trigger system;
several called helpers (sub_20C1E, sub_20C46, sub_21217, sub_21128,
sub_2117F, sub_22B78, sub_29B0F) remain untraced.

Run via:
    .\run_ida_script.ps1 name_dungeon_viewport_renderer.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x20FB7: "RenderDungeonViewport",
    0x21015: "RenderDungeonViewRow",
    0x212B8: "TryTriggerMonsterEncounterAtCell",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x20FB7,
    "First-person dungeon corridor viewport renderer: resets "
    "word_3292C, calls RenderDungeonViewRow 6x with decreasing cell "
    "counts (0x11/0x11/5/3/3/3) and different row-data pointers "
    "(word_328E6..F0), then sub_21217 once more (word_328F2). Called "
    "from sub_20C1E and sub_20C46.",
    False,
)
ida_bytes.set_cmt(
    0x21015,
    "Renders one depth row of the dungeon corridor view: iterates "
    "8-byte cell records (forward then backward from a midpoint), "
    "drawing each cell's picture (table 0xE551, 12-byte stride) via "
    "sub_29B0F, and calls TryTriggerMonsterEncounterAtCell once per "
    "cell while incrementing/decrementing word_3292C (a per-frame row "
    "depth counter). Called 6x by RenderDungeonViewport.",
    False,
)
ida_bytes.set_cmt(
    0x212B8,
    "Per-cell encounter check: only fires for word_3292C >= 0x11 (the "
    "farthest visible rows) and a flag bit on the cell record "
    "([di+6] bit 0x400); rolls a probability (sub_22B78) before "
    "calling SpawnMonsterInFacingDirection. Called once per cell from "
    "RenderDungeonViewRow.",
    False,
)
