"""
Names sub_25456, called once from ShowCharacterSummary -- a
significant new find extending the confirmed party-record +0x1C
bitfield: its low 6 bits (0x1-0x20), previously unaccounted for
outside the already-fully-mapped affliction bits (0x40 and up), encode
which "secondary class" (ids 4-9 in the 27-class table: MONK,
ALCHEMIST, PALADIN, MAGE, DRUID, MARKSMAN) this character has reached.

Gated on [bx+0x94] being nonzero (an unconfirmed "eligible" marker,
bx=word_328D4). Maps the character's class id ([bx+0xE], 4 through 9)
to a single bit (0x20 down to 0x1 respectively) which is OR'd into
[bx+0x1C], and to a small per-class 2-entry table (fixed data at
0xD213/0xD217/0xD21B/0xD21F/0xD223/0xD227) of ability-flag indices,
each set via the already-named SetRecordFlag_CA (the +0xCA per-record
ability/skill flag bank) when nonzero. In short: marks the character's
secondary-class tier and grants up to 2 class-specific ability flags
alongside it -- plausibly a class-promotion/unlock effect, though
[+0x94]'s exact trigger condition isn't confirmed.
-> ApplySecondaryClassTierFlags

Run via:
    .\run_ida_script.ps1 name_apply_class_tier_flags.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25456
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplySecondaryClassTierFlags", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplySecondaryClassTierFlags': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Gated on [bx+0x94]!=0 (bx=word_328D4): maps class id [bx+0xE] "
    "(4-9) to a bit (0x20..0x1) OR'd into [bx+0x1C] -- new low bits "
    "of that field, distinct from the confirmed affliction bits -- "
    "and to a 2-entry table of ability-flag indices set via "
    "SetRecordFlag_CA. Called from ShowCharacterSummary.",
    False,
)
