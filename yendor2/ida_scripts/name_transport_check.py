"""
Revisited sub_1CDBC/sub_1CE6B -- flagged many rounds ago (and even
earlier, before this continuation) as "the transport-check table,
all-zero at rest so unconfirmable". With GetInventorySlotPtr's
inventory-layout knowledge now in hand, sub_1CE6B resolves cleanly.

sub_1CE6B(implicit word_328D4) -> FindItemInInventoryRange: walks
[word_328D4+0x11A] through 8 entries stride 4 -- EXACTLY
GetInventorySlotPtr's main-inventory slot data (group base 0x118 + 2).
For each nonzero item id, if it falls in [word_3293E, word_32940] (an
item-id range, i.e. "any item of this type/category"), returns the
byte offset where it was found. Otherwise, if the item is a container
(sub_12554 catalog lookup, [+0xC] bit 0x2000), recurses into it via
sub_1CECB (not traced) to search inside. After the 8 main slots, also
checks a single extra slot at +0x13E (plausibly an "equipped"
transport-item slot) the same way. A generic "does this character
carry an item of this type, anywhere including open containers"
search -- not transport-specific itself.

sub_1CDBC -> CheckTransportAvailability: given an item-type range
(word_3293E/word_32940), first checks a fixed 6-entry table at 0x9519
for a direct range match (setting word_32974 to it); if none, calls
SyncAllContainers then FindItemInInventoryRange for each party member
in turn, stopping at the first one who has a qualifying item. Reads as
"can the party use this mode of transport" -- either a scripted route
match or someone carrying the right item (a boat, a horse, etc.).

Run via:
    .\run_ida_script.ps1 name_transport_check.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1CE6B: "FindItemInInventoryRange",
    0x1CDBC: "CheckTransportAvailability",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1CE6B,
    "FindItemInInventoryRange (implicit word_328D4, range = "
    "word_3293E..word_32940): searches the 8 main inventory slots "
    "([+0x11A], matches GetInventorySlotPtr's layout) for an item id "
    "in range, recursing into container-type items (sub_12554 "
    "[+0xC] bit 0x2000) via sub_1CECB. Also checks one extra slot at "
    "+0x13E (plausibly 'equipped' transport item). Generic inventory "
    "search, not transport-specific by itself.",
    False,
)
ida_bytes.set_cmt(
    0x1CDBC,
    "CheckTransportAvailability(word_3293E/word_32940 = an item-type "
    "range): checks a fixed 6-entry table at 0x9519 for a direct "
    "match first; if none, calls SyncAllContainers then "
    "FindItemInInventoryRange for each party member until one "
    "qualifies. 'Can the party use this mode of transport' -- a "
    "scripted route or someone carrying the right item.",
    False,
)
