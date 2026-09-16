"""
Names sub_12D5C, called once from RunClueEntryMenu -- a sibling
single-row scroll handler for the clue entry list, alongside the
already-named HandleClueEntryScrollInput ('I'/'Q', single-entry step)
and ScrollClueEntryListPageUp/Down (full-page jump). This one handles
'H'/'P' keys (0x48/0x50), stepping the visible cursor (word_2E3EE) by
4 (one grid row, given the /4 division found in sibling
RecomputeClueEntryPageBounds), and only invoking the full-page
functions when already at the current page's boundary:

- 'H': if word_2E3EE isn't yet at the page's top bound (word_2E3F0),
  just steps up by 4. If it is at the top bound, and word_328CC bit
  0x100 is set, calls ScrollClueEntryListPageUp and lands the cursor
  at the new page's bottom (word_2E3F2); otherwise stays clamped at
  word_2E3F0. errorCode=1.
- 'P': mirrors this for the bottom bound (word_2E3F2), stepping down
  by 4, or paging via ScrollClueEntryListPageDown (word_328CC bit
  0x80) and landing at the new page's top (word_2E3F0). errorCode=2.

-> HandleClueEntryRowScrollInput

Run via:
    .\run_ida_script.ps1 name_cluebook_row_scroll.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12D5C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleClueEntryRowScrollInput", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleClueEntryRowScrollInput': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "'H'/'P' single-row (step 4) scroll for the clue entry list, "
    "paging via ScrollClueEntryListPageUp/Down only at the current "
    "page's top/bottom bound (word_2E3F0/word_2E3F2). errorCode=1/2, "
    "0 if no active list/key. Called from RunClueEntryMenu.",
    False,
)
