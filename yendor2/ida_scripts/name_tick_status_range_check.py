"""
Names sub_1A5A6, called 3 times from sub_1A582 (matching
TickStatusEffects' own pre-existing comment noting it's "called both
directly from HandleGameCommand and from sub_1A5A6"): loops cx times,
calling IsItemRangeAvailable(ax) each time (ax held constant across
the loop) and, whenever it sets word_32974 nonzero (an item found in
range), also calls TickStatusEffects. Reads as "check for an
available item in this range cx times, ticking status effects each
time one's found" -- the exact relationship between the range check
and which status effect ticks isn't fully traced. -> CheckAndTickAvailableAilment

Run via:
    .\run_ida_script.ps1 name_tick_status_range_check.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1A5A6
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CheckAndTickAvailableAilment", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CheckAndTickAvailableAilment': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Loops cx times calling IsItemRangeAvailable(ax); calls "
    "TickStatusEffects whenever it sets word_32974 nonzero. Called 3 "
    "times from sub_1A582.",
    False,
)
