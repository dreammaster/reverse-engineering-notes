"""
Names sub_14D26, called repeatedly from RunClueEntryMenu (the F8 clue
book's per-category menu loop): handles category-switching input.

Keyboard: ESC (0x1B) and digit keys ';'-'@' (mapped to word_2E40A =
category 2-9) fall through to a shared "apply new category" block
(loc_14DC7); 'K' and 'P' are hotkeys gated on word_328CC bits 0x40/
0x20 (plausibly the same registration-lock bits `RunClueEntryMenu`
already shows `ShowClueBookRegistrationNag` for); byte_2E400==9 (Tab?)
calls `ShowClueBookHelpScreen` directly. Mouse: hit-tests region table
0x6876 for a category 1-9, with categories 1 and 9 specially gated the
same way as their keyboard equivalents. The shared "apply" block walks
a 7-bit category mask in word_328CC (bits 0x8000..0x80, categories
2-8), and when the newly selected category's bit isn't already set,
marks it and plays a sound cue (ax=1) via the sound dispatch
(sub_28412). -> HandleClueCategorySelection

Run via:
    .\run_ida_script.ps1 name_cluebook_category_select.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14D26
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleClueCategorySelection", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleClueCategorySelection': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "RunClueEntryMenu's category-switching input handler: keyboard "
    "(ESC/digit keys/'K'/'P' hotkeys, byte_2E400==9 for "
    "ShowClueBookHelpScreen) and mouse (region table 0x6876, categories "
    "1-9) both feed into a shared 'apply new category' block that walks "
    "a 7-bit category mask in word_328CC and plays a sound cue on "
    "change. 'K'/'P' gated on word_328CC bits 0x40/0x20, plausibly the "
    "same registration-lock bits ShowClueBookRegistrationNag checks.",
    False,
)
