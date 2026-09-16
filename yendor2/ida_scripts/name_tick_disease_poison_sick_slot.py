"""
Names sub_1A14D, called once from TickPartyAilmentIconBar -- the
concrete "id 2" helper already described in file-formats.md
(DISEASED/POISONED/SICK, the normal path), completing the full
3-helper TickPartyAilmentIconBar cluster this round (alongside
TickCurseHexJinxAilmentSlot and TickPerceptionGatedAilmentSlot).

bx=word_328D4 (current party member). No-ops if [bx+0x1C] & 0x1C40
(incapacitation mask) is nonzero. Otherwise sums a weighted severity
into [di+0x10]: DISEASED (0x2000) +0xC, POISONED (0x4000) +6, SICK
(0x8000) +3. If nonzero, fills the icon-bar slot the same way its two
siblings do ([di+8]/[di+0xA]=word_3293E/word_32940, [di+0xC]=bx) and
sets word_328CA bit 0x100. -> TickDiseasePoisonSickAilmentSlot

Run via:
    .\run_ida_script.ps1 name_tick_disease_poison_sick_slot.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1A14D
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TickDiseasePoisonSickAilmentSlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TickDiseasePoisonSickAilmentSlot': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "No-op if incapacitated ([bx+0x1C]&0x1C40). Adds weighted "
    "severity to [di+0x10]: DISEASED+0xC, POISONED+6, SICK+3. If "
    "nonzero, fills icon-bar slot di and sets word_328CA bit 0x100. "
    "Called from TickPartyAilmentIconBar.",
    False,
)
