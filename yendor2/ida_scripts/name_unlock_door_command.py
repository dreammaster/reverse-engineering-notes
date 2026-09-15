"""
Traced sub_29738 (HandleGameCommand's handler for word_32974 in
0x21-0x2E plus 0x2F) -- the unlock-door command, tying together
ProbeFacingTile, LoadLockState, and ShowLockStatus's exact strings.

Calls ProbeFacingTile to find what's directly ahead. If it's a
lock-type object (type flags 0x8000/0x4000), loads its state via
LoadLockState or LoadCurgameRecord. Checks the "already unlocked" bit
(word_32DC8/byte_32DCD, the same test ShowLockStatus performs) and
shows the "NOT LOCKED" message directly if so. Otherwise compares the
door's required-key flags (word_32DCE) against the player's currently
held/selected key (word_36C81/word_2E548) to decide whether the
player has the matching key -- the actual unlock attempt.

-> UnlockDoorCommand

Run via:
    .\run_ida_script.ps1 name_unlock_door_command.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x29738
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UnlockDoorCommand", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UnlockDoorCommand': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "HandleGameCommand's unlock-door handler (word_32974 in "
    "0x21-0x2E or ==0x2F). Uses ProbeFacingTile to find what's ahead; "
    "if it's a lock-type object, loads its state via LoadLockState/"
    "LoadCurgameRecord. Shows 'NOT LOCKED' directly if the "
    "already-unlocked bit is set (same test as ShowLockStatus). "
    "Otherwise compares the door's required-key flags (word_32DCE) "
    "against the player's held key (word_36C81/word_2E548) to "
    "resolve the unlock attempt.",
    False,
)
