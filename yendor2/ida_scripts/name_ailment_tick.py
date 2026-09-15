"""
Traced AdvanceGameClock's 5-minute periodic timer chain (sub_1FD24 /
sub_1FDB1) -- turns out to be a status-ailment duration ticker, not an
item/torch timer as first guessed.

sub_1FDB1 -> TickAilmentDuration(si=a slot, cx=count, dx=elapsed
delta): for each of cx consecutive 4-byte slots, checks if [si] holds
one of 3 specific codes (9/0xF/0xC -- the exact trio TickStatusEffects
already manages via word_36C79, from many rounds ago). If so,
decrements [si+2] (remaining duration) by dx; on expiry, zeroes it,
bumps [si], and decrements one of 3 global per-ailment-type counters
(0x9425/0x9429/0x942B, selected by which code). When that counter
reaches 0 (no instances of this ailment remain anywhere), clears the
matching bit in word_36C79 and flags a redraw. Also sets word_328CA
bit 0x40 if dx>1 (a coarser-tick marker, not confirmed).

sub_1FD24 -> TickWorldAilments: the 5-minute sweep. Calls
TickAilmentDuration over the 6-entry table at 0x9519 (also used by
CheckTransportAvailability -- may be a more general "world slot" table
than pure transport ranges), then over every party member's 8 main
inventory slots ([+0x11A], matches GetInventorySlotPtr) -- so ailments
are apparently tracked as special entries occupying the same slot
storage as items/world-table rows, not a separate structure. Then
calls sub_1FE0A (a related status-flag sweep, not traced) once, and
finally sums all 12 known status-duration counters
(word_36C83..word_36C9D) -- if every one is 0, clears word_3295A bit
0x800, the flag that gates this whole timer from firing again (an
optimization: stop ticking once nothing is left to tick).

Run via:
    .\run_ida_script.ps1 name_ailment_tick.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1FDB1: "TickAilmentDuration",
    0x1FD24: "TickWorldAilments",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1FDB1,
    "TickAilmentDuration(si=slot array, cx=count, dx=elapsed delta): "
    "for each 4-byte slot whose [si] is one of the 3 ailment codes "
    "(9/0xF/0xC, matching TickStatusEffects), decrements [si+2] by "
    "dx; on expiry, zeroes it, bumps [si], and decrements one of 3 "
    "global per-ailment counters (0x9425/0x9429/0x942B). At 0, "
    "clears the matching word_36C79 bit and flags a redraw.",
    False,
)
ida_bytes.set_cmt(
    0x1FD24,
    "5-minute periodic sweep (AdvanceGameClock). Runs "
    "TickAilmentDuration over the 6-entry table at 0x9519 and every "
    "party member's 8 main inventory slots ([+0x11A]) -- ailments "
    "occupy the same slot storage as items/world-table rows. Calls "
    "sub_1FE0A once (a related status sweep, not traced). Sums all "
    "12 known status-duration counters; if all 0, clears word_3295A "
    "bit 0x800 so this timer stops firing until something needs it "
    "again.",
    False,
)
