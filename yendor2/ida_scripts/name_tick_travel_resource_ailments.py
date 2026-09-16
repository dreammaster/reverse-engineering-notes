"""
Names sub_1A582, called once from TravelToDestination -- clears
word_328C4 bit 0x40, then calls the already-named
CheckAndTickAvailableAilment 3 times with different (item/effect id,
range/count) pairs: (ax=9, cx=word_36C85), (ax=0xF, cx=word_36C89),
(ax=0xC, cx=word_36C8B). Per CheckAndTickAvailableAilment's own
documented behavior (loops IsItemRangeAvailable and ticks the status
effect whenever a matching item turns up in range), this checks 3
distinct resource/consumable item types for availability during
world travel -- plausibly food, water, and light-source consumption
tracking, though the specific item ids (9/0xF/0xC) aren't confirmed.
-> TickTravelResourceAilments

Run via:
    .\run_ida_script.ps1 name_tick_travel_resource_ailments.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1A582
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TickTravelResourceAilments", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TickTravelResourceAilments': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clears word_328C4 bit 0x40, then calls "
    "CheckAndTickAvailableAilment 3x with (ax=9,cx=word_36C85), "
    "(ax=0xF,cx=word_36C89), (ax=0xC,cx=word_36C8B) -- checks 3 "
    "resource/consumable item types for availability during travel, "
    "plausibly food/water/light-source tracking. Called from "
    "TravelToDestination.",
    False,
)
