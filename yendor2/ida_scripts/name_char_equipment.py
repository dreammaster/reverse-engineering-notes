"""
Names sub_24A5B, ShowPartyMembers' second pipeline step: draws a 3x3
grid of pictures (9 cells, 0x21=33px apart, picture id incrementing by
2 per cell from a base derived from [si+0x10]) -- fits the manual's
equip-slot diagram (helmet/armor/gloves x2/rings x2/leggings/boots/
projectile/container/weapon/shield). [si+0x10] also gates a gender-
dependent message (compared against 2), tying it to the character's
sex field used elsewhere. The character equipment display.

-> ShowCharacterEquipment

Run via:
    .\run_ida_script.ps1 name_char_equipment.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x24A5B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowCharacterEquipment", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowCharacterEquipment': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "ShowPartyMembers' second pipeline step: draws a 3x3 grid of "
    "equipment-slot icons (DrawPicture, incrementing picture id by 2 "
    "per cell) -- fits the manual's equip-slot diagram. Also shows a "
    "gender-dependent message ([si+0x10] compared against 2). The "
    "character equipment display.",
    False,
)
