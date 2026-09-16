"""
Names sub_2A788, called once from HandleGameCommand -- the "Search"
command: examine the facing tile for a lock, a door/container, or a
hidden trap.

Prompts for a caster (ShowConfirmPrompt id 8) if not already chosen,
rejects an incapacitated character ([+0x1C] bits 0x1C40, the
confirmed afflictions bitfield), then waits for the player to click a
target tile (WaitForTargetClick) and probes it (ProbeFacingTile).
Branches on the probed cell's overlay flags [+2]: bit 0x8000 -> loads
a lock record (LoadLockState) and shows its status
(ShowLockStatus)/description directly if the caster lacks the
relevant proficiency (word_32DC8/byte_32DCD skill-mask check); bit
0x4000 -> loads a container/door record (LoadCurgameRecord) the same
way; neither bit set -> a hidden-trap search: rolls a
FailsSavingThrow using the caster's own stat ([+0x6C]) against
word_32DC0. On a failed save, triggers the trap (sub_274B4) with a
backfire message and redraws the caster's status panel. On a
successful save, marks the caster's proficiency-mask bit, persists a
record, applies a saving-throw status effect
(ApplySavingThrowEffect), and shows either "found something" or
"nothing found" (clearing the hidden-trap flag on a find).
-> HandleSearchCommand

Run via:
    .\run_ida_script.ps1 name_handle_search_command.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2A788
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleSearchCommand", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleSearchCommand': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "The 'Search' command: examine the facing tile for a lock "
    "(overlay flag 0x8000), a door/container (0x4000), or a hidden "
    "trap (neither bit) via a saving-throw skill check -- a failed "
    "search-for-traps roll triggers the trap (sub_274B4). Called "
    "once from HandleGameCommand.",
    False,
)
