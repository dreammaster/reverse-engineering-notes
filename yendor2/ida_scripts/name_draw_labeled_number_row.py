"""
Names sub_13C86, called from ShowClueBookSpellDetail (for its "MP:"/
"NUORE:"/"ORE:" cost-field rows) and from sibling sub_13C1D (which
uses it for a fixed "1" value with a special highlight color 0xA7,
gated on a 2-entry class-id match against word_3330A -- plausibly
part of the "6-class eligibility marker row" documented for that
screen). A generic "draw one row: caller-preset text, a formatted
number, then a caller-preset string again, advance to the next line"
primitive:

1. writeString at _textPos_x=0x7A using whatever si the caller set
   beforehand.
2. Sets _font_fgColor (caller-configurable) and _textPos_x=0x68, then
   FormatNumber+StripCommasAndSpaces (the same pattern as
   FormatNumberCompact) on the input number (ax), writeString's the
   compact result.
3. _textPos_x=0x2C, writeString again with bx/si carried over from
   step 2 (whether this redraws the same text or something written by
   FormatNumber isn't fully confirmed -- si isn't visibly reset
   between the two writeString calls in this function, but
   writeString's own effect on si wasn't traced this round).
4. Advances _textPos_y by 6 (next line) and clears errorCode.

-> DrawLabeledNumberRow

Run via:
    .\run_ida_script.ps1 name_draw_labeled_number_row.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x13C86
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawLabeledNumberRow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawLabeledNumberRow': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws a row: writeString at x=0x7A (caller's si), a "
    "FormatNumber+StripCommasAndSpaces'd number (ax) at x=0x68, then "
    "writeString again at x=0x2C, advances _textPos_y by 6, clears "
    "errorCode. Called from ShowClueBookSpellDetail (cost fields) and "
    "sub_13C1D (a class-eligibility marker).",
    False,
)
