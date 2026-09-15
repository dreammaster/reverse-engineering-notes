"""
Names sub_185A2, called from `start`, `HandleDungeonInput`, and all
three shop screens (RunSellItemScreen/RunEnhanceItemScreen/
RunRepairItemScreen via sub_1869D's DrawMouseCursor sequence).

Core role: refreshes the 4 party-member portrait slots
(g_partySlotAssignment 0x95EB/95ED/95EF/95F1) -- for each occupied
slot, sets a portrait-dirty-ish bit in word_328C6 (0x4000/2000/1000/
0x800, a distinct bit family from the shop action bits) and calls
sub_19133 (not traced) with a slot-specific offset (word_328BC) and a
shared height (word_328C0=8).

Conditional add-on: when any shop action bit is active
(word_328C6 & 0x1C, i.e. sell/enhance/repair), also draws a
context-sensitive hint -- confirmed via message dump: "SPACEBAR TO
ENHANCE ITEM" (bit 8), "SPACEBAR TO REPAIR ITEM" (bit 4), or the
default "SPACEBAR TO SELL ITEM OR ESC TO UNDO" (bit 0x10 or no shop
bit set).

-> RefreshPartyPortraits

Run via:
    .\run_ida_script.ps1 name_refresh_party_portraits.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x185A2
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RefreshPartyPortraits", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RefreshPartyPortraits': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Refreshes the 4 party-member portrait slots (g_partySlotAssignment) "
    "via sub_19133, then -- only when a shop action bit is active "
    "(word_328C6 & 0x1C) -- draws a context hint: 'SPACEBAR TO ENHANCE "
    "ITEM' / 'SPACEBAR TO REPAIR ITEM' / default 'SPACEBAR TO SELL "
    "ITEM OR ESC TO UNDO'. Called from `start`, HandleDungeonInput, "
    "and all three shop screens.",
    False,
)
