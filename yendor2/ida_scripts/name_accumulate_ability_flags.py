"""
Names sub_29040, called from UseItem and UseAbilityScroll -- scans a
catalog table and accumulates flags for entries the current character
qualifies for via their learned special abilities.

Resolves the character (word_32924 id -> SelectPartyRecordById) and
reads their learned-abilities bitmask ([+0xB4], the confirmed
"bitmask of which of at least 4 special abilities this character has
learned" field). Then walks a table at word_2E54C (0x3A-byte stride,
word_2E432 entries, identity/purpose not otherwise traced) accumulating
two flag words (word_2E40C, word_2E40E, both zeroed first) by ORing in
each qualifying entry's [+0x16]/[+0x18] fields. An entry qualifies if
its [+0x10] bit 1 is set (always applies), or if its [+0x0E] bit 8 is
set AND its [+0x12] ability-bitmask overlaps the character's learned
abilities.

In short: "given what special abilities this character knows,
accumulate the flags contributed by every catalog entry they
qualify for" -- plausibly determining which item-use options or
interaction flags are currently unlocked. The word_2E54C table's own
identity isn't confirmed. -> AccumulateLearnedAbilityFlags

Run via:
    .\run_ida_script.ps1 name_accumulate_ability_flags.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x29040
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "AccumulateLearnedAbilityFlags", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'AccumulateLearnedAbilityFlags': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "For the character selected via word_32924, walks a table at "
    "word_2E54C (stride 0x3A, count word_2E432) and ORs each "
    "qualifying entry's [+0x16]/[+0x18] into word_2E40C/word_2E40E. "
    "An entry qualifies if [+0x10] bit 1 is set, or [+0x0E] bit 8 is "
    "set and [+0x12] overlaps the character's learned-abilities "
    "bitmask ([+0xB4]). Called from UseItem and UseAbilityScroll.",
    False,
)
