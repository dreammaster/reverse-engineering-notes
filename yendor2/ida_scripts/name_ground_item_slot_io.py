"""
Names 3 functions found investigating a ranked candidate, all part of
PlaceItemOnGround's (already-named) ground-item slot read/write cycle:

sub_1A34C (0x1A34C, called 3 times from PlaceItemOnGround): reads a
slot record (FileEntry_Read, errorCode=0xB, same sub_27E3A/word_36863
shape as WriteContainerSubBlock's write side) -- the read counterpart,
here for a ground/world-object slot rather than a party-record
sub-block. -> ReadGroundItemSlot

sub_1A320 (0x1A320, also called from PlaceItemOnGround): clears a
17-word scratch buffer (0xAFEC), transplants word_36863 (the count
ReadGroundItemSlot just read) into word_36E0F (saving the previous
value in word_3884C), then calls sub_1A36A to commit the write.
-> PrepareGroundItemSlotWrite

sub_1A36A (0x1A36A, called only from PrepareGroundItemSlotWrite): a
minimal write-commit (FileEntry_Write(errorCode=0xB) + ErrorCheck),
the same shape as CommitContainerWrite but for the ground-item slot.
-> CommitGroundItemWrite

Run via:
    .\run_ida_script.ps1 name_ground_item_slot_io.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x1A34C, "ReadGroundItemSlot"),
    (0x1A320, "PrepareGroundItemSlotWrite"),
    (0x1A36A, "CommitGroundItemWrite"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1A34C,
    "Reads a ground/world-object item slot record (FileEntry_Read, "
    "errorCode=0xB) into word_36863. Called 3 times from "
    "PlaceItemOnGround.",
    False,
)
ida_bytes.set_cmt(
    0x1A320,
    "Clears a 17-word scratch buffer, transplants word_36863 into "
    "word_36E0F (saving the previous value), then calls "
    "CommitGroundItemWrite. Called from PlaceItemOnGround.",
    False,
)
ida_bytes.set_cmt(
    0x1A36A,
    "Minimal write-commit (FileEntry_Write, errorCode=0xB) for the "
    "ground-item slot, the same shape as CommitContainerWrite. Called "
    "from PrepareGroundItemSlotWrite.",
    False,
)
