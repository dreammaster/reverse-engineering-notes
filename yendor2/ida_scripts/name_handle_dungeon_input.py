"""
Named sub_16407 -- one of RunDungeonGameLoop's 3 per-iteration input
handlers (alongside sub_16881/sub_25AAC), and the one that turned out
to own mouse-click monster-panel selection and the actual attack
resolution (ResolveAttack/UpdateMonsterWoundTier, this round).
Confirmed structure: selects a party member via a caller-supplied
pointer (word_328D4 = [bx]), then handles a wide range of dungeon UI
input (movement, monster-panel clicks, attacks, dialogs via
RunGameDialog) in a long dispatch chain.

Also documents where a monster's HP is actually decremented: right
after UpdateMonsterWoundTier, this function does
`[word_32A1E+0x10] -= word_2E49C` -- confirms g_monsterSlots' +0x10
field doubles as current HP in the active-combat context (see
file-formats.md's combat section for the full note).

-> HandleDungeonInput

Run via:
    .\run_ida_script.ps1 name_handle_dungeon_input.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16407
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleDungeonInput", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleDungeonInput': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "One of RunDungeonGameLoop's 3 per-iteration input handlers. "
    "Selects a party member (word_328D4 = [bx], a caller-supplied "
    "pointer), then dispatches a wide range of dungeon UI input: "
    "movement, monster-panel clicks (sets word_32A1E as a target-"
    "selection shortcut alongside SelectActiveMonster), attack "
    "resolution (ResolveAttack + UpdateMonsterWoundTier, then "
    "[word_32A1E+0x10] -= word_2E49C -- the actual HP subtraction), "
    "and dialogs via RunGameDialog.",
    False,
)
