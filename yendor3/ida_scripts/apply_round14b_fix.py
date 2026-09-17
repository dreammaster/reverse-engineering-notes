"""
Round 14 correction: round 14's rename of 0x2BBB5 to
'PollForEscapeKeyOnly' silently collided with an already-existing,
correctly-named function at a DIFFERENT address (0x15044, called from
sub_14F40) -- IDA auto-suffixed it to 'PollForEscapeKeyOnly_0' without
erroring, which went unnoticed until this follow-up check.

Checking the actual caller resolves which of the two yendor2
byte-identical duplicates it really is: yendor2's
`PlayCharacterCreationIntroAnimation` calls `PollForEscapeKeyOnlyAlt`,
not `PollForEscapeKeyOnly` -- and yendor3's
`PlayCharacterCreationIntroAnimation` calls this exact same address
(0x2BBB5) repeatedly. So 0x2BBB5 should have been named
`PollForEscapeKeyOnlyAlt` all along, not `PollForEscapeKeyOnly`. Fixed.

(The real `PollForEscapeKeyOnly`, at 0x15044, is unaffected --
untouched by round 14, correctly named from an earlier round.)

Run via:
    .\run_ida_script.ps1 apply_round14b_fix.py
"""
import idc
import ida_name

ea = 0x2BBB5
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PollForEscapeKeyOnlyAlt", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PollForEscapeKeyOnlyAlt': {'ok' if ok else 'FAILED'}")
