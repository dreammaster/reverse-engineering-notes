"""
Names 4 more small helpers.

sub_12FB0 (called from ShowClueCategoryEntries and
AssignClueCategoryEntryIds): clears the [+8] id field across all 14
entries of the clue category hit-test table (0x68D2) -- the reset
counterpart to AssignClueCategoryEntryIds.
-> ClearClueCategoryEntryIds

sub_13FBF (called from DrawClueBookMapGrid and
ListCompatibleClueBookItems): fills a 250-word scratch buffer
(0x6976) with the sentinel value 0xFFFF. -> ResetClueBookMarkerBuffer

sub_203E4 (called twice from FillVisibleAreaWithSelectedTile): nudges
word_2E496 by 1 toward word_2E384 (increment or decrement to
converge) -- a step-counter-toward-target primitive.
-> StepCounterTowardTarget

sub_209C0 (called from TickStatusEffects and ApplyStatusEffect):
returns si = word_3297A if it's nonzero (an active override),
otherwise si = word_32978 + word_3297C (a computed base+offset) --
resolves which icon-bar slot address to use.
-> ResolveIconBarBaseAddress

Run via:
    .\run_ida_script.ps1 name_final_small_cluster.py
"""
import idc
import ida_name
import ida_bytes

entries = [
    (0x12FB0, "ClearClueCategoryEntryIds",
     "Clears the [+8] id field across all 14 entries of the clue "
     "category hit-test table (0x68D2). Reset counterpart to "
     "AssignClueCategoryEntryIds."),
    (0x13FBF, "ResetClueBookMarkerBuffer",
     "Fills a 250-word scratch buffer (0x6976) with sentinel 0xFFFF. "
     "Called from DrawClueBookMapGrid and "
     "ListCompatibleClueBookItems."),
    (0x203E4, "StepCounterTowardTarget",
     "Nudges word_2E496 by 1 toward word_2E384. Called twice from "
     "FillVisibleAreaWithSelectedTile."),
    (0x209C0, "ResolveIconBarBaseAddress",
     "Returns word_3297A if nonzero (an active override), else "
     "word_32978+word_3297C (computed base+offset). Called from "
     "TickStatusEffects and ApplyStatusEffect."),
]

for ea, name, desc in entries:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
