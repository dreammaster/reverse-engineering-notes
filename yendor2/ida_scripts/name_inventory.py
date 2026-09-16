"""
Names sub_245AE: draws up to 8 DrawListEntryLabel entries (icon +
text), one per _val1.._val8, each individually skippable via a bit in
word_328C4 (empty slot?), at 8 consecutive Y positions -- matches the
8-item-slot-per-character savegame layout documented in
file-formats.md's Hex Hacking Item Guide cross-reference. Then loops
handling per-slot selection (sub_255C7) and 'N' (next character)/'Q'
(back)/'E' (exit entirely) keys -- the same three keys ShowPartyMembers'
per-character iteration uses. This is the character's inventory/
equipment display. -> ShowCharacterInventory

Run via:
    .\run_ida_script.ps1 name_inventory.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x245AE
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowCharacterInventory", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowCharacterInventory': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws up to 8 item entries (DrawListEntryLabel, one per "
    "_val1.._val8, each skippable via a word_328C4 bit -- likely empty "
    "slots) -- matches the 8-item-slot-per-character savegame layout "
    "from file-formats.md. Then a selection loop: 'N' next character, "
    "'Q' back, 'E' exit entirely (mirrors ShowPartyMembers' outer "
    "iteration). The character inventory/equipment screen.",
    False,
)
