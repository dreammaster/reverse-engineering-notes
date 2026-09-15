"""
Names sub_1BA96, called from FinishItemUse and ShowItemUsagePreview.
Builds the message text for an item-use confirmation/preview.

If the held item's [+0xE] flags have bit 0x2000 set (a special item
type): calls sub_1C809 (not traced), then either applies the item's
effect directly (ApplyItemEffectFlags, when word_328C6 bit 0x40 is
set) or copies a fixed generic message (selected by the item's [+0x10]
value 1/2/3) into the message buffer.

Otherwise: reads a message count ([+0x14]) and pointer ([+0x12]) from
the item record and copies that many 0x22-byte message entries from
an EMS-backed segment (word_328F4) into the buffer -- item-specific
usage text.

-> BuildItemUseMessage

Run via:
    .\run_ida_script.ps1 name_build_item_use_message.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1BA96
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "BuildItemUseMessage", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'BuildItemUseMessage': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Builds the message text for an item-use confirmation/preview: "
    "for special items ([+0xE] bit 0x2000), either applies the effect "
    "directly or shows a generic message by [+0x10] tier; otherwise "
    "copies item-specific message entries from an EMS-backed segment "
    "([+0x12]/[+0x14]). Called from FinishItemUse and "
    "ShowItemUsagePreview.",
    False,
)
