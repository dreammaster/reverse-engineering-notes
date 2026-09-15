"""
CORRECTION: TryInteractAtPosition's comment (written very early this
session, before this area of code was understood) called sub_1766F
"a weight/capacity check". Wrong -- traced its body and it's a
lock-state loader.

sub_1766F(ax=1-based lock/object id): reads a bit-packed array from
CURGAME (FileEntry bx=0x8FFB, block 0x556C) -- (id-1)/8 selects the
byte, (id-1)%8 selects the bit, producing a test mask in word_32DC8.
Reads a second CURGAME block (0x556D) into an EMS-mapped buffer at a
fixed offset scaled by the id (id*0x1A). Finally splits word_32DD0
into word_32DC0 (quotient) / word_32DC2 (remainder) via /100.

Confirmed by TryInteractAtPosition's own call site: right after
calling sub_1766F it tests byte_32DCD and word_32DCE bits 0x20/0x40 --
the exact globals ShowLockStatus reads to choose between "NOT LOCKED"/
"LOCKED"/"MAGICALLY LOCKED"/etc. So this is loading a lock/door's
persisted state (has it been unlocked before, its key-tier
requirement, trap status) from CURGAME, not a capacity check.

-> LoadLockState

Corrects TryInteractAtPosition's stale comment to match.

Run via:
    .\run_ida_script.ps1 fix_lock_state_not_weight_check.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1766F
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "LoadLockState", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'LoadLockState': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "LoadLockState(ax=1-based lock/object id): reads a bit-packed "
    "'previously unlocked?' array from CURGAME (block 0x556C, "
    "(id-1)/8 byte + (id-1)%8 bit -> word_32DC8 mask), reads a second "
    "CURGAME block (0x556D) into an EMS buffer at offset id*0x1A, "
    "and splits word_32DD0 into word_32DC0/word_32DC2 via /100. Feeds "
    "word_32DCE and friends, which ShowLockStatus reads to choose its "
    "message.",
    False,
)

tip_ea = idc.get_name_ea_simple("TryInteractAtPosition")
print(f"TryInteractAtPosition @ {tip_ea:#x}")
new_cmt = (
    "Validates an interaction/move at (ax, bx) via FindObjectAtPosition. "
    "Nothing there -> errorCode=0. Something there -> branches on its "
    "type flags ([si+2]): LoadLockState (CORRECTED from a wrong "
    "'weight/capacity check' guess -- it loads a lock/door's persisted "
    "state from CURGAME, feeding ShowLockStatus's message choice), "
    "LoadCurgameRecord, or specific failure codes. Caller (`start`'s "
    "main loop) uses the resulting errorCode to decide whether to "
    "autosave to CURGAME."
)
ida_bytes.set_cmt(tip_ea, new_cmt, False)
print("TryInteractAtPosition comment corrected")
