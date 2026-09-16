"""
Names sub_25D82 -- the single biggest cross-confirming find this
session. Called from sub_25B34. Draws "AFFLICTIONS:" then tests every
individual bit of the party record's +0x1C status word, drawing the
matching name (or "NONE" if +0x1C & 0xFF80 == 0):

  0x2000 DISEASED   0x4000 POISONED   0x8000 SICK
  0x400  STONED     0x800  FROZEN     0x1000 PARALYZED
  0x80   CURSED     0x100  HEXED      0x200  JINXED

This is the complete, definitive map of the 9 affliction bits in +0x1C,
resolving in one shot:
- TickStatusEffects/ApplyStatusEffect's 0x400/0x800/0x1000 group ->
  STONED/FROZEN/PARALYZED (the "timed" ailments).
- CastSpell's 0x18 dispel bits (0x2000/0x4000/0x8000) -> DISEASED/
  POISONED/SICK (the dispellable-only group).
- CheckPartyWipeAndReinitLevel's 0x1C40 incapacitation mask -> bit 6
  (DEAD) + STONED + FROZEN + PARALYZED -- literally "can't act."
- TickPartyAilmentIconBar's two groups -> DISEASED/POISONED/SICK
  (effect id 2, all characters) and CURSED/HEXED/JINXED (effect id 0xE,
  MP-gated characters only).
- DrawCharacterProtectionsList's 9 protection values at +0x20..+0x30
  now have their exact matching active-flag bits too.
-> DrawAfflictionsList

Run via:
    .\run_ida_script.ps1 name_afflictions_list.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25D82
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawAfflictionsList", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawAfflictionsList': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws 'AFFLICTIONS:' then every active +0x1C affliction bit by "
    "name: 0x2000 DISEASED, 0x4000 POISONED, 0x8000 SICK, 0x400 STONED, "
    "0x800 FROZEN, 0x1000 PARALYZED, 0x80 CURSED, 0x100 HEXED, 0x200 "
    "JINXED (or 'NONE' if none of 0xFF80 are set). The complete map of "
    "+0x1C's affliction bits. Called from sub_25B34.",
    False,
)
