"""
CORRECTION: CheckTransportAvailability (named 2 rounds ago) turns out
to be a more general-purpose check than its name implies. Traced a
NEW caller, sub_2B17F (an item-icon-dispatch handler, word_32974==
0x2C8): it calls the same function 4 times with word_3293E==word_32940
(exact single-item checks, not ranges) for 4 specific item ids
(0x254-0x257) -- if ALL FOUR are absent from every party member's
inventory, it plays a success sound and runs an animated sequence that
re-checks those same 4 items plus a 5th (0x2C8) in reverse order, with
a screen update after each. This reads as "the party has used/given
away all N required quest items" -- a completion check, not transport
availability. Since the function's own mechanism (a fixed lookup table
OR a party-wide inventory search over an id range, single ids included
as a degenerate range) is genuinely generic, "transport" was too
specific a guess from its first-seen use.

Renaming to a name describing the actual mechanism, and correcting
its own comment plus the two docs mentions.

-> IsItemRangeAvailable

Run via:
    .\run_ida_script.ps1 fix_transport_check_generic_name.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1CDBC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "IsItemRangeAvailable", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'IsItemRangeAvailable': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "CORRECTED from 'CheckTransportAvailability' -- too specific a "
    "guess. Given an item-id range (word_3293E/word_32940, a single "
    "id if equal), first checks a fixed 6-entry table at 0x9519 for a "
    "direct range match (setting word_32974 to it); if none, calls "
    "SyncAllContainers then FindItemInInventoryRange for each party "
    "member until one qualifies. Generic 'does the party have an item "
    "in this id range' check -- used both for a boat/horse-style "
    "transport gate and, via sub_2B17F, for a quest-item-completion "
    "check (4 specific items all absent triggers a completion "
    "sequence).",
    False,
)
print("Note: also update FindItemInInventoryRange's/CheckKeyItem-adjacent docs mentions manually if needed.")
