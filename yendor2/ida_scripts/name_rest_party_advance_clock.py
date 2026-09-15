"""
Names sub_1E64A, called from `start` (action-toolbar button 3) and
sub_2C0FE: the party rest/camp action ("R rest" in the manual).

Bails if carrying an item. Checks eligibility (sub_1EA18) and shows a
2-line rejection (msg 0x7DA5) if not eligible (e.g. can't rest here).
Otherwise draws a resting animation (picture 8), then advances the
game clock: if word_328C4 bit 2 is set (a full/uninterrupted rest),
adds a flat 8 hours (word_36D01 += 0x1E0 -- matches the documented
"resting advances the clock by 8 hours" convention); otherwise loops
up to 8 times adding 1 hour each (+= 0x3C), calling the already-named
ProcessLevelMonsters per hour and stopping early if combat starts.
Handles day rollover (word_36D01 wrapping at 0x59F) by calling the
already-named ResetDailyAbilityCharges and advancing the
month/day/week counters (word_36CFB/36CFD/36CFF). Shows how many
hours were rested (msgs 0x7D3E/0x7D58), then polls for input before
resuming via RunDungeonGameLoop -- or, if combat interrupted the rest,
counts living party members for a follow-up step.

-> RestPartyAndAdvanceClock

Run via:
    .\run_ida_script.ps1 name_rest_party_advance_clock.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1E64A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RestPartyAndAdvanceClock", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RestPartyAndAdvanceClock': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "The party rest/camp action ('R rest'). Checks eligibility "
    "(sub_1EA18), advances the game clock (8 hours flat for a full "
    "rest, or up to 8 hourly ticks calling ProcessLevelMonsters and "
    "stopping if combat starts), handles day rollover "
    "(ResetDailyAbilityCharges + calendar counters), shows hours "
    "rested, then resumes via RunDungeonGameLoop. Called from `start` "
    "and sub_2C0FE.",
    False,
)
