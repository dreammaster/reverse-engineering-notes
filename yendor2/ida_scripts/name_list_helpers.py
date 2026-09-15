"""
Names two small helpers found while following the scratch-buffer
(0xAFA8) lead from the string-utility work:

- sub_1F1B8: word_3291E (a 1-based list-row index) * 10, indexed into a
  table at 0x5CD0 (10 bytes/entry); reads two words from the entry and
  derives _textPos_x/_textPos_y from them (+0xC and +1 respectively).
  -> GetListItemPosition
- sub_1F197: StrFillN(bx=0xAFA8, ah=0x19, al=' ') to blank the 25-byte
  scratch label buffer, then writeString(bx=0xAFA8) at the stored
  position -- i.e. blanks it then "displays" the now-empty buffer,
  which is really just erasing whatever label text was there before.
  -> EraseLabelText

Both are called from the large sub_1EA6E (1631 bytes, called from
`start` and InitGame directly, draws GameDialog_drawButtons and touches
all four FileEntry ops) -- that function itself is too large/multi-
purpose to confidently name yet, but looks like a major top-level screen
(main menu or HUD) with a list/label rendering subsystem inside it.

Run via:
    .\run_ida_script.ps1 name_list_helpers.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1F1B8: "GetListItemPosition",
    0x1F197: "EraseLabelText",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1F1B8,
    "Looks up list row word_3291E (1-based) in a 10-byte-per-entry table "
    "at 0x5CD0 and sets _textPos_x/_textPos_y from it (+0xC / +1). Used "
    "to position a label for the currently-selected list row.",
    False,
)
ida_bytes.set_cmt(
    0x1F197,
    "Blanks the 25-byte scratch label buffer (0xAFA8) via StrFillN, then "
    "writeString's it at the stored position -- erases whatever label "
    "text was previously drawn there.",
    False,
)
