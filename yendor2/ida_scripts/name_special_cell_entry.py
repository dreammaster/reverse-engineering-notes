"""
Names sub_116F3, moderate confidence: called from HandleMovementInput
when the player's destination cell's type value falls in the
[_val32, _val31] range -- the same range-check pattern
IsMonsterStepBlocked uses for its "special" cell branch (exact
semantics of that range still unconfirmed there too).

Two paths: if byte_2E400 == 'H' (0x48 -- not one of the manual's
documented hotkeys; may be a real but unsurveyed key, or an internal
sentinel value rather than a literal keypress), it looks up the
destination cell's type as an index into the 0xE551 tile-type table
(already documented: 12-byte stride, +0xA = g_pictureDir byte),
copying several of its fields plus a second, fixed-index (0x6EB8) entry
from the same table into a scratch struct at 0xE821, then calls
RefreshDungeonScreen and plays a sound. Otherwise, it just plays a
different sound (_val30) via the sound dispatch. Both paths fall
through to the shared movement-apply code (advancing word_36CF7/
word_36CF9, the player's position, and calling RevealCellsAroundPlayer).
Reads as handling entry onto a "special" (trap-door/stairs-like?) cell
type, with the 'H' path doing extra tile-data setup + a screen refresh
-- but the exact nature of the special cell type and the 'H' condition
aren't confirmed. -> HandleSpecialCellEntry

Run via:
    .\run_ida_script.ps1 name_special_cell_entry.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x116F3
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleSpecialCellEntry", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleSpecialCellEntry': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Called from HandleMovementInput when the destination cell's type "
    "is in the [_val32,_val31] range (same range-check shape as "
    "IsMonsterStepBlocked's 'special' branch). If byte_2E400=='H', "
    "looks up 0xE551 tile-type table entries (destination cell's type, "
    "plus a fixed index 0x6EB8) into scratch 0xE821, then "
    "RefreshDungeonScreen + a sound; otherwise just a different sound. "
    "Both paths fall through to the shared movement-apply code. Exact "
    "nature of the special cell type and the 'H' condition not "
    "confirmed.",
    False,
)
