"""
Names the 6 sex/race/class letter-code and display-name-pointer
tables in ultima_bootup.idb, resolving the roadmap's flagged
"exact letter/index encodings ... not yet individually decoded"
item. Confirmed programmatically via dump_char_creation_tables.py
(not hand-parsed from a raw hex dump):

  SEX_KEYS 'M','F','O' -> SEX_NAME_PTRS -> Male/Female/Other
  RACE_KEYS 'H','E','D','B','F' -> RACE_NAME_PTRS ->
      Human/Elf/Dwarf/Bobbit/Fuzzy
  CLASS_KEYS 'F','C','W','T','P','L','B','D','I','A','R' ->
      CLASS_NAME_PTRS -> Fighter/Cleric/Wizard/Thief/Paladin/Lark/
      Barbarian/Druid/Illusionist/Alchemist/Ranger

Confirmed via getMenuChoice's own code that RosterEntry._sex/._race/
._class store the raw ASCII KEY LETTER itself (e.g. 'M'/'F'/'O' for
sex), not a 0-based index: getMenuChoice matches the pressed key
against the KEYS array via `repne scasb` to compute an index only for
looking up the live preview name (`[bx+si]` into the PTRS array), but
the value it ultimately returns in AL (after an Enter/arrow-key
confirm loop) is the ORIGINAL uppercased key character, saved via
`push ax`/`pop ax` around the confirm wait untouched. showCharacterDetails
independently confirms this by re-doing the same `repne scasb` against
the KEYS array using the STORED RosterEntry byte as the search value
-- which only works if that byte IS the ASCII letter.
"""

import idc
import ida_bytes

DRY_RUN = False

# (keys_ea, key_count, ptrs_ea, keys_name, ptrs_name)
TABLES = [
    (0x11045, 3, 0x11048, "SEX_KEYS", "SEX_NAME_PTRS"),
    (0x1106B, 5, 0x11070, "RACE_KEYS", "RACE_NAME_PTRS"),
    (0x110D0, 11, 0x110DB, "CLASS_KEYS", "CLASS_NAME_PTRS"),
]


def define_bytes(ea, size, name):
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, size)
    ok = idc.create_data(ea, idc.FF_BYTE, size, idc.BADADDR)
    ok2 = idc.set_name(ea, name, idc.SN_NOWARN)
    return ok, ok2


def define_words(ea, count, name):
    size = count * 2
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, size)
    ok = idc.create_data(ea, idc.FF_WORD, size, idc.BADADDR)
    ok2 = idc.set_name(ea, name, idc.SN_NOWARN)
    return ok, ok2


def main():
    if DRY_RUN:
        print("[dry] would name 6 tables; set DRY_RUN=False to apply")
        return
    for keys_ea, count, ptrs_ea, keys_name, ptrs_name in TABLES:
        ok, ok2 = define_bytes(keys_ea, count, keys_name)
        print(f"{keys_ea:#x} {keys_name} ({count} bytes): data={ok} name={ok2}")
        ok, ok2 = define_words(ptrs_ea, count, ptrs_name)
        print(f"{ptrs_ea:#x} {ptrs_name} ({count} words): data={ok} name={ok2}")


if __name__ == "__main__":
    main()
