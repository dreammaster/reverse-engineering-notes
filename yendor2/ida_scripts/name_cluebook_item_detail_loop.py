"""
Names sub_13119, the shared F5 item-subtype-3-6 category loop
(JEWELS/ARTIFACTS/UNIQUE ITEMS, MAGIC SCROLLS/QUARTZ, POTIONS,
SUPPLIES/FOOD) called from ShowClueBook -- the "more complex sibling"
of RunClueBookItemCategory flagged earlier this session as untraced.

Simpler shape than RunClueBookItemCategory (no region-table click
navigation): draws the current entry via ShowClueBookItemDetail, then
-- if the loaded record's [+0xC] bit 0x1000 is set, or its id
(word_32974) falls in RestCharacter's dispatch range (0x36-0x46) or
CastSpell's (0x12-0x1D, or the special id _val46) -- draws an extra
ability-info overlay (sub_1381C or sub_13957, not traced). Confirms
that some clue-book items (very plausibly the "MAGIC SCROLLS" subtype)
grant a spell/ability when used, and the clue book shows what that
ability does.

-> RunClueBookItemDetailWithAbilityInfo

Run via:
    .\run_ida_script.ps1 name_cluebook_item_detail_loop.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x13119
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunClueBookItemDetailWithAbilityInfo", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunClueBookItemDetailWithAbilityInfo': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shared F5 item-subtype 3-6 category loop (Jewels/Artifacts/"
    "Unique, Magic Scrolls/Quartz, Potions, Supplies/Food): draws via "
    "ShowClueBookItemDetail, then -- if a flag bit is set or the item "
    "id falls in RestCharacter's or CastSpell's dispatch range -- an "
    "extra ability-info overlay (sub_1381C/sub_13957, not traced). "
    "Some clue-book items (plausibly Magic Scrolls) grant a spell when "
    "used, and this shows what it does.",
    False,
)
