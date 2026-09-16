"""
Names a cluster of small WORLD.DAT resource-lookup/block-prep stubs.

Two families, distinguished by whether they touch a FileEntry struct
(bx = FileEntry pointer, sets up [+4]/[+6]/[+8]/[+0xA]/[+0xC]) or
just compute a location (word_368A7/A9/AB/AD/AF: id, length,
zero-flag, offset-lo, offset-hi) for a caller to use directly.

Lookup-only family (id in ax, category-relative index in bx, indexes
two fixed tables to get a 32-bit offset + length): each has one
distinct, already-named caller.
  sub_27B0D (LoadConversationText_4000) -> LookupConversationTextBlockOffset_4000
  sub_27D20 (LoadConversationText_2000) -> LookupConversationTextBlockOffset_2000
  sub_27CC9 (LoadConversationText_1000) -> LookupConversationTextBlockOffset_1000
  sub_27D55 (LoadConversationText_800)  -> LookupConversationTextBlockOffset_800
  sub_27B84 (UpdateScrollArrows, ShowClueBookSpellDetail) -> LookupSpellDescriptionBlockOffset
  sub_27BAD (PlayMusicTrack) -> LookupMusicTrackBlockOffset
  sub_27E53 (TriggerSoundEvent) -> LookupSoundEffectBlockOffset

FileEntry-prep family (LoadMasterPalette family, each with a fixed
record size and table, all 3 called from LoadItemData at different
offsets -- LoadItemData reads 3 differently-sized item-data
sub-blocks):
  sub_27BD6 (LoadItemData+0x2A, size 0x28) -> PrepareItemDataBlockRead28
  sub_27B42 (LoadItemData+0x50, size 0x3A) -> PrepareItemDataBlockRead3A
  sub_27BF7 (LoadItemData+0xA0, size 0x22) -> PrepareItemDataBlockRead22

Run via:
    .\run_ida_script.ps1 name_worlddat_lookup_and_prep_cluster.py
"""
import idc
import ida_name
import ida_bytes

lookup_desc = (
    "Resource-lookup stub: given an id (ax) and category index (bx), "
    "indexes two fixed tables to compute word_368A7/A9/AD/AF (id, "
    "length, offset-lo, offset-hi). Called from {caller}."
)
prep_desc = (
    "Resource-block-setup stub (LoadMasterPalette family): configures "
    "a FileEntry for a fixed-size ({size}) record read. Called from "
    "LoadItemData."
)

entries = [
    (0x27B0D, "LookupConversationTextBlockOffset_4000", lookup_desc.format(caller="LoadConversationText_4000")),
    (0x27D20, "LookupConversationTextBlockOffset_2000", lookup_desc.format(caller="LoadConversationText_2000")),
    (0x27CC9, "LookupConversationTextBlockOffset_1000", lookup_desc.format(caller="LoadConversationText_1000")),
    (0x27D55, "LookupConversationTextBlockOffset_800", lookup_desc.format(caller="LoadConversationText_800")),
    (0x27B84, "LookupSpellDescriptionBlockOffset", lookup_desc.format(caller="UpdateScrollArrows and ShowClueBookSpellDetail")),
    (0x27BAD, "LookupMusicTrackBlockOffset", lookup_desc.format(caller="PlayMusicTrack")),
    (0x27E53, "LookupSoundEffectBlockOffset", lookup_desc.format(caller="TriggerSoundEvent")),
    (0x27BD6, "PrepareItemDataBlockRead28", prep_desc.format(size="0x28")),
    (0x27B42, "PrepareItemDataBlockRead3A", prep_desc.format(size="0x3A")),
    (0x27BF7, "PrepareItemDataBlockRead22", prep_desc.format(size="0x22")),
]

for ea, name, desc in entries:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
