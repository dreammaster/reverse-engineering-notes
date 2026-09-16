"""
Names the pair sub_13014/sub_12FED, called from both
HandleClueEntryScrollInput (its deferred-to "special mode" branches)
and unnamed sub_12D5C -- a full-page jump-scroll for the clue book's
entry list, as opposed to HandleClueEntryScrollInput's own single-step
adjustment.

Both work the same way: subtract the current page offset (word_2E3F0)
back out of word_2E3EE, then recompute word_2E3F0 by a fixed page
size of 0x38 (56) -- sub_13014 subtracts it (page up), clamped to a
minimum of word_2E3EA+2; sub_12FED adds it (page down), clamping the
resulting word_2E3EE against the upper bound word_2E3F2 -- then
re-adds the new offset into word_2E3EE, calls still-unnamed sub_12FC1
(plausibly a redraw of the newly-scrolled-to page), and sets errorCode
(1/2, matching HandleClueEntryScrollInput's own convention) to signal
the change.
-> ScrollClueEntryListPageUp / ScrollClueEntryListPageDown

Run via:
    .\run_ida_script.ps1 name_cluebook_page_scroll.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x13014: "ScrollClueEntryListPageUp",
    0x12FED: "ScrollClueEntryListPageDown",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x13014,
    "Full page-up jump for the clue entry list: recomputes "
    "word_2E3F0 -= 0x38 (clamped to word_2E3EA+2), adjusts "
    "word_2E3EE by the delta, calls sub_12FC1 (redraw), errorCode=1. "
    "Called from HandleClueEntryScrollInput and sub_12D5C.",
    False,
)
ida_bytes.set_cmt(
    0x12FED,
    "Full page-down jump for the clue entry list: recomputes "
    "word_2E3F0 += 0x38 (word_2E3EE clamped to upper bound "
    "word_2E3F2), calls sub_12FC1 (redraw), errorCode=2. Called from "
    "HandleClueEntryScrollInput and sub_12D5C.",
    False,
)
