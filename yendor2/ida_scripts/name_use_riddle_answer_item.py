"""
Names sub_1A5F6, called once from UseItem -- a genuinely new item
mechanic: a riddle/password item requiring the player to type the
correct answer.

Looks up a tier value (ax, checked against _val40) in a table at
0xBF48 to get word_32904 (an id/pointer for the expected answer),
draws DrawItemUseConfirmDialog plus an optional wrapped description
(via ComputeCostMessageIndentMode + DrawIndentedTextColumn if the
item has description text), then opens a 34-character text entry
field (EditTextField) for the player to type an answer, with "PRESS
ESCAPE TO EXIT" shown as a standing hint (confirmed via string dump
at 0x8171). Compares the typed text's length and content
byte-for-byte against the expected answer string. On an exact match,
shows "THAT SOUNDS GOOD TO ME." (0x841C) and sets word_328C6 bit
0x40; on a mismatch, shows "THAT IS INCORRECT." (0x8409); either way
loops back to prompt again unless the player pressed ESC.
-> UseRiddleAnswerItem

Run via:
    .\run_ida_script.ps1 name_use_riddle_answer_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1A5F6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseRiddleAnswerItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseRiddleAnswerItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Riddle/password item: opens a text-entry field for the player "
    "to type an answer, comparing it byte-for-byte against an "
    "expected string looked up via the item's tier value. Confirmed "
    "via string dump: 'THAT SOUNDS GOOD TO ME.' on a match, 'THAT IS "
    "INCORRECT.' on a mismatch. Called once from UseItem.",
    False,
)
