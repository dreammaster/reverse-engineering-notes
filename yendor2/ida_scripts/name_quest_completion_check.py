"""
Names sub_2B17F, the item-icon-dispatch handler (word_32974==0x2C8)
that led to the IsItemRangeAvailable correction: checks whether 4
specific items (ids 0x254-0x257) are ALL absent from every party
member's inventory (one IsItemRangeAvailable call per item, single-id
range each), and if so, plays a success sound and runs an animated
screen-update sequence re-checking those 4 items plus a 5th (0x2C8) in
reverse order. Reads as "the party has used/given away all N required
quest items" -- a completion check/reward sequence, not transport.
Exact narrative (which items, what they unlock) not identified.

-> CheckQuestItemsCompleted

Run via:
    .\run_ida_script.ps1 name_quest_completion_check.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2B17F
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CheckQuestItemsCompleted", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CheckQuestItemsCompleted': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Item-icon-dispatch handler (word_32974==0x2C8). Checks whether "
    "4 specific items (ids 0x254-0x257) are ALL absent from every "
    "party member's inventory (one IsItemRangeAvailable call per "
    "item). If so, plays a success sound and runs an animated "
    "screen-update sequence re-checking those 4 items plus a 5th "
    "(0x2C8) in reverse order -- reads as a quest-item-completion "
    "reward sequence. Exact narrative not identified.",
    False,
)
