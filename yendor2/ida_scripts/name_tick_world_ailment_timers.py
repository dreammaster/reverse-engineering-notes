"""
Names sub_1FE0A, called once from TickWorldAilments (its own internal
per-tick countdown helper).

Gated on word_36C79 bit 0x8000 ("timers pending" flag) -- returns
immediately if clear. Otherwise clears that bit and walks a 6-entry
global (not per-party-member) countdown-timer array at 0x9433,
paired one-for-one with 6 individual bits in word_36C79 (bits 3-8,
cleared via a rotating single-bit mask as the loop advances). For
each nonzero counter, subtracts dx (the tick amount); if it reaches
zero or below, sets word_328C4 bit 0x400 (a redraw/flash signal),
zeroes the slot, and clears its word_36C79 bit; if it's still
running, re-sets the 0x8000 "pending" bit so the next tick checks
again. -> TickWorldAilmentTimers

Run via:
    .\run_ida_script.ps1 name_tick_world_ailment_timers.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1FE0A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TickWorldAilmentTimers", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TickWorldAilmentTimers': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Gated on word_36C79 bit 0x8000 ('timers pending'). Ticks down 6 "
    "global countdown timers at 0x9433 by dx, each paired with its "
    "own word_36C79 bit (3-8): on expiry, sets word_328C4 bit 0x400 "
    "(redraw signal) and clears the bit; if still running, re-arms "
    "the 0x8000 pending flag. Called once from TickWorldAilments.",
    False,
)
