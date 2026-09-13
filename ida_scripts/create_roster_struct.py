"""
One-off structural setup: creates the RosterEntry struct (64 bytes),
confirmed field-by-field against real code in ultima_bootup.idb
(showCharacterDetails, handleCreateCharacter, showRegister,
getRosterEntryPointer) and cross-confirmed independently in
ultima_exodus.idb (castSpell, healHitPoints, addExperienceClamped,
addGoldClamped, combatCmdNegateTime -- see docs/overview.md's findings
log). This is the "Savegame"-equivalent struct for Ultima III, matching
the convention set by ultima1/ultima2's own Savegame structs.

IDA structs are per-IDB, not shared -- run this against BOTH
ultima_bootup.idb and ultima_exodus.idb (same MEMBERS list, since the
in-memory layout is identical: both use `[bx+N]`-style access into
0x40-byte character records copied from ROSTER.ULT/PARTY.ULT):

    .\\run_ida_script.ps1 -Idb ultima_bootup -ScriptName create_roster_struct.py
    .\\run_ida_script.ps1 -Idb ultima_exodus -ScriptName create_roster_struct.py

apply_structs_bootup.py/apply_structs_exodus.py only edit an *existing*
struct (add_struc_member/set_member_name) -- they don't create one from
scratch, hence this separate one-off script. Run this once per IDB,
then those scripts' OPERATIONS lists can add/rename further members as
more fields are traced. Idempotent per-member (add_struc_member on an
offset that already has a member fails harmlessly with an error code,
printed but not treated as fatal) -- safe to re-run after adding new
members to MEMBERS, though a member added after this script's first run
in a given IDB will need a manual add_struc_member call there too if
this script isn't re-run wholesale (this script doesn't currently
special-case "struct exists but member doesn't").

Every offset here comes from a real `[bx+N]` access in the disassembly,
not from the external file-formats.md documentation alone (though it
matches that documentation exactly for every field checked) -- see the
apply_renames_bootup.py/apply_renames_exodus.py notes for the specific
evidence per field.
"""

import idc
import ida_struct

DRY_RUN = False

STRUCT_NAME = "RosterEntry"
STRUCT_SIZE = 0x40

