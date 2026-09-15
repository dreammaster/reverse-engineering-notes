"""
Names sub_14C37, called twice from RunClueEntryMenu (the F8 clue book's
per-category menu loop, at +0x40 and +0xAE). Draws the clue book's top
navigation bar in two parts:

1. Two conditional hotkey hints, gated on word_328CC bits 0x40/0x20:
   msg 0x8EF7 = "d LIST" (i.e. "...d) LIST"), msg 0x8EFE = "MAP c"
   (i.e. "...c) MAP") -- confirmed by dumping the raw message bytes
   with surrounding context, which shows the full status-line string
   "a  MORE  b...d LIST...MAP c..." (a/b/c/d are the hotkey letters,
   MORE/LIST/MAP the option labels).

2. A row of 7 category-tab icons at fixed y=0xB4, x starting at 0x3E
   and stepping 0x1E per icon. A 7-entry table at DS:0xAFAC is built
   from 7 fixed base picture ids (0x20, 0x145, 0x147, 0x153, 0x149,
   0x14B, 0x14D) each conditionally incremented by 1 (a distinct
   "selected/highlighted" variant picture) based on 7 bits of
   word_328CC (0x8000 down to 0x200, one bit per tab) -- then all 7
   are drawn via DrawPicture. Reads as the clue book's category tab
   bar, each tab showing its own icon in a highlighted state when its
   bit is set. Exact category identities (which tab = which topic) not
   traced -- only the drawing mechanism is confirmed.

-> DrawClueBookNavBar

Run via:
    .\run_ida_script.ps1 name_clue_book_nav_bar.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14C37
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawClueBookNavBar", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawClueBookNavBar': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clue book (F8) nav bar, called from RunClueEntryMenu. Draws two "
    "conditional hotkey hints (word_328CC bit 0x40 -> 'd) LIST', bit "
    "0x20 -> 'c) MAP'), then a row of 7 category-tab icons at y=0xB4 "
    "(x from 0x3E, step 0x1E): 7 base picture ids (0x20/0x145/0x147/"
    "0x153/0x149/0x14B/0x14D), each +1'd to a highlighted variant when "
    "its bit (word_328CC 0x8000..0x200) is set, drawn via DrawPicture. "
    "Category identities not traced -- only the mechanism is confirmed.",
    False,
)
