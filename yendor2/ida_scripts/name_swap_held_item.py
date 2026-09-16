"""
Names sub_268A0, moderate-high confidence: called from the still-
untraced sub_2621C (a flagged container-related open lead).

Plays a sound (ax=6, distinct from FinishPlacingHeldItem's ax=4),
restores the cursor background, then does a careful save/restore
dance around the held-item triple (word_31948/word_3194C/word_3194A):
saves the original triple, calls sub_26B4F (presumably picks up/removes
an item from a target slot, changing the held-item globals as a side
effect), restores the *original* triple just long enough to call
sub_266D4 (presumably places that original item into the target slot),
then restores the *new* triple sub_26B4F left behind (the slot's former
item, now held). Finishes with UpdateCursorForHeldItem,
DrawPartyMemberPortrait, and the common redraw-completion helper. Reads
as a classic "swap the currently-held item with a slot's item"
operation; sub_26B4F/sub_266D4 themselves aren't traced.
-> SwapHeldItemWithSlot

Run via:
    .\run_ida_script.ps1 name_swap_held_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x268A0
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SwapHeldItemWithSlot", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SwapHeldItemWithSlot': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Swaps the currently-held item (word_31948/3194C/3194A) with a "
    "target slot's item via a save/restore dance around sub_26B4F "
    "(pick up the slot's item) and sub_266D4 (place the original held "
    "item into the slot). Called from the still-untraced, "
    "container-related sub_2621C.",
    False,
)
