"""
IDA Pro script: master list of struct-definition updates (new members,
renames, retypes) for Ultima II (DOS).

Companion to apply_renames.py, same rationale: one accumulating,
git-trackable file instead of a new one-off script per finding. Add an
entry to OPERATIONS below whenever a struct field's meaning becomes
clear, then re-run. Idempotent: each operation checks current state and
skips if already applied.

Convention: DRY_RUN is left False (Paul's call, 2026-08-17, same as
apply_renames.py) -- new entries take effect the moment they're added
and the script is re-run. Sanity-check a new entry's offset/name before
adding it, since there's no dry-run safety net here anymore.

Scope: operations on IDA *struct* definitions (`Savegame`, `FCB`, and
any future ones) via add_struc_member / set_member_name -- i.e. things
that show up as `player.field_2B` -> `player._something` in the .asm.
This is a different IDA API surface from apply_renames.py's
idc.set_name (which addresses one specific linear address, not a
struct-relative offset), hence the split.

Handles array-typed members (2026-08-17): a struct member can be an
N-element byte/word/dword array, not just a scalar -- e.g.
`player.field_60[di]` where di ranges 0-6 is really a 7-byte array, not
the 1-byte scalar it was originally auto-created as (a single indexed
access is enough for IDA to render `name[reg]` syntax even when the
"official" declared size is too small, so this kind of undersizing is
easy to miss until you've actually traced the index range). Two ops
cover this:
  - "add_member": for an offset with NO existing member yet. Supports
    element_size (1/2/4, byte/word/dword) and count (defaults to 1 for
    a plain scalar); total member size is element_size * count.
  - "resize_member": for an offset that already has a member, but at
    the wrong size (or default IDA-auto-generated name) -- deletes and
    recreates it via del_struc_member + add_struc_member. This is a
    superset of add_member's job (it also handles the "nothing there
    yet" case fine, del_struc_member is skipped when there's nothing to
    delete) but kept as a separate op so OPERATIONS reads clearly: a
    "resize_member" entry signals there was a pre-existing conflicting
    member at that offset, which is worth knowing at a glance.

Does NOT cover: struct-of-arrays-style globals. `_mapMonsters` (the
per-map monster table, loaded from MONXFF) is accessed as parallel byte
arrays -- `_mapMonsters[di]`, `(_mapMonsters+20h)[di]`,
`(_mapMonsters+40h)[di]`, ... one byte per monster slot (0-31) per
field, not one struct per monster -- so it doesn't fit an IDA struct
type the way `player` (a single Savegame instance) does, even with the
array-member support above (that's for one array *inside* a single
struct instance, not many parallel arrays standing in for an
array-of-structs). Naming those sub-arrays is still a rename-once-split
problem: the field boundaries (`_mapMonsters+20h/40h/60h/80h`, and
further unnamed ones out past `_mapMonsters+137h` used in the
monster-AI loop and in `transact`'s shopkeeper branch, e.g. `[di+197h]`
= monster type/class byte, `[di+1D7h]` = shopkeeper item-index flag)
are real findings worth tracking, but need a bespoke script
(ida_bytes.create_data to split the array, then set_name on each piece)
similar in shape to resolve_command_jump_table.py -- not yet written.
Note this here so the lead isn't lost; write that script once the
remaining field offsets (0x177, 0x1B7, 0x1F7, 0x217, and whatever's
between 0x80 and 0x137) are better understood.

Each OPERATIONS entry is a dict:
  {"op": "add_member", "struct": name, "member": name, "offset": int,
   "element_size": 1|2|4, "count": int (default 1), "note": str}
  {"op": "rename_member", "struct": name, "offset": int, "new_name": str,
   "note": str}
  {"op": "resize_member", "struct": name, "offset": int, "new_name": str,
   "element_size": 1|2|4, "count": int, "note": str}
"""

import idc
import ida_struct

DRY_RUN = False

