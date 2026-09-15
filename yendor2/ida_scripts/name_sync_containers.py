"""
Traced sub_2952A (called at the top of RepairItemCommand, and from
sub_1CDBC) down to a small cluster that extends the container-
persistence finding from a few rounds ago (LoadContainerContents/
SaveAndCloseContainer).

sub_2957E -> SyncContainerContents(di=bag marker offset): identical to
SaveAndCloseContainer's write-back logic (if the bag has contents,
write them to CURGAME) but does NOT clear the marker afterward --
saves without closing the container.

sub_2955C -> SyncPartyMemberContainers(implicit word_328D4): calls
SyncContainerContents for all 3 of one character's bag slots
(+0x17C/+0x1A2/+0x1C8).

sub_2952A -> SyncAllContainers: iterates all 4 g_partySlotAssignment
members, calling SyncPartyMemberContainers for each -- commits every
open bag's contents to CURGAME across the whole party. Called before
RepairItemCommand's risky roll, presumably to keep the savegame
current in case the attempt destroys an item.

Run via:
    .\run_ida_script.ps1 name_sync_containers.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2957E: "SyncContainerContents",
    0x2955C: "SyncPartyMemberContainers",
    0x2952A: "SyncAllContainers",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2957E,
    "Like SaveAndCloseContainer's write-back (if the bag at [di] has "
    "contents, write them to CURGAME) but doesn't clear the marker "
    "afterward -- saves without closing.",
    False,
)
ida_bytes.set_cmt(
    0x2955C,
    "Calls SyncContainerContents for all 3 of word_328D4's bag slots "
    "(+0x17C/+0x1A2/+0x1C8).",
    False,
)
ida_bytes.set_cmt(
    0x2952A,
    "Iterates all 4 g_partySlotAssignment members, calling "
    "SyncPartyMemberContainers for each -- commits every open bag's "
    "contents to CURGAME across the whole party.",
    False,
)
