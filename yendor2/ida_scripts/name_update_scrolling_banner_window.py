"""
Names sub_1FC53, called from PlayStudioCreditsIntro (right after
`mov word_36D01, 1E0h` in the credits sequence) and from ShowClueBook.

Reads word_36D01 as a tick/time value and computes a scroll offset
(word_36D91) into a fixed source table at 0x4A5C: outside the range
0x167-0x4AA, no offset change; within 0x167-0x1D9 (an entry
transition zone) or 0x438-0x4AA (an exit transition zone), computes a
partial offset scaled by how far into the zone word_36D01 is (and
sets word_3295A bit 0x2000, a fade/transition flag, plus
word_32950); within the steady middle range 0x1D9-0x438, the offset
stays fixed at whatever the entry zone last computed. Copies a fixed
0x30-word (96-byte) window from the computed source offset through a
scratch buffer at 0x9535 to a final destination at 0x46CA. Reads as
scrolling a fixed-size window of a longer table/banner into view over
time, with distinct fade-in/fade-out edge zones -- most likely the
mechanism behind the credits sequence's scrolling text/graphics; the
exact nature of the 0x4A5C source table's content (text vs. graphics)
not independently confirmed. -> UpdateScrollingBannerWindow

Run via:
    .\run_ida_script.ps1 name_update_scrolling_banner_window.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1FC53
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UpdateScrollingBannerWindow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UpdateScrollingBannerWindow': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Computes a scroll offset into a fixed source table (0x4A5C) "
    "from the tick value word_36D01, with distinct entry/steady/exit "
    "zones (0x167-0x1D9/0x1D9-0x438/0x438-0x4AA), then copies a "
    "fixed 0x30-word window through a scratch buffer (0x9535) to a "
    "destination at 0x46CA. Called from PlayStudioCreditsIntro and "
    "ShowClueBook.",
    False,
)
