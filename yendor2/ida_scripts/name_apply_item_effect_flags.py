"""
Traced sub_1BB48 -- called from UseItem's own fallback path and from
inside several of its type-specific handlers (UseItemType_400,
sub_1BBED). It's a shared "apply this item's bit-level effects" step,
and confirms/extends the global-flag system found two rounds ago:
items can set/clear up to 6 global quest/world-state flags each, not
just the 1-2 seen from GrantMonsterRewards.

sub_1BB48 -> ApplyItemEffectFlags: uses SelectItemUseRecord to get the
current item-use record (es:si), then:
  - If es:[si+0xE] bit 0x20: clears bits in word_328F6/word_328F8
    (via es:[si+0x16]/es:[si+0x18] as complement masks) -- a "clear
    saved state" step.
  - Else if bit 0x10: copies word_328F6/word_328F8 into
    word_2E40C/word_2E40E -- a "restore saved state" step.
  - Unconditionally: clears bits in word_2E40C/word_2E40E (via
    es:[si+0x1E]/es:[si+0x20] as complement masks) then sets bits
    (via es:[si+0x1A]/es:[si+0x1C]) -- directly applying the item's
    effect onto these two flag words (plausibly current player/party
    status-effect or equipment-bonus flags, not confirmed).
  - Unless bit 2 is set together with word_328C6 bit 0x40 (in which
    case it just clears that bit and returns), iterates 6 more signed
    flag-index fields (es:[si+0x2E] onward, stride 2): positive sets
    via SetGlobalFlag, negative (negated) clears via ClearGlobalFlag,
    zero skips that slot. So an item can flip up to 6 global flags,
    not just the 1-2 seen from GrantMonsterRewards' 2-field version.

Run via:
    .\run_ida_script.ps1 name_apply_item_effect_flags.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1BB48
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyItemEffectFlags", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyItemEffectFlags': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Applies the current item-use record's (SelectItemUseRecord) bit-"
    "level effects: es:[si+0xE] bit 0x20 clears word_328F6/word_328F8 "
    "via a complement mask; bit 0x10 instead restores "
    "word_2E40C/word_2E40E from them. Unconditionally clears then "
    "sets bits in word_2E40C/word_2E40E from the item's own masks "
    "(es:[si+0x1A]/[si+0x1C]/[si+0x1E]/[si+0x20] -- plausibly current "
    "player/party status or equipment-bonus flags, not confirmed). "
    "Then, unless bit 2 + word_328C6 bit 0x40 both hold, walks 6 "
    "signed flag-index fields (es:[si+0x2E]+) applying SetGlobalFlag/"
    "ClearGlobalFlag to each nonzero one -- an item can flip up to 6 "
    "global quest/world-state flags.",
    False,
)
