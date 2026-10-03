"""Create the Character struct (docs/character.h) in the database.  Safe to re-run (recreates it)."""
import ida_bytes, ida_struct, idc

FIELDS = [
    (0x00, "name", 16), (0x10, "sex", 1), (0x11, "race", 1), (0x12, "alignment", 1), (0x13, "charClass", 1),
    (0x14, "might", 2), (0x16, "intellect", 2), (0x18, "personality", 2), (0x1A, "endurance", 2), (0x1C, "speed", 2),
    (0x1E, "accuracy", 2), (0x20, "luck", 2), (0x22, "acTemp", 1), (0x23, "level", 1), (0x24, "tempLevel", 1),
    (0x25, "birthDay", 1), (0x26, "tempAge", 1), (0x27, "skills", 18), (0x39, "awards", 26), (0x53, "spells", 36),
    (0x77, "lloydMap", 1), (0x78, "lloydX", 1), (0x79, "lloydY", 1), (0x7A, "hasSpells", 1), (0x7B, "currentSpell", 1),
    (0x7C, "quickOption", 1), (0x7D, "slotPresent", 19), (0x90, "slotFlags", 19), (0xA3, "slotElement", 19),
    (0xB6, "slotMetal", 19), (0xC9, "slotAttribute", 19), (0xDC, "slotId", 19), (0xEF, "unknownEF", 19),
    (0x102, "acBonus", 1), (0x103, "unknown103", 16), (0x113, "conditions", 16), (0x123, "unknown123", 2),
    (0x125, "hp", 2), (0x127, "sp", 2), (0x129, "birthYear", 2), (0x12B, "experience", 4),
]
sid = ida_struct.get_struc_id("Character")
if sid != idc.BADADDR:
    ida_struct.del_struc(ida_struct.get_struc(sid))
sid = ida_struct.add_struc(idc.BADADDR, "Character", False)
sp = ida_struct.get_struc(sid)
for off, name, size in FIELDS:
    flag = ida_bytes.byte_flag()
    if size == 2 and name not in ("might", "intellect", "personality", "endurance", "speed", "accuracy", "luck"):
        flag = ida_bytes.word_flag()
    elif size == 4:
        flag = ida_bytes.dword_flag()
    if flag == ida_bytes.word_flag() or flag == ida_bytes.dword_flag():
        ida_struct.add_struc_member(sp, name, off, flag, None, size)
    else:
        ida_struct.add_struc_member(sp, name, off, flag, None, size)
print("Character struct created, size", ida_struct.get_struc_size(sp))
