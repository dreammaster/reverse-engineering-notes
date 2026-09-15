"""
Followed up on ShowGameClockCommand's globals to find the actual clock
mechanism -- a genuinely major, foundational finding.

sub_2814C -> ComputeGameClockTime: converts word_36D01 (minutes since
midnight, 0-1439) into 12-hour clock fields for ShowGameClockCommand:
word_32934 = "AM"/"PM" (ASCII, byte-swapped in the mov immediate),
word_32948 = hour (1-12), word_3295C = minute. Handles the 12:xx AM
special case (hour forced to 12) and the AM/PM hour-12 wraparound.

sub_1FE4F -> AdvanceGameClock: the master per-minute clock tick.
Increments word_36D01; on overflow past 1440 (a full day), calls
sub_28FF9 (a 'new day' event, not traced), resets to 1, and rolls the
calendar -- day (word_36CFB) wraps at 31 into month (word_36CFD),
which wraps at 13 into year (word_36CFF): confirms a 30-day-month,
12-month-year in-game calendar. Also fires sub_1FFE4 (not traced,
plausibly a lighting/spawn-rate change) at exactly 6:00 AM (word_36D01
==0x168) or 6:00 PM (==0x438) -- dawn/dusk events. Separately runs a
5-tick (5-minute) periodic countdown (word_32954, gated on word_3295A
bit 0x800) that calls sub_1FD24 when it lapses.

Confirmed initial/advancement values elsewhere: the clock starts at
0x1E0 (8:00 AM, game start) and 0x1E0 (8 hours) is added on rest --
matches a classic "resting advances 8 hours" mechanic; a separate
+0x3C (1 hour) add exists too (context not traced this round).

Run via:
    .\run_ida_script.ps1 name_game_clock_tick.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2814C: "ComputeGameClockTime",
    0x1FE4F: "AdvanceGameClock",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2814C,
    "Converts word_36D01 (minutes since midnight, 0-1439) to 12-hour "
    "clock fields: word_32934='AM'/'PM', word_32948=hour(1-12), "
    "word_3295C=minute. Handles the 12:xx AM special case and PM "
    "hour-12 wraparound.",
    False,
)
ida_bytes.set_cmt(
    0x1FE4F,
    "Master per-minute game-clock tick. Increments word_36D01; past "
    "1440 (a full day), calls sub_28FF9 ('new day', not traced), "
    "resets to 1, and rolls the calendar: day (word_36CFB) wraps at "
    "31 into month (word_36CFD), which wraps at 13 into year "
    "(word_36CFF) -- a 30-day-month, 12-month-year calendar. Fires "
    "sub_1FFE4 (not traced, plausibly lighting/spawn-rate) at exactly "
    "6:00 AM or 6:00 PM -- dawn/dusk. Also runs a separate 5-minute "
    "periodic countdown (word_32954, gated on word_3295A bit 0x800) "
    "calling sub_1FD24 when it lapses.",
    False,
)
