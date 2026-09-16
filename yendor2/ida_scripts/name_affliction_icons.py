"""
Names sub_22615, called from DrawPartyMemberStatusPanel: draws 4
affliction/status icons (category 0x90), a visual counterpart to the
text-based DrawAfflictionsList. Directly confirms and reuses the same
+0x1C affliction bit groups DrawAfflictionsList mapped by name:
- icon at [di+0xA]/[di+0xE]: DISEASED(0x2000)/POISONED(0x4000)/
  SICK(0x8000) -> icon ids 7/6/5, else 4 (none active)
- icon at [di+0x18]: STONED(0x400)/FROZEN(0x800)/PARALYZED(0x1000)
  -> icon ids 0xA/9/8, else 4
- icon at [di+0x22]: CURSED(0x80)/HEXED(0x100)/JINXED(0x200)
  -> icon ids 0xD/0xC/0xB, else 4
- icon at [di+0x32]/[di+0x36]: sums the 9 protection values at
  +0x20..+0x30 (DrawCharacterProtectionsList's exact fields) and draws
  icon 0xE if the total is nonzero (has some protection bonus), else
  icon 0 (word_2E530 stays 0x15 from its initial default... actually
  left at 0x15 when sum is 0, only overridden to 0xE when nonzero).
-> DrawAfflictionIconRow

Run via:
    .\run_ida_script.ps1 name_affliction_icons.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x22615
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawAfflictionIconRow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawAfflictionIconRow': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws 4 affliction/status icons (category 0x90), a visual "
    "counterpart to DrawAfflictionsList: DISEASED/POISONED/SICK "
    "(+0x1C 0x2000/0x4000/0x8000), STONED/FROZEN/PARALYZED (0x400/"
    "0x800/0x1000), CURSED/HEXED/JINXED (0x80/0x100/0x200), and a "
    "4th icon if the 9 protection values (+0x20..+0x30) sum to "
    "nonzero. Called from DrawPartyMemberStatusPanel.",
    False,
)
