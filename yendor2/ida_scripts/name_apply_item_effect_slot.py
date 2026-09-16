"""
Names sub_1AE4C, called from sub_1ACD7 (the same caller cluster as
ClassifyItemServiceTier): computes an icon-bar slot pointer
(si = 0xC50 + slot_index*0x14, the same icon-bar slot layout
TickPartyAilmentIconBar/ApplySavingThrowEffect populate, slot_index
derived from word_32924's offset into g_partySlotAssignment), calls
PrepareTrapEffectSlots(ax=0) (effect id 0 -- a new id, not seen
elsewhere yet), and fills the slot's effect-def pointer, the item id
(word_3290A), a value dereferenced from word_32904, the current party
record pointer (word_328D4), and a delta between the two -- then calls
ApplyEffectAndDrawIconBar. Reads as "apply this item's effect (id 0)
via the icon-bar mechanism and draw it," but the exact narrative isn't
confirmed. -> ApplyItemEffectIconSlot

Run via:
    .\run_ida_script.ps1 name_apply_item_effect_slot.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1AE4C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyItemEffectIconSlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyItemEffectIconSlot': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Populates an icon-bar slot (0xC50 + slot_index*0x14, the same "
    "layout TickPartyAilmentIconBar/ApplySavingThrowEffect use) for "
    "effect id 0 tied to the current item, then calls "
    "ApplyEffectAndDrawIconBar. Called from sub_1ACD7.",
    False,
)
