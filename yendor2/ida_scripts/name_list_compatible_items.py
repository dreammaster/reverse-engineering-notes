"""
Names sub_1472A, called from RunClueBookItemCategory and
RunClueBookWeaponCategory (the F8 clue book's item/weapon category
handlers): loads the current entry's catalog record and checks its
usability flags ([+0xC] bits 0xC000/0xE00, plus word_2E548's [+2]
bits 0x800/0x100 -- target/class-eligibility-style flags used
elsewhere). If eligible, iterates up to 9 more catalog ids (incrementing
word_32974) re-checking the same eligibility test each time and calling
sub_147D8 (not traced) to draw each matching entry at a fixed position
(0x15, 0x88) -- building a filtered list of catalog items compatible
with the current weapon/armor category. Restores the original catalog
record before returning. -> ListCompatibleClueBookItems

Run via:
    .\run_ida_script.ps1 name_list_compatible_items.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1472A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ListCompatibleClueBookItems", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ListCompatibleClueBookItems': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Checks the current item's usability flags ([+0xC] bits 0xC000/"
    "0xE00, word_2E548[+2] bits 0x800/0x100); if eligible, iterates up "
    "to 9 more catalog ids re-checking eligibility and drawing each "
    "match (sub_147D8) -- a filtered compatible-items list for the F8 "
    "clue book's weapon/armor category view. Called from "
    "RunClueBookItemCategory and RunClueBookWeaponCategory.",
    False,
)
