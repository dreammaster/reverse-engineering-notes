"""
Names sub_2095A: the exact mirror of TickStatusEffects, confirming both
with high confidence. On word_32974 == 8/0xE/0xB, sets the
corresponding active-flag bit in word_36C79 (0x2000/0x800/0x400 --
exactly what TickStatusEffects clears) and increments the matching
duration counter (word_36C85/36C89/36C8B -- exactly what
TickStatusEffects decrements). Applying/extending a timed status
effect. -> ApplyStatusEffect

Run via:
    .\run_ida_script.ps1 name_apply_status_effect.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2095A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyStatusEffect", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyStatusEffect': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Applies/extends a timed status effect: on word_32974==8/0xE/0xB, "
    "sets the corresponding active flag (word_36C79) and increments "
    "the matching duration counter (word_36C85/36C89/36C8B) -- the "
    "exact mirror of TickStatusEffects, which decrements these and "
    "clears the flag on expiry.",
    False,
)