OPERATIONS = [
    {"op": "rename_member", "struct": "Savegame", "offset": 0x2E,
     "new_name": "_torches",
     "note": "checked/decremented by ignite_torch (\"NONE OWNED!\" if "
             "zero, asm ~9629-9647); +1..4 (BCD) dropped by most "
             "monster kills in attack (asm ~8199-8209). See "
             "docs/file-formats.md#monx--monsternpc-data."},

    {"op": "rename_member", "struct": "Savegame", "offset": 0x2F,
     "new_name": "_keys",
     "note": "checked/decremented by unlock (\"NO KEYS THAT FIT!\" if "
             "zero, asm ~10884-10908); +2 (BCD) dropped by killing a "
             "Guard (TILE_GUARD, monster type 0x60) in attack (asm "
             "~8134-8143). See docs/file-formats.md#monx--monsternpc-data."},

    {"op": "rename_member", "struct": "Savegame", "offset": 0x30,
     "new_name": "_thievesTools",
     "note": "saves you from a trap death (\"ESCAPED! BY USE OF "
             "TOOLS!\", decremented on use, asm ~7599-7631) -- die "
             "instead if zero; +1 (BCD) dropped by killing a Thief "
             "(TILE_THIEF, monster type 0xFC) in attack (asm "
             "~8146-8177). See docs/file-formats.md#monx--monsternpc-data."},

    # -- 4 fields decoded via the TEXT_STRINGS (cs:4C60h) table Paul
    # formatted directly in IDA -- cross-checked each command's write
    # site against zstats' matching read formula, see
    # docs/overview.md#text_strings--the-cs4c60h-table-fully-decoded --

    {"op": "rename_member", "struct": "Savegame", "offset": 0x2B,
     "new_name": "_readiedWeapon",
     "note": "0=unarmed..9=Quick Sword. Set by ready right after "
             "\" READY.\" (asm ~10454); read by attack's melee damage "
             "formula (field_2B*8 + _strength, asm ~8050) and by "
             "zstats (field_2B+0x13 indexes TEXT_STRINGS' weapon "
             "names, matching ready's own digit+0x13 prompt formula)."},

    {"op": "rename_member", "struct": "Savegame", "offset": 0x2C,
     "new_name": "_readiedArmor",
     "note": "0=none/Skin..6=Power. Set by wear_armor (asm ~11053); "
             "read by zstats (field_2C+0x1D indexes TEXT_STRINGS' "
             "armor names, matching wear_armor's own digit+0x1D "
             "prompt formula)."},

    {"op": "rename_member", "struct": "Savegame", "offset": 0x2D,
     "new_name": "_readiedSpell",
     "note": "0=none..9=Kill. Set directly by magic from the keypress "
             "(asm 10089); read by zstats (field_2D+0x24 indexes "
             "TEXT_STRINGS' spell names -- None/Light/Down Ladder/Up "
             "Ladder/Passwall/Surface/Prayer/Magic Missile/Blink/Kill, "
             "matching Ultima II's real spell list)."},

    {"op": "rename_member", "struct": "Savegame", "offset": 0xA5,
     "new_name": "_gems",
     "note": "closes out an open item from the _mapMonsters decoding "
             "pass (was: 'Fighter-kill drop, +1 BCD, no consumption "
             "site found'). view requires it nonzero (\"VIEW WHAT?\" "
             "otherwise, asm 10927-10935) and decrements it on use "
             "(asm 10962-10968) -- spending a gem to see the world "
             "map. See docs/overview.md as above."},

    # -- found while tracing _readiedSpell into cast --

    {"op": "rename_member", "struct": "Savegame", "offset": 0xA1,
     "new_name": "_wands",
     "note": "wand-owned count. cast requires field_A1+field_A2 != 0 "
             "(\"NEED WAND OR STAFF!\" otherwise, asm ~13153-13165) -- "
             "TEXT_STRINGS index 47 is WAND. See "
             "docs/overview.md#cast--how-_readiedspell-drives-spell-effects."},

    {"op": "rename_member", "struct": "Savegame", "offset": 0xA2,
     "new_name": "_staves",
     "note": "staff-owned count, same gate as _wands above -- "
             "TEXT_STRINGS index 48 is STAFF."},

    # -- 3 per-item "owned"/"charges" arrays, surfaced while tracing
    # ready/wear_armor/cast. field_60 and field_80 already existed as
    # (wrongly undersized, 1-byte) struct members from earlier auto-
    # analysis; [di+76h] didn't exist as a member at all. Index ranges
    # confirmed from every read/write site: armor 0-6 (wear_armor wraps
    # input 7 -> 0), weapon 0-9 (ready's digit prompt is 1-9, get
    # increments a random 0-7 via `rand_byte & 7`), spell 0-9 (cast's
    # dispatch, see
    # docs/overview.md#cast--how-_readiedspell-drives-spell-effects). --

    {"op": "resize_member", "struct": "Savegame", "offset": 0x60,
     "new_name": "_armorOwned", "element_size": 1, "count": 7,
     "note": "was a 1-byte scalar, actually a 7-byte array (index "
             "0-6, one per TEXT_STRINGS armor type). Checked by "
             "wear_armor before letting you wear something you don't "
             "have (asm ~11019). No other struct member exists in "
             "0x61-0x7F, safe to expand."},

    {"op": "add_member", "struct": "Savegame", "offset": 0x76,
     "member": "_weaponOwned", "element_size": 1, "count": 10,
     "note": "not a recognized struct member before this -- raw "
             "`[di+76h]` displacement in ready (asm ~10406), get (asm "
             "~9472-9475, incremented on a random weapon pickup), "
             "steal (asm ~10601-10604), zstats (asm ~11370). 10-byte "
             "array (index 0-9, one per TEXT_STRINGS weapon type), "
             "0x76 through 0x7F -- lands exactly against field_80's "
             "start at 0x80, confirmed no collision."},

    {"op": "resize_member", "struct": "Savegame", "offset": 0x80,
     "new_name": "_spellCharges", "element_size": 1, "count": 10,
     "note": "was a 1-byte scalar, actually a 10-byte array (index "
             "0-9, one per TEXT_STRINGS spell). Gates and is "
             "decremented by cast, see "
             "docs/overview.md#cast--how-_readiedspell-drives-spell-effects."},

    # -- 3 magical-item protection flags for end_of_turn's random trap
    # encounters, found while closing out the cast trace. Not
    # recognized struct members before this (raw player+0xNNh
    # displacement). Same mechanic all three: item owned +
    # rand_byte()>=0x40 (~75%) resists the trap, otherwise (or if not
    # owned at all) it hits regardless. See
    # docs/overview.md#player0xa30xa40xae--magical-item-protection-flags-traced --

    {"op": "add_member", "struct": "Savegame", "offset": 0xA3,
     "member": "_bootsOwned", "element_size": 1, "count": 1,
     "note": "resists the leg-paralysis trap (\"LEGS PARALIZED!\" -> "
             "\"SAVED BY MAGICAL BOOTS!\", asm ~2062-2076). "
             "TEXT_STRINGS index 49 is BOOTS."},

    {"op": "add_member", "struct": "Savegame", "offset": 0xA4,
     "member": "_cloakOwned", "element_size": 1, "count": 1,
     "note": "resists the arm-paralysis trap (\"ARMS PARALIZED!\" -> "
             "\"SAVED BY MAGICAL CLOAK\", asm ~2101-2116). "
             "TEXT_STRINGS index 50 is CLOAK."},

    {"op": "add_member", "struct": "Savegame", "offset": 0xAE,
     "member": "_idolOwned", "element_size": 1, "count": 1,
     "note": "resists the sleep trap (\"SLEEP SPELL!\" -> \"SAVED BY "
             "IDOL!\", asm ~2160-2178). TEXT_STRINGS index 60 is "
             "GREEN IDOL."},

    # -- found while tracing board's vehicle-boarding requirements --

    {"op": "rename_member", "struct": "Savegame", "offset": 0xA7,
     "new_name": "_ankhOwned",
     "note": "already a recognized struct member (player.field_A7), "
             "just unnamed. Required to board the Rocket in `board` "
             "(\"A METALIC VOICE COMMANDS: YOU MUST HAVE AN ANKH!\" if "
             "zero, asm ~6140-6147). TEXT_STRINGS index 53 is ANKH. "
             "See docs/overview.md#boards-vehicle-boarding-requirements-fully-traced."},

    # -- field_A9/field_AC, confirmed which-is-which by re-reading the
    # fixed-up asm before applying (NOT the order given in chat, which
    # had them swapped -- field_AC gates Frigate, field_A9 gates
    # Airplane, verified at asm ~6036-6069) --

    {"op": "rename_member", "struct": "Savegame", "offset": 0xAC,
     "new_name": "_frigateAllowed",
     "note": "gates boarding the Frigate in `board` -- zero: "
             "\"THE CREW OF THIS SHIP WILL NOT LET YOU BOARD!\" "
             "(asm ~6038-6045); no plain-Ship fallback exists. See "
             "docs/overview.md#boards-vehicle-boarding-requirements-fully-traced."},

    {"op": "rename_member", "struct": "Savegame", "offset": 0xA9,
     "new_name": "_planeAllowed",
     "note": "gates boarding the Airplane in `board` -- zero: "
             "\"STRANGE YOU CAN'T GET IN!\" (asm ~6066-6073)."},
]

