"""
Names sub_12FC1, called from both ScrollClueEntryListPageUp and
ScrollClueEntryListPageDown right after they update the scroll offset
word_2E3F0 -- recomputes the clue entry list's page window bounds
rather than drawing anything (despite being described as "plausibly a
redraw" when those two callers were named; corrected here).

Defaults the visible-row count (word_2E3EC) to 0xE (14) and the page's
upper bound (word_2E3F2) to word_2E3F0+0x34 (52). If that upper bound
exceeds the category's total entry count (word_2E3F4), clamps
word_2E3F2 to word_2E3F4 instead and recomputes word_2E3EC as the
actual remaining-rows count: `(word_2E3F4-word_2E3F0)/4 + 1` (the /4
confirms a 4-entries-per-row grid, matching the step-by-4 stride seen
in the sibling HandleClueEntryRowScrollInput). -> RecomputeClueEntryPageBounds

Run via:
    .\run_ida_script.ps1 name_recompute_page_bounds.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12FC1
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RecomputeClueEntryPageBounds", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RecomputeClueEntryPageBounds': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Recomputes the clue entry list's page window from the new "
    "word_2E3F0: word_2E3F2 = word_2E3F0+0x34, clamped to total count "
    "word_2E3F4; word_2E3EC (visible row count) defaults to 0xE, or "
    "(word_2E3F4-word_2E3F0)/4+1 on the last partial page. Called "
    "from ScrollClueEntryListPageUp/Down.",
    False,
)
