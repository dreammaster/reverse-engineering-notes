"""
Names sub_2B384, moderate confidence: called once from
ProcessLevelMonsters (the monster AI tick), right after that function
computes a one-cell step toward the player (word_2E402/word_2E406/
word_2E404, compared against word_36CF7/word_36CF9 -- the player's
position) and just before checking whether the resulting cell
(word_328D2) is the player's own cell (triggering an encounter).

Given es:bx = a map-cell record (word_2E562 segment, [si+6]+word_2E404
offset, si = the monster record), it returns pass/fail via errorCode
(0 = step allowed, 1 = blocked, matching ClassifyFloorType/
IsCellTypeImpassable's convention): blocked outright if cell flags
have bits 0xC00 set; a "special" cell (flags & 0x6000) is passable only
if a monster trait flag ([si+0x94] bit 0x10) is set (walls/doors this
monster can bypass?); a handful of other value-range checks against
_val31/_val32 and fixed ranges (0x27-0x2A, ax<=1, ax==0x25) gated by
other [si+0x94] bits (0x8/0x14/0x1A); otherwise falls through to the
same ClassifyFloorType + IsCellTypeImpassable pair used elsewhere for
plain terrain-passability checks. The exact meaning of each [si+0x94]
bit (monster movement traits -- flying/incorporeal/door-opening, etc.)
isn't confirmed; naming only the overall "can this monster take this
step" behavior. -> IsMonsterStepBlocked

Run via:
    .\run_ida_script.ps1 name_monster_step_blocked.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2B384
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "IsMonsterStepBlocked", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'IsMonsterStepBlocked': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Checks whether a monster's (si) computed one-cell step (es:bx, "
    "bx=[si+6]+word_2E404) is passable, returning via errorCode (0=ok, "
    "1=blocked). Cell flag bits 0xC00 always block; bits 0x6000 need "
    "monster trait [si+0x94] bit 0x10; several other branches gate on "
    "value ranges (_val31/_val32, 0x27-0x2A, <=1, ==0x25) and other "
    "[si+0x94] bits (8/0x14/0x1A) whose exact meaning (movement traits: "
    "flying/incorporeal/door-opening?) isn't confirmed; otherwise falls "
    "through to ClassifyFloorType + IsCellTypeImpassable, the same pair "
    "used for plain terrain checks. Called once from ProcessLevelMonsters "
    "right after it computes a one-cell step toward the player.",
    False,
)
