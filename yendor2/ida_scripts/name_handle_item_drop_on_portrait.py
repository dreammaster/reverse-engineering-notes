"""
Names sub_18C79, called from `start` and HandleDungeonInput -- the
"drop held item onto a party member's portrait" handler (give/equip
via drag-and-drop).

Hit-tests against the same table (0x61C2) HandlePartyStatusPanelInput
uses; indices 1/0xB/0x15/0x1F select one of the 4 portrait zones
(word_95EB/95ED/95EF/95F1, the confirmed g_partySlotAssignment
entries), setting up a clip rectangle (word_328BC/328C0) for the drop
target and word_32924 = the selected party record pointer. Any other
index (or an empty slot) rejects via FlashStatusWarning. Checks
whether the held item's weight fits the character's remaining carry
capacity ([+0x118] + held quantity vs. the confirmed cap at [+0x56]),
exempting a special item id (0x11), then loads the item's catalog
record and dispatches on its [+0xC] category flags (0x8000/0x2000/
0x4000/0x800/...) to apply it appropriately (equip/stack/etc, not
individually traced). -> HandleItemDropOnPartyPortrait

Run via:
    .\run_ida_script.ps1 name_handle_item_drop_on_portrait.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x18C79
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleItemDropOnPartyPortrait", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleItemDropOnPartyPortrait': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Drop-held-item-onto-portrait handler (give/equip via drag-and-"
    "drop): hit-tests table 0x61C2 for one of the 4 portrait zones, "
    "checks carry-capacity, then dispatches on the item's [+0xC] "
    "category flags. Called from `start` and HandleDungeonInput.",
    False,
)
