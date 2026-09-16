"""
Names two more small helpers.

sub_2A766, called once from sub_2572C: strips commas from an in-place
string like StripCommasAndSpaces, but instead of also stripping
spaces, converts each space to '0' -- a zero-padding variant (e.g.
turning "  42" into "0042" after removing thousands separators).
-> StripCommasZeroPadSpaces

sub_2572C, called from the still-unnamed sub_2044C/sub_2047B and
others: chains FormatNumber with StripCommasZeroPadSpaces on the
shared 0xAFA8 buffer, the zero-padded sibling of FormatNumberCompact.
-> FormatNumberZeroPadded

sub_239CD, referenced from a data/jump table in seg073 (not a direct
call): clamps an accumulated drag position (word_2E782 += cx,
word_2E784 += dx) within bounds (word_2E778/word_2E77A horizontally,
word_31958/word_3195A vertically), then returns it in cx/dx -- offset
by (8,8) unless the currently-held item type (word_31946) is 0 (none)
or 0x1D (a specific item type that apparently doesn't need the
hotspot offset). Plausibly clamps and hotspot-adjusts the cursor
position used to draw a held/dragged item. -> ClampDragCursorPosition

Run via:
    .\run_ida_script.ps1 name_zero_pad_format_and_drag_clamp.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x2A766: "StripCommasZeroPadSpaces",
    0x2572C: "FormatNumberZeroPadded",
    0x239CD: "ClampDragCursorPosition",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2A766,
    "Strips ',' and converts ' ' to '0' in an in-place string -- the "
    "zero-pad sibling of StripCommasAndSpaces. Called from "
    "FormatNumberZeroPadded.",
    False,
)
ida_bytes.set_cmt(
    0x2572C,
    "FormatNumber then StripCommasZeroPadSpaces on the shared 0xAFA8 "
    "buffer -- zero-padded sibling of FormatNumberCompact. Called "
    "from sub_2044C, sub_2047B, and others.",
    False,
)
ida_bytes.set_cmt(
    0x239CD,
    "Clamps an accumulated drag position (word_2E782/word_2E784) "
    "within bounds, then offsets by (8,8) unless the held item type "
    "(word_31946) is 0 or 0x1D -- plausibly the cursor position used "
    "to draw a held/dragged item. Referenced from a data/jump table "
    "in seg073.",
    False,
)
