"""
Names sub_12DD8, called once from RunClueEntryMenu (the clue book's
per-category menu loop) -- a pagination input handler in the same
"I"/"Q" hotkey family as the already-named HandlePagedEntryNavigation,
but for the clue-book entry list, and additionally driven by a mouse
hit-test.

Only runs if word_2E3EA is nonzero (some "list has content" gate).
Accepts either the last keypress (byte_2E400 == 'I'/0x49 or
'Q'/0x51) or a mouse click resolved via HitTestRegionTable against
table 0x6960 (returning 0=no hit, 1=~'I' region, 2=~'Q' region).

- 'I'/region 1: if word_328CC bit 0x100 is set, defers entirely to
  still-unnamed sub_13014 (plausibly a "jump" variant active in some
  special mode) and returns. Otherwise, if the candidate index
  word_2E3F0 differs from the current word_2E3EE, adopts it and
  signals change via errorCode=1.
- 'Q'/region 2: same shape, gated on word_328CC bit 0x80, deferring to
  still-unnamed sub_12FED, else comparing/adopting word_2E3F2 with
  errorCode=2.
- Otherwise (no change, or the gate/compare didn't fire): errorCode=0.

Exact identity of word_2E3F0/word_2E3F2 (plausibly precomputed
previous/next clamped indices) and of sub_13014/sub_12FED isn't
confirmed, so left unnamed this round. -> HandleClueEntryScrollInput

Run via:
    .\run_ida_script.ps1 name_handle_clue_entry_scroll.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12DD8
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleClueEntryScrollInput", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleClueEntryScrollInput': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Pagination handler for RunClueEntryMenu's entry list: 'I' key or "
    "a HitTestRegionTable(0x6960) mouse hit region 1 adopts "
    "word_2E3F0 into word_2E3EE (errorCode=1) unless word_328CC bit "
    "0x100 defers to sub_13014; 'Q'/region 2 does the same with "
    "word_2E3F2 (errorCode=2), deferring to sub_12FED on word_328CC "
    "bit 0x80. errorCode=0 if nothing changed. Called from "
    "RunClueEntryMenu.",
    False,
)
