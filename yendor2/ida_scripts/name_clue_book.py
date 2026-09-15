"""
Names sub_10C40: PollKeyboardInput calls it specifically when the
extended-key scan code equals 0x42 ('B') -- 0x42 is the standard BIOS
scan code for F8, and docs/manual.txt line 220 documents "F8 On-line
clue book" as a hotkey. sub_10C40 in turn calls ShowPagedEntryScreen
(icon + message + scroll arrows + fade, named earlier this session),
which fits a paginated hint/help-topic viewer well. -> ShowClueBook

This resolves much of ShowPagedEntryScreen's earlier "book/sign text vs.
a catalog" ambiguity: it's most likely the clue book's paginated entries,
not a spell/item catalog.

Run via:
    .\run_ida_script.ps1 name_clue_book.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x10C40
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowClueBook", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowClueBook': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Triggered by PollKeyboardInput on scan code 0x42 (F8) -- "
    "docs/manual.txt: 'F8 On-line clue book'. Calls ShowPagedEntryScreen "
    "to display the (paginated) clue book entries.",
    False,
)
