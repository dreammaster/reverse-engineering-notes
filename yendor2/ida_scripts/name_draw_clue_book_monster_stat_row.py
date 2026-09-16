"""
Names sub_148B2, called 7 times from ShowClueBookMonsterDetail, once
per bestiary stat row (HP/AC/attacks/etc, each call site setting its
own _textPos_x/_textPos_y, bx=label message pointer, ax=field offset
within the loaded monster record).

Draws the label via writeString (bx already set by the caller), then
reinterprets the caller's ax as a field offset (popped into bx),
checks es:[bx] (the monster record field at that offset) for
zero -- if zero, skips the value draw entirely (stat not applicable
to this monster) -- else formats and draws the field's numeric value
right-aligned at a fixed column (x=0x113) via FormatNumber +
StripCommasAndSpaces + writeString. -> DrawClueBookMonsterStatRow

Run via:
    .\run_ida_script.ps1 name_draw_clue_book_monster_stat_row.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x148B2
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawClueBookMonsterStatRow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawClueBookMonsterStatRow': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws one bestiary stat row: label via writeString (caller sets "
    "bx=msg), then treats the caller's ax as a monster-record field "
    "offset (popped into bx) -- if es:[bx]==0, skips the value "
    "(stat not applicable), else draws es:[bx]'s value right-aligned "
    "at x=0x113. Called 7 times from ShowClueBookMonsterDetail, one "
    "per stat row.",
    False,
)
