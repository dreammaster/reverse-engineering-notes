"""
Traced sub_12C96, BuildLoadValidationMessage's caller -- a scrollable
save-slot list display, completing this round's save/load-menu thread.

Draws word_2E3EC entries at positions from a table (0x68D2, stride
0xA), color-coding the currently-selected one (word_2E3EE) differently
(further modified by a couple of flag bits). For each entry, calls
BuildLoadValidationMessage to get its status/validation text and
draws it if present. Afterward shows scroll indicators ("more above"/
"more below"-style markers via aAMoreB/byte_36755) when the list has
entries beyond the visible window (word_2E3EA count vs. word_2E3F0/
word_2E3F2 scroll position).

-> DrawSaveSlotList

Run via:
    .\run_ida_script.ps1 name_draw_save_slot_list.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12C96
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawSaveSlotList", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawSaveSlotList': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws a scrollable save-slot list: word_2E3EC entries at "
    "positions from a table (0x68D2, stride 0xA), highlighting the "
    "selected one (word_2E3EE). Each entry's status/validation text "
    "comes from BuildLoadValidationMessage. Shows scroll indicators "
    "when the list extends beyond the visible window.",
    False,
)
