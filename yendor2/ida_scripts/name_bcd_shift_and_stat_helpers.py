"""
Names 4 more small helpers.

sub_19DA3/sub_19DCF (called only from MulBCD4ByWord): internal
one-nibble (4-bit) shift helpers on the 4-byte packed-BCD buffer at
si. sub_19DA3 shifts left, sub_19DCF shifts right.
-> ShiftBCD4LeftNibble / ShiftBCD4RightNibble

sub_234A7 (called from ProcessLevelMonsters and CompactMonsterSlots):
gated on word_32A1E (the confirmed active-combat-monster pointer)
being nonzero. Searches all 3 g_monsterSlots entries for the one
matching the old word_32A1E address (after it's been cleared to 0)
and restores word_32A1E to wherever that record ended up -- fixes up
the active-monster pointer after CompactMonsterSlots moves records
around. -> RelocateActiveMonsterPointer

sub_2A982 (called from ApplyMultiStatEffect): clamps ax to a max --
0x270F (9999) for offsets 0x52/0x92/0x54/0x94 (the confirmed max-HP/
max-MP-family fields), else 0x3E7 (999) -- then stores it at
[bx+si]. A generic "clamp a stat value, higher cap for HP/MP-type
fields" helper. -> ClampStatEffectValue

Run via:
    .\run_ida_script.ps1 name_bcd_shift_and_stat_helpers.py
"""
import idc
import ida_name
import ida_bytes

entries = [
    (0x19DA3, "ShiftBCD4LeftNibble",
     "Shifts the 4-byte packed-BCD buffer at si left by one nibble. "
     "Called only from MulBCD4ByWord."),
    (0x19DCF, "ShiftBCD4RightNibble",
     "Shifts the 4-byte packed-BCD buffer at si right by one nibble. "
     "Called only from MulBCD4ByWord."),
    (0x234A7, "RelocateActiveMonsterPointer",
     "Re-finds word_32A1E (active-combat-monster pointer) among the "
     "3 g_monsterSlots entries after it's moved, fixing up the "
     "pointer. Called from ProcessLevelMonsters and "
     "CompactMonsterSlots."),
    (0x2A982, "ClampStatEffectValue",
     "Clamps ax to 9999 for HP/MP-family field offsets "
     "(0x52/0x92/0x54/0x94), else 999, then stores it at [bx+si]. "
     "Called from ApplyMultiStatEffect."),
]

for ea, name, desc in entries:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
