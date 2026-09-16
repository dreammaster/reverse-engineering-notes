"""
Names sub_1075E, called once from `start` right before a
TravelToDestination call gated on errorCode==0.

Checks the party's exact position against a hardcoded triple
(word_36CF7==0x200, word_36CF9==0x2B, word_36CF5==0x4000) -- a single
specific map/coordinate/flag combination. If matched, and if
word_328CA bit 0x2 is clear, shows a 2-line message "REGISTER TODAY!"
(confirmed via string dump at DS:0x8FD2) and sets errorCode=1, which
the caller uses to SKIP the TravelToDestination call -- i.e. this
blocks movement across a specific map boundary and nags the player to
register, the classic shareware "you've reached the edge of the demo
area" gate.

Confirmed dead in this binary: `start` unconditionally sets
word_328CA bit 0x2 right after ParseCommandLineSwitches
(`or word_328CA, 2`), and nothing anywhere in the binary ever clears
that bit again (checked: no `and word_328CA, immediate` masks out
bit 0x2). So the `jnz` at the top of this function always taken here
-- the boundary block and its nag message can never actually fire in
this build. This matches a second, related dead branch at the game's
exit sequence (`start`, shutdown path) which also tests the same bit
and, when clear, prints "Thank You for playing Yendorian Tales Book I
Chapter 2" / "Please register your copy today." via DOS INT21/AH=9 --
also unreachable here. -> EnforceDemoBoundary

Run via:
    .\run_ida_script.ps1 name_enforce_demo_boundary.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1075E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "EnforceDemoBoundary", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'EnforceDemoBoundary': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shareware demo boundary check: if the party is at the exact "
    "position (word_36CF7==0x200, word_36CF9==0x2B, "
    "word_36CF5==0x4000) and word_328CA bit 0x2 ('registered "
    "version' flag) is clear, shows 'REGISTER TODAY!' and sets "
    "errorCode=1 to block the caller's TravelToDestination. Dead in "
    "this binary: word_328CA bit 0x2 is unconditionally set at boot "
    "in `start` and never cleared anywhere, so this block can never "
    "fire. Called once from `start`.",
    False,
)
