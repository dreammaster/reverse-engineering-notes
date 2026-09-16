"""
Names sub_1AC80, called from ApplyMapTriggerEffect: the map-trigger
counterpart to ApplyItemEffectIconSlot. Classifies an item ([bx+di])
via ClassifyItemServiceTier; if valid, populates an icon-bar slot (si,
same layout as the other icon-bar slot populators) with the item
pointer/id, a caller-supplied flag (ds:[bp+2] bit 0x200 -> 0 or 1,
this function takes a stack-frame argument), and one of two
word_2E548 sub-fields based on the item's category (the same
GetClassifiedItemStatField pattern) -- else does nothing if
classification fails. -> ApplyTriggerEffectIconSlot

Run via:
    .\run_ida_script.ps1 name_apply_trigger_effect_slot.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1AC80
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyTriggerEffectIconSlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyTriggerEffectIconSlot': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Map-trigger counterpart to ApplyItemEffectIconSlot: classifies an "
    "item via ClassifyItemServiceTier and, if valid, populates an "
    "icon-bar slot with its id/pointer, a caller-supplied flag, and "
    "the same word_2E548 category-based field GetClassifiedItemStatField "
    "selects. Called from ApplyMapTriggerEffect.",
    False,
)
