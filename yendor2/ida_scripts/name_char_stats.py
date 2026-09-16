"""
Names sub_24BF2, another ShowPartyMembers pipeline step: after some
prep calls (sub_252EF, sub_23F58, sub_1B30C, sub_254CC -- presumably
computing/formatting stat values), draws a header message then two text
blocks via sub_23AF2 (cx=1 line, then cx=6 lines) -- 6 lines matches
the docs' "6 core attributes" (STRENGTH, DEXTERITY, STAMINA,
INTELLIGENCE, WISDOM, CHARISMA) exactly. The character stats display.

-> ShowCharacterStats

Run via:
    .\run_ida_script.ps1 name_char_stats.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x24BF2
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowCharacterStats", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowCharacterStats': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "ShowPartyMembers pipeline step: draws a header then 6 lines of "
    "text via sub_23AF2 -- matches the 6 core attributes (STRENGTH/"
    "DEXTERITY/STAMINA/INTELLIGENCE/WISDOM/CHARISMA) from the manual "
    "exactly. The character stats display.",
    False,
)
