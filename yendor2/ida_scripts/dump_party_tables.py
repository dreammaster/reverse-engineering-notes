"""
Read-only: dumps the two string tables that define the party record's
field meanings, for docs23/file-formats.md's party-record table.

  1. The 27-entry attribute/skill name table at DS:0x7DC7 (13-byte stride).
     Entry i names the record field at +0x3C+2i (current) / +0x7C+2i (max),
     per UseAttributeBoostItem and ShowArmorAttributeBonusList.
  2. The three 9-entry class-name tables (11-byte stride) that
     GetClassNameString indexes as tier*10 + base.

    .\run_ida_script.ps1 dump_party_tables.py -NoExport
"""
import idc

DS_BASE = 0x2D860


def text(offset, size):
    raw = bytes(idc.get_bytes(DS_BASE + offset, size))
    return raw.split(b"\0")[0].decode("latin1").rstrip()


print("STAT TABLE @0x7DC7:")
for i in range(27):
    print("  idx %2d current=+0x%02x max=+0x%02x name=%r" % (i, 0x3C + 2 * i, 0x7C + 2 * i, text(0x7DC7 + i * 13, 13)))

for tier, base in enumerate((0x7982, 0x8434, 0x8497)):
    names = [text(base + i * 11, 11) for i in range(9)]
    print("CLASS tier %d (ids %d-%d): %s" % (tier, tier * 10 + 1, tier * 10 + 9, names))
