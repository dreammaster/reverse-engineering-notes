"""
Names sub_28034, called from a raw address (seg000:0AFA) very early
in the binary that IDA never resolved into a named function -- not
independently confirmed, but plausibly a low-level hook (e.g. a
timer/interrupt tick) given how early the call site sits and how
heavily this function is gated against interfering with other
screens.

Returns immediately unless word_328C6 bits 0x80 and 0x200 are both
clear, word_328CA bit 0x1000 is clear, AND word_328C4 bit 0x2000 is
set -- i.e. only when a specific debug-overlay flag is on and no
conflicting screen/mode is active. When active, draws 3 rows of
"<label><value>  <label><value>" pairs via FormatNumberCompact +
StpCpy + writeString: row 1 labeled 'H'/'V' for word_2E55C/word_2E564,
row 2 (same 'H'/'V' labels) for word_36CF7/word_36CF9 (the confirmed
party world X/Y position, per EnforceDemoBoundary), row 3 labeled
'B'/'F' for a pair read from a table indexed by word_328D2. Reads as
a debug HUD overlay showing 3 pairs of coordinate-like values; the
exact meaning of the H/V and B/F field pairs (beyond row 2 being
world X/Y) is not confirmed. -> DrawDebugPositionOverlay

Run via:
    .\run_ida_script.ps1 name_draw_debug_position_overlay.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x28034
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawDebugPositionOverlay", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawDebugPositionOverlay': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Debug HUD overlay, gated on word_328C6 bits 0x80/0x200 clear, "
    "word_328CA bit 0x1000 clear, and word_328C4 bit 0x2000 set. "
    "Draws 3 rows of labeled numeric pairs: 'H'/'V' for "
    "word_2E55C/word_2E564, 'H'/'V' again for word_36CF7/word_36CF9 "
    "(confirmed party world X/Y), and 'B'/'F' for a pair read via a "
    "word_328D2-indexed table. Called from an unresolved raw address "
    "(seg000:0AFA), plausibly a low-level tick hook.",
    False,
)
