"""
One-off structural setup: creates the RosterEntry struct (64 bytes) in
ultima_bootup.idb, confirmed field-by-field against real code in
showCharacterDetails, handleCreateCharacter, showRegister, and
getRosterEntryPointer -- see docs/overview.md's findings log and
docs/file-formats.md's ROSTER.ULT section for the full evidence trail.
This is the "Savegame"-equivalent struct for Ultima III, matching the
convention set by ultima1/ultima2's own Savegame structs.

apply_structs_bootup.py only edits an *existing* struct (add_struc_member/
set_member_name) -- it doesn't create one from scratch, hence this
separate one-off script. Run this once, then apply_structs_bootup.py's
OPERATIONS list can add/rename further members as more fields are traced.

Every offset here comes from a real `[bx+N]` access in the disassembly,
not from the external file-formats.md documentation alone (though it
matches that documentation exactly for every field checked) -- see the
apply_renames_bootup.py notes for showCharacterDetails/
handleCreateCharacter for the specific evidence per field.
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
    (0x1A, "_hitPoints", 2, 1, "showCharacterDetails (printHexWord/BCD), set to 150 (BCD) at creation."),
    (0x1C, "_maxHitPoints", 2, 1, "set to 150 (BCD) at creation alongside _hitPoints -- offset inferred from handleCreateCharacter's parallel word write, not independently displayed by showCharacterDetails (which only shows one HP value) -- LOW CONFIDENCE on the exact label, confirmed only that *a* second HP-shaped word lives here."),
    (0x1E, "_experience", 2, 1, "showCharacterDetails (printHexWord/BCD)."),
    (0x21, "_food", 2, 1, "showCharacterDetails (printHexWord/BCD), set to 150 (BCD) at creation."),
    (0x23, "_gold", 2, 1, "showCharacterDetails (printHexWord/BCD)."),
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