# (offset, name, element_size, count, note)
MEMBERS = [
    (0x00, "_name", 1, 0xA,
     "character name, ASCII, null-padded. readLine reads up to 9 "
     "chars during creation; external doc gives the field 10 bytes."),
    (0x0E, "_marksAndCards", 1, 1,
     "confirmed via ultima_exodus.idb's cmdYell ('Y'), which checks "
     "bit 0x40 of this field after a successful keyword match -- "
     "matches docs/file-formats.md's externally-sourced 'Marks/cards "
     "bitmask' field label exactly. Individual bit meanings not "
     "decoded."),
    (0x0F, "_torches", 1, 1,
     "confirmed via ultima_exodus.idb's cmdIgniteTorch ('I'), which "
     "spends 1 torch (BCD-borrow-checked decrement, dungeon-only) to "
     "set a lit-torch flag (byte_115CE). Matches docs/file-formats.md's "
     "externally-sourced 'Torch count (BCD)' field exactly. Offsets "
     "0x0A-0x0E remain unlabeled (external doc: 4 bytes unknown at "
     "0x0A, a 'Marks/cards' bitmask at 0x0E)."),
    (0x10, "_partyMember",
     1, 1, "0x00 = not in active party, 0xFF = in active party "
     "(handleFormParty/clearPartySelection/showRegister all check/set "
     "this)."),
    (0x11, "_status", 1, 1,
     "'G'=Good, 'P'=Poisoned (both alive per isCharacterAlive), "
     "'D'=Dead, 'A'=Ashes (aGood/aPoisoned/aDead/aAshes string table, "
     "not yet confirmed which byte maps to which beyond G/P)."),
    (0x12, "_strength", 1, 1, "showCharacterDetails/gatherCharacterCreationInput, BCD."),
    (0x13, "_dexterity", 1, 1, "showCharacterDetails/gatherCharacterCreationInput, BCD."),
    (0x14, "_intelligence", 1, 1, "showCharacterDetails/gatherCharacterCreationInput, BCD."),
    (0x15, "_wisdom", 1, 1, "showCharacterDetails/gatherCharacterCreationInput, BCD."),
    (0x16, "_race", 1, 1, "showCharacterDetails/gatherCharacterCreationInput, index into a fixed race-name table."),
    (0x17, "_class", 1, 1, "showCharacterDetails/gatherCharacterCreationInput, index into a fixed class-name table."),
    (0x18, "_sex", 1, 1, "showCharacterDetails/gatherCharacterCreationInput, index into Male/Female/Other."),
    (0x19, "_magicPoints", 1, 1,
     "current magic points (BCD). Found and confirmed in "
     "ultima_exodus.idb's castSpell: a spell's BCD-packed MP cost is "
     "compared against this field and BCD-subtracted from it on a "
     "successful cast ('M.P. too low!' otherwise)."),
    (0x1A, "_hitPoints", 2, 1, "showCharacterDetails (printHexWord/BCD), set to 150 (BCD) at creation."),
    (0x1C, "_maxHitPoints", 2, 1,
     "set to 150 (BCD) at creation alongside _hitPoints. CONFIRMED "
     "2026-09-13 via ultima_exodus.idb's healHitPoints, which clamps "
     "_hitPoints to this field's value after adding -- no longer just "
     "an offset guess from the parallel creation-time write."),
    (0x1E, "_experience", 2, 1, "showCharacterDetails (printHexWord/BCD); addExperienceClamped in ultima_exodus.idb confirms this offset via real arithmetic (BCD add, clamped to 9999)."),
    (0x21, "_food", 2, 1, "showCharacterDetails (printHexWord/BCD), set to 150 (BCD) at creation."),
    (0x23, "_gold", 2, 1, "showCharacterDetails (printHexWord/BCD); addGoldClamped in ultima_exodus.idb confirms this offset via real arithmetic (BCD add, clamped to 9999)."),
    (0x25, "_gems", 1, 1,
     "confirmed via ultima_exodus.idb's cmdPeer ('P'), which spends 1 "
     "gem (BCD-borrow-checked decrement, 'None Left!' on underflow) to "
     "peer at a dungeon layout or overworld view."),
    (0x26, "_keys", 1, 1,
     "confirmed via ultima_exodus.idb's cmdUnlock ('U'), which spends "
     "1 key (BCD-borrow-checked decrement) to unlock a door (tile "
     "0xB8) after prompting 'Whose key? '."),
    (0x27, "_powder", 1, 1,
     "confirmed via ultima_exodus.idb's combatCmdNegateTime, which "
     "spends 1 Powder (BCD-borrow-checked decrement) to set "
     "_negateTimeDuration=0x0A. Matches docs/file-formats.md's "
     "externally-sourced 'Powders' field label."),
    (0x28, "_armourIndex", 1, 1, "currently-equipped armour type index, showCharacterDetails."),
    (0x29, "_armourOwned", 1, 7, "7-entry owned-quantity array (Cloth/Leather/Chain/Plate/+2 Chain/+2 Plate/Exotic per external doc) -- only [0] confirmed written (=1 at creation); the array bounds are from file-formats.md, not independently re-derived here."),
    (0x30, "_weaponIndex", 1, 1, "currently-equipped weapon type index, showCharacterDetails."),
    (0x31, "_weaponOwned", 1, 0xF, "15-entry owned-quantity array (Dagger/Mace/.../Exotic per external doc) -- only [0] confirmed written (=1 at creation); array bounds from file-formats.md."),
]


def main():
    sid = idc.get_struc_id(STRUCT_NAME)
    if sid == idc.BADADDR:
        print(f"Creating struct {STRUCT_NAME!r} ({STRUCT_SIZE} bytes)")
        if not DRY_RUN:
            sid = idc.add_struc(-1, STRUCT_NAME, 0)
            if sid == idc.BADADDR:
                print("[!] add_struc FAILED")
                return
    else:
        print(f"Struct {STRUCT_NAME!r} already exists (sid={sid})")

    if DRY_RUN:
        for offset, name, esize, count, note in MEMBERS:
            print(f"  +{offset:#x} {name} ({esize}x{count})")
        print("\n[dry] nothing created. Set DRY_RUN = False to apply.")
        return

    size_flags = {1: idc.FF_BYTE, 2: idc.FF_WORD, 4: idc.FF_DWORD}
    for offset, name, esize, count, note in MEMBERS:
        total = esize * count
        flag = size_flags[esize] | idc.FF_DATA
        err = idc.add_struc_member(sid, name, offset, flag, -1, total)
        status = "ok" if err == 0 else f"error {err}"
        print(f"  +{offset:#x} {name} ({total} bytes) -> {status}")

    cur_size = idc.get_struc_size(sid)
    print(f"\nFinal struct size: {cur_size:#x} (expected {STRUCT_SIZE:#x})")


if __name__ == "__main__":
    main()
