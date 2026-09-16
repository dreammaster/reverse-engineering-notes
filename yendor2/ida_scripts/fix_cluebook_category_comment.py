"""
Corrects HandleClueCategorySelection's comment: guessed word_328CC bits
0x40/0x20 (gating the 'K'/'P' hotkeys) were "plausibly the same
registration-lock bits ShowClueBookRegistrationNag checks" -- wrong.
DrawClueBookNavBar's own pre-existing comment already documents those
exact bits as the "d) LIST" / "c) MAP" hotkey-hint toggles, not
registration locks (registration-lock is word_328CA bit 1 + an entry's
own +2 bit 0x8000, a completely different flag). Fixing before this
gets committed.

Run via:
    .\run_ida_script.ps1 fix_cluebook_category_comment.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x14D26,
    "RunClueEntryMenu's category-switching input handler: keyboard "
    "(ESC/digit keys/'K'/'P' hotkeys, byte_2E400==9 for "
    "ShowClueBookHelpScreen) and mouse (region table 0x6876, categories "
    "1-9) both feed into a shared 'apply new category' block that walks "
    "a 7-bit category mask in word_328CC and plays a sound cue on "
    "change. 'K'/'P' gated on word_328CC bits 0x40/0x20 -- the same "
    "'d) LIST'/'c) MAP' hotkey-hint bits DrawClueBookNavBar draws (NOT "
    "the registration-lock flag, which is a different bit).",
    False,
)
print("done")