_SIZE_FLAGS = {1: idc.FF_BYTE, 2: idc.FF_WORD, 4: idc.FF_DWORD}


def _get_struct_id(name):
    sid = idc.get_struc_id(name)
    if sid == idc.BADADDR:
        print(f"[!] struct {name!r} not found -- typo, or does it need "
              f"creating first? This script only edits existing structs.")
        return None
    return sid


def add_member(struct, member, offset, element_size, count, note):
    sid = _get_struct_id(struct)
    if sid is None:
        return
    existing = idc.get_member_name(sid, offset)
    if existing:
        print(f"{struct}+{offset:X}: already {existing!r} -- skipping add "
              f"(use \"resize_member\" if it needs to change size/name)")
        return
    total = element_size * count
    label = f"{count} x {element_size}-byte elements = {total} bytes" \
        if count > 1 else f"{total} byte(s)"
    print(f"{struct}+{offset:X}: add member {member!r} ({label})")
    print(f"    {note}")
    if DRY_RUN:
        return
    flag = _SIZE_FLAGS[element_size] | idc.FF_DATA
    err = idc.add_struc_member(sid, member, offset, flag, -1, total)
    if err != 0:
        print(f"    [!] add_struc_member FAILED, error code {err}")


def rename_member(struct, offset, new_name, note):
    sid = _get_struct_id(struct)
    if sid is None:
        return
    cur = idc.get_member_name(sid, offset)
    if cur == new_name:
        print(f"{struct}+{offset:X}: already {new_name!r} -- skipping")
        return
    print(f"{struct}+{offset:X}: {cur!r} -> {new_name!r}")
    print(f"    {note}")
    if DRY_RUN:
        return
    ok = idc.set_member_name(sid, offset, new_name)
    if not ok:
        print("    [!] set_member_name FAILED")


