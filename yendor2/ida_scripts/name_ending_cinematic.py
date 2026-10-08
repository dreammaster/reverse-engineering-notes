"""
PlayStudioCreditsIntro is not a studio credits sequence: it is Chapter 2's ending cinematic (the parchment found in the Quartz Chamber, the
letter from Paltivar's followers, the arrival at Yendor, the audience with Zamora, and a "Coming soon" teaser of Book II). Reached from `start`
when global flags 0x69-0x6B are set (src23/introstory.c, introEndingScript). The two helpers are renamed with it.

Run via:
    .\run_ida_script.ps1 name_ending_cinematic.py -NoExport
"""
import idc
import ida_name

RENAMES = {
    "PlayStudioCreditsIntro": "PlayChapterEndingCinematic",
    "PlayCreditsWipeAnimation": "PlayEndingScrollWipeAnimation",   # picture 8 of category 0 slides up over picture 7, 65 steps
    "PlayCreditsFrameAnimation": "PlayEndingFigureAnimation",      # 14 pictures (category 6, 0x22-0x2F), 2 ticks each
}

for old, new in RENAMES.items():
    ea = idc.get_name_ea_simple(old)
    if ea == idc.BADADDR:
        print(f"{old}: not found")
        continue
    ok = ida_name.set_name(ea, new, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {new!r}: {'ok' if ok else 'FAILED'}")
