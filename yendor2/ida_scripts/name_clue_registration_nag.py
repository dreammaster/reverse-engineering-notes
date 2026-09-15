"""
Traced sub_14AE8 (RunClueEntryMenu's "selected an unavailable entry"
callee) and dumped its message: "REGISTER YOUR COPY OF THE CLUE BOOK
TODAY!" -- a shareware registration nag. Confirms the clue book has
registration-locked entries: RunClueEntryMenu only calls this when
the global "registered" flag (word_328CA bit 1) is clear AND the
selected entry's own flag ([+2] bit 0x8000) says it requires
registration.

-> ShowClueBookRegistrationNag

Run via:
    .\run_ida_script.ps1 name_clue_registration_nag.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14AE8
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowClueBookRegistrationNag", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowClueBookRegistrationNag': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Plays a sound and shows 'REGISTER YOUR COPY OF THE CLUE BOOK "
    "TODAY!' -- the shareware registration nag for clue-book entries "
    "that require registration (called when the global 'registered' "
    "flag, word_328CA bit 1, is clear and the entry's own bit 0x8000 "
    "says it's registration-locked).",
    False,
)
