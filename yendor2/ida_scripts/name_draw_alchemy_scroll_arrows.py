"""
Names sub_1E356, called twice from RunAlchemyScreen -- the alchemy
spell list's scroll-arrow indicator.

Clears word_328CA bits 0xC00, then sets bit 0x800 (and draws an
up-arrow glyph char at x=0xA5) if word_33316 (current page) > 1, and
sets bit 0x400 (and draws a down-arrow glyph char at x=0xB0) if
word_33314 (last/total page) > word_33316. A standard "can scroll
up"/"can scroll down" pagination indicator for the alchemy screen's
13-spells-per-page list (file-formats.md). -> DrawAlchemySpellListScrollArrows

Run via:
    .\run_ida_script.ps1 name_draw_alchemy_scroll_arrows.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1E356
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawAlchemySpellListScrollArrows", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawAlchemySpellListScrollArrows': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Pagination scroll-arrow indicator: draws an up-arrow glyph "
    "(x=0xA5, sets word_328CA bit 0x800) if word_33316 (current "
    "page) > 1, and a down-arrow glyph (x=0xB0, sets bit 0x400) if "
    "word_33314 (last page) > word_33316. Called from "
    "RunAlchemyScreen for its 13-spells-per-page list.",
    False,
)
