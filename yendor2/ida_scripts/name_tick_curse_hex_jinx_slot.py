"""
Names sub_1A195, called once from TickPartyAilmentIconBar -- this is
the concrete "id 0xE" helper already described in file-formats.md
("0x80/0x100/0x200 = CURSED/HEXED/JINXED, gated on having MP") but not
yet individually pinned down to an address.

bx=word_328D4 (current party member). No-ops if [bx+0x1C] & 0x1C40
(the confirmed incapacitation mask) is nonzero, or if [bx+0x54] (MP)
is 0. Otherwise adds a weighted severity to [di+0x10] for each active
affliction: CURSED (0x80) +0x10, HEXED (0x100) +8, JINXED (0x200) +4.
If the total is nonzero, fills the icon-bar slot the same way the
sibling helpers do ([di+8]/[di+0xA]=word_3293E/word_32940,
[di+0xC]=bx) and sets word_328CA bit 0x100.
-> TickCurseHexJinxAilmentSlot

Run via:
    .\run_ida_script.ps1 name_tick_curse_hex_jinx_slot.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1A195
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TickCurseHexJinxAilmentSlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TickCurseHexJinxAilmentSlot': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "No-op if incapacitated ([bx+0x1C]&0x1C40) or out of MP "
    "([bx+0x54]==0). Adds weighted severity to [di+0x10]: CURSED+0x10, "
    "HEXED+8, JINXED+4. If nonzero, fills icon-bar slot di and sets "
    "word_328CA bit 0x100. Called from TickPartyAilmentIconBar.",
    False,
)
