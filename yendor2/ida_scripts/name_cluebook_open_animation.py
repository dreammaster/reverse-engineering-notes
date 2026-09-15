"""
Traced sub_15142, called once from ShowClueBook's init sequence. A
one-time (word_328CC bit 0x10 latch) intro animation: swaps in a
special palette (LoadMasterPalette offset 0x442A), draws g_pictureDir
entry 0xC, plays two short timed sound cues with waits (~30 and ~35
ticks), then restores the normal palette. Skipped entirely if
word_328C4 bit 0x8000 is set (plausibly "in combat, skip decorative
intro") or if already shown this session. Always ends by drawing a
border/frame picture (entry 0x1D).

-> PlayClueBookOpenAnimation

Run via:
    .\run_ida_script.ps1 name_cluebook_open_animation.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x15142
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PlayClueBookOpenAnimation", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PlayClueBookOpenAnimation': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "One-time (word_328CC bit 0x10 latch) clue-book intro animation: "
    "swaps in a special palette, draws g_pictureDir entry 0xC, plays "
    "two timed sound cues, restores the normal palette. Skipped if "
    "word_328C4 bit 0x8000 is set or already shown. Always ends by "
    "drawing a border/frame picture (entry 0x1D).",
    False,
)
