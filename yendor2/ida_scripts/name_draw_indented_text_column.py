"""
Names sub_28A76, called from ShowHealingCostPrompt and unnamed
sub_1A5F6 -- a mode-dispatching wrapper around the plain
DrawStringColumn and a per-line helper sub_28B94, selecting between
several "how many leading lines get zero indent" wrapping patterns
based on word_328C4 bits.

If word_328C4 bit 0x20 is set, or bit 0x2 is clear, just calls
DrawStringColumn directly (optionally zeroing fontOffset first if bit
0x2 is set). Otherwise picks one of 3 looped patterns (each calling
sub_28B94 up to 5 times per cycle, stopping early once it reports
cx==0 -- presumably "no more text"), differing in how many of the
per-cycle calls reset fontOffset to 0 first: bit 0x10 resets before
every call (fontOffset never accumulates), bit 0x8 resets before the
first 2, bit 0x4 resets before the first 4 (of 5). Reads as drawing a
word-wrapped text column with a caller-selectable "hanging indent"
style, where fontOffset is the per-line indent and these modes control
how many leading lines are unindented before continuation lines pick
up the accumulated offset. -> DrawIndentedTextColumn

Run via:
    .\run_ida_script.ps1 name_draw_indented_text_column.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28A76
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawIndentedTextColumn", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawIndentedTextColumn': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Dispatches on word_328C4 bits to either call DrawStringColumn "
    "directly, or loop sub_28B94 (per-line draw, cx==0 stops) with "
    "one of 3 'how many leading lines get fontOffset reset to 0' "
    "patterns (bit 0x10=every line, 0x8=first 2, 0x4=first 4 of 5) -- "
    "a hanging-indent text column mode selector. Called from "
    "ShowHealingCostPrompt and sub_1A5F6.",
    False,
)
