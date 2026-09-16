"""
Names sub_2C0FE, called from RunAlchemyScreen and InteractWithContainer
-- confirmed via InteractWithContainer's own call sequence
(ConfirmContainerInteraction -> sub_2C0FE -> ConsumeItemChargeResource)
to be the "apply the item/container's coded magical effect, right
before its charge is spent" dispatcher.

A bitmask switch over two caller-populated flag words, word_33302
(15 distinct bits) and word_33306 (4 more), each mapped to its own
effect implementation via a flat if-else chain (no bit combines with
another within one call -- each call handles exactly one effect type
then returns, `byte_2E400=0; retf`). Two branches read closely:

- word_33302 bit 0x8000: applies an icon-bar status effect
  (PrepareTrapEffectSlots + ResetOrCopyTargetPositionFields +
  ApplyEffectAndDrawIconBar) to a single target party member
  (word_3331C, resolved via the confirmed g_partySlotAssignment
  search).
- word_33302 bit 0x4000: the identical effect, but looped across all
  4 party slots -- the "whole party" version of the same status
  effect.
- word_33302 bit 0x80: an unrelated effect entirely -- plays a sound
  then sets one of 4 world-state globals (word_36C93/95/97/99,
  selected by word_332E4) to a caller value (word_332E6) -- some kind
  of persistent world/region timer or counter, not an icon-bar
  effect.

The remaining ~16 branches (not individually traced -- this is a
4,200-byte function) plausibly cover the rest of a magical item's or
spell's possible effect repertoire: teleport, stat drain, curse,
item destruction, etc., matching the breadth of enchanted-item/
trapped-container effects a first-person RPG of this era would need.
-> ApplyEncodedItemEffect

Run via:
    .\run_ida_script.ps1 name_apply_encoded_item_effect.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2C0FE
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyEncodedItemEffect", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyEncodedItemEffect': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Bitmask-switch effect dispatcher (word_33302/word_33306, ~19 "
    "distinct effect types, one handled per call) applying a "
    "spell's/container's coded magical effect. Confirmed via "
    "InteractWithContainer: called right before "
    "ConsumeItemChargeResource, i.e. 'apply the effect, then spend "
    "the charge'. Covers single-target and whole-party icon-bar "
    "status effects plus other effect types (world-state timers, "
    "etc.) not individually traced. Called from RunAlchemyScreen and "
    "InteractWithContainer.",
    False,
)
