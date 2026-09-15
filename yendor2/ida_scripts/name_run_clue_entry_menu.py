"""
Names sub_12B84, the clue book's interactive per-category entry menu
(all 13 call sites are inside ShowClueBook). PollKeyboardInput-driven
loop: mouse clicks are hit-tested against the entry-list region table
(0x68D2, same one DrawClueEntryList positions entries from) to select
an entry (word_2E3EE) and refresh the display
(ShowClueCategoryEntries); Enter/Space on an unread entry (status bit
0x8000 clear) calls sub_14AE8 (not traced, plausibly "show full clue
detail" / "mark as read").

-> RunClueEntryMenu

Run via:
    .\run_ida_script.ps1 name_run_clue_entry_menu.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12B84
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunClueEntryMenu", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunClueEntryMenu': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Interactive per-category clue-book entry menu (PollKeyboardInput "
    "loop). Mouse clicks hit-test against the entry-list region table "
    "(0x68D2) to select an entry and refresh via "
    "ShowClueCategoryEntries. Enter/Space on an unread entry (status "
    "bit 0x8000 clear) calls sub_14AE8 (not traced, plausibly show "
    "full detail / mark as read).",
    False,
)
