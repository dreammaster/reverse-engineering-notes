"""
CORRECTION: the last 4 rounds named a chain of functions
(CheckWorldDatCompatibility, BuildLoadValidationMessage,
DrawSaveSlotList, ShowSaveSlotMenu) as a save/load-slot menu. Wrong --
checked every caller of the chain's entry point (sub_12B84) and found
all 13 of them are inside ShowClueBook (the F8 on-line clue book,
confirmed by its own pre-existing comment quoting the manual). There
is no other caller anywhere. This is the clue-book's entry-list
system, not a save-game menu.

Reframing with this context:

- CheckWorldDatCompatibility -> BuildClueLocationSuffix: reads a clue
  record from WORLD.DAT (block 3) plus a secondary field, trims
  trailing spaces. Two character positions hold an encoded level/map
  number (not validation sentinels) -- if the first is set (!='0'),
  appends ' LEVEL X' to the clue text and returns 2; else if the
  second is set (!=' '), appends ' MAP X' and returns 1; else returns
  0 (a generic clue with no location suffix). So clue-book entries are
  tagged with which level/map they apply to, when relevant.

- BuildLoadValidationMessage -> BuildClueEntryText: dispatches on
  word_2E3F6 (a clue CATEGORY selector, not a failure type) to
  whichever lookup applies for that category (state 1 uses
  BuildClueLocationSuffix), composing each individual clue's full
  display text.

- DrawSaveSlotList -> DrawClueEntryList: draws the scrollable list of
  individual clue entries for the current category, highlighting the
  selected one, with scroll indicators.

- ShowSaveSlotMenu -> ShowClueCategoryEntries: the per-category
  init+draw step -- reads the category's entry count, initializes
  scroll/selection state, draws the frame and the entry list.

- sub_12B84 stays unnamed this round (it's the interactive per-
  category menu loop, PollKeyboardInput-driven) -- a reasonable next
  target, but the chain underneath it is the priority fix.

Run via:
    .\run_ida_script.ps1 fix_cluebook_not_saveslot.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x21DE2: "BuildClueLocationSuffix",
    0x12ECD: "BuildClueEntryText",
    0x12C96: "DrawClueEntryList",
    0x12E59: "ShowClueCategoryEntries",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x21DE2,
    "CORRECTED from 'CheckWorldDatCompatibility' -- this is the "
    "clue-book's entry system (all callers trace to ShowClueBook, "
    "the F8 on-line clue book), not a save/load check. Reads a clue "
    "record from WORLD.DAT (block 3) plus a secondary field. Two "
    "character positions hold an encoded level/map number: if set, "
    "appends ' LEVEL X' (returns 2) or ' MAP X' (returns 1) to the "
    "clue text; else returns 0 (generic clue, no location suffix).",
    False,
)
ida_bytes.set_cmt(
    0x12ECD,
    "CORRECTED from 'BuildLoadValidationMessage'. Dispatches on "
    "word_2E3F6 (a clue-book CATEGORY selector, ~16 categories) to "
    "compose one clue entry's full display text -- category 1 uses "
    "BuildClueLocationSuffix for the level/map tag; other categories "
    "use different lookups, not individually traced.",
    False,
)
ida_bytes.set_cmt(
    0x12C96,
    "CORRECTED from 'DrawSaveSlotList'. Draws the scrollable list of "
    "individual clue-book entries for the current category "
    "(word_2E3EC entries, table at 0x68D2), highlighting the "
    "selected one (word_2E3EE), text from BuildClueEntryText, with "
    "scroll indicators when the category has more entries than fit "
    "on screen.",
    False,
)
ida_bytes.set_cmt(
    0x12E59,
    "CORRECTED from 'ShowSaveSlotMenu'. Per-category clue-book "
    "init+draw: on first call, reads this category's entry count "
    "(0xF3F4, indexed by word_2E3F6) and initializes scroll/"
    "selection state; every call draws the frame (DrawMessageBox) "
    "plus header/footer and the entry list (DrawClueEntryList).",
    False,
)
