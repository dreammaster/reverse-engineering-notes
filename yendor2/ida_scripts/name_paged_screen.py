"""
Names a small, self-contained UI cluster traced this session while
following up on the text-drawing candidates from
rank_naming_candidates.py:

- sub_132F2: top-level handler. Calls sub_150E5 (draw box + text),
  sub_138C0 (scroll-arrow update), Fade?, and reads/writes word_3293A
  (current page/entry index, bounded 1..0x1F) plus several word_2E3xx
  "msg"-commented globals. -> ShowPagedEntryScreen
- sub_138C0: clears then conditionally sets two 1-character glyphs
  (aAMoreB / byte_36755) to 'a'/'b' or blank depending on whether
  word_3293A is at the first (1) or last (0x1F) entry -- a scroll-
  arrow-availability indicator. Also loads the entry's data via
  FileEntry_Read (bx=0x9043, record index word_3293A-1, record size
  0x4FB) and draws an icon (sub_23B76) + a message (writeString).
  -> UpdateScrollArrows
- sub_150E5: fills/draws a box (sub_14B24), sets font colors/position,
  and prints two strings from word_2E3F8/word_2E3FA (both already
  commented "msg" from the reapplied prior annotations).
  -> DrawMessageBox

Not fully confirmed whether the displayed content is book/sign text, a
spellbook, or another paginated catalog -- names are chosen to reflect
the structurally-confirmed behavior (paginated single-entry display with
icon + text + scroll arrows) without asserting which game content it is.

Run via:
    .\run_ida_script.ps1 name_paged_screen.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x132F2: "ShowPagedEntryScreen",
    0x138C0: "UpdateScrollArrows",
    0x150E5: "DrawMessageBox",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x132F2,
    "Top-level paginated-entry display screen: shows one entry (via "
    "DrawMessageBox), the scroll-arrow state for the current page "
    "(via UpdateScrollArrows), and a Fade? transition. Current page "
    "index is word_3293A (bounded 1..0x1F). Content type (book/sign "
    "text vs. a catalog like spells) not confirmed.",
    False,
)
ida_bytes.set_cmt(
    0x138C0,
    "Shows/hides the two scroll-arrow glyphs (aAMoreB / byte_36755) "
    "based on whether word_3293A (current entry index) is at the first "
    "(1) or last (0x1F) entry, then loads that entry's data via "
    "FileEntry_Read (fixed FileEntry at bx=0x9043, record size 0x4FB) "
    "and draws its icon + message.",
    False,
)
ida_bytes.set_cmt(
    0x150E5,
    "Draws a box (via sub_14B24) then two lines of text from "
    "word_2E3F8/word_2E3FA (both commented 'msg'), positioned via "
    "word_2E3FC. Called by ShowPagedEntryScreen.",
    False,
)
