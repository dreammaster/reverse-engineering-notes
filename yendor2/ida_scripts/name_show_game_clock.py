"""
Traced sub_2819F (HandleGameCommand's handler for word_32974==7) and
dumped its two fixed template strings directly from the data segment:
"12:12 AM" and "12/12/1212" -- placeholder clock/calendar text whose
individual digit positions get overwritten with the actual in-game
time (word_32948=hour-ish, word_3295C=minute-ish, word_32934=AM/PM
byte) and date (word_36CFD=month-ish, word_36CFB=day-ish,
word_36CFF/word_36CFB=year components) values via sub_2572C (the
"look up string N, get pointer" idiom used to fetch each pre-formatted
number substring). Confirms the game tracks an in-game calendar, not
just a coarse "time of day" -- a genuine, standalone finding.

-> ShowGameClockCommand

Run via:
    .\run_ida_script.ps1 name_show_game_clock.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2819F
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowGameClockCommand", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowGameClockCommand': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "HandleGameCommand's handler for word_32974==7. Shows the "
    "in-game clock/calendar: fixed template strings '12:12 AM' and "
    "'12/12/1212' (dumped directly from the data segment) have their "
    "digit positions overwritten with the current time "
    "(word_32948/word_3295C/word_32934) and date "
    "(word_36CFD/word_36CFB/word_36CFF) via sub_2572C. Confirms an "
    "in-game calendar system, not just a coarse time-of-day value.",
    False,
)