def resize_member(struct, offset, new_name, element_size, count, note):
    sid = _get_struct_id(struct)
    if sid is None:
        return
    cur_name = idc.get_member_name(sid, offset)
    if cur_name == new_name:
        print(f"{struct}+{offset:X}: already {new_name!r} -- skipping "
              f"(not re-verifying size; delete the member by hand in IDA "
              f"first if it needs correcting again)")
        return
    total = element_size * count
    print(f"{struct}+{offset:X}: {cur_name!r} -> {new_name!r} "
          f"({count} x {element_size}-byte elements = {total} bytes)")
    print(f"    {note}")
    if DRY_RUN:
        return
    if cur_name:
        if not idc.del_struc_member(sid, offset):
            print("    [!] del_struc_member FAILED -- not attempting the add")
            return
    flag = _SIZE_FLAGS[element_size] | idc.FF_DATA
    err = idc.add_struc_member(sid, new_name, offset, flag, -1, total)
    if err != 0:
        print(f"    [!] add_struc_member FAILED, error code {err}")


def main():
    for entry in OPERATIONS:
        op = entry["op"]
        if op == "add_member":
            add_member(entry["struct"], entry["member"], entry["offset"],
                       entry["element_size"], entry.get("count", 1),
                       entry["note"])
        elif op == "rename_member":
            rename_member(entry["struct"], entry["offset"],
                          entry["new_name"], entry["note"])
        elif op == "resize_member":
            resize_member(entry["struct"], entry["offset"],
                          entry["new_name"], entry["element_size"],
                          entry["count"], entry["note"])
        else:
            print(f"[!] unknown op {op!r}, skipping entry: {entry}")

    if not OPERATIONS:
        print("[no-op] OPERATIONS is empty -- nothing to apply yet.")
    elif DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
