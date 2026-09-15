"""
Traced sub_12E59, DrawSaveSlotList's caller -- the save-slot menu's
init+draw step.

On first call (word_2E3EA==0), reads a per-category slot count from a
table at 0xF3F4 (indexed by word_2E3F6, the same "category" selector
BuildLoadValidationMessage/DrawSaveSlotList use), initializes the
scroll/selection state (word_2E3EE/word_2E3F0/word_2E3F2/word_2E3F4),
and calls sub_12FB0 (not traced). Caps the visible list at 14 entries
(word_2E3EC). Every call then draws the message-box frame
(DrawMessageBox, already named) plus header/footer text (sub_14C37,
sub_1303C, not traced) and the slot list itself (DrawSaveSlotList).

-> ShowSaveSlotMenu

Run via:
    .\run_ida_script.ps1 name_show_save_slot_menu.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12E59
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowSaveSlotMenu", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowSaveSlotMenu': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Save-slot menu init+draw. On first call, reads a per-category "
    "slot count (0xF3F4, indexed by word_2E3F6) and initializes "
    "scroll/selection state, capping the visible list at 14 entries. "
    "Every call draws the frame (DrawMessageBox) plus header/footer "
    "(sub_14C37/sub_1303C, not traced) and the slot list "
    "(DrawSaveSlotList).",
    False,
)
