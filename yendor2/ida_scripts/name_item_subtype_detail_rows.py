"""
Names sub_13780 and sub_1385C -- per-subtype extra stat rows for the
clue book's F5 item subtypes 1 (ARMOR/RINGS) and 8 (WEAPONS), called
after ShowClueBookItemDetail's generic BASE VALUE/WEIGHT fields.

sub_13780 (-> ShowArmorDetailRow): "ABSORPTION-" (msg 0x8A96, confirmed
by dump -- matches ShowClueBookMonsterDetail's own "ABSORPTION-"
field) plus a value from word_2E548, then sub_149DD/sub_148EA (not
traced) and sub_14994 (shared with ShowWeaponDetailRow). Called from
RunClueBookItemCategory.

sub_1385C (-> ShowWeaponDetailRow): "DAMAGE:" (msg 0x7C81) plus a
value, then "2-HANDED:" (msg 0x8ADE) with "YES"/"NO" (msgs 0x8AE8/
0x8AEC) selected by a flag on the held item ([+2] bit 0), then
sub_14994. Called from RunClueBookWeaponCategory.

Run via:
    .\run_ida_script.ps1 name_item_subtype_detail_rows.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x13780: "ShowArmorDetailRow",
    0x1385C: "ShowWeaponDetailRow",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x13780,
    "F5 subtype 1 (ARMOR/RINGS) extra row: 'ABSORPTION-' plus a "
    "value. Called from RunClueBookItemCategory, after "
    "ShowClueBookItemDetail's generic fields.",
    False,
)
ida_bytes.set_cmt(
    0x1385C,
    "F5 subtype 8 (WEAPONS) extra row: 'DAMAGE:' plus a value, then "
    "'2-HANDED: YES/NO'. Called from RunClueBookWeaponCategory, after "
    "ShowClueBookItemDetail's generic fields.",
    False,
)
