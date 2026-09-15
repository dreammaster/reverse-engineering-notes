"""
Names sub_141D9, the F2 "MONSTER STATISTICS" clue-book detail panel
(called once by RunClueBookMonsterCategory, which redraws it whenever
dirty). A 362-line function drawing dozens of individually labeled
fields -- dumped the label messages, confirming a monster stat sheet:
EXPERIENCE:, GOLD:, MAGIC ORE:, NUORE: (loot -- consistent with
GrantMonsterRewards' 4 staged loot fields), HEALTH-, ACCURACY-,
DEXTERITY-, ABSORPTION-, DAMAGE-, RANGED ACC.-, RANGED DAM.- (combat
stats), and POISON:/DISEASE:/PARALYSIS:/FREEZING:/HEXING:/CURSING:/
FIRE:/COLD:/ELECTRIC:/POWER: (elemental/status resistances or
vulnerabilities). Individual field offsets into the loaded monster
record (fe, set up by LoadClueBookMonsterEntry) not traced this
round -- confidently the F2 detail screen given the content, but the
specific offset for each stat is a future-round task.

-> ShowClueBookMonsterDetail

Run via:
    .\run_ida_script.ps1 name_cluebook_monster_detail.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x141D9
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowClueBookMonsterDetail", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowClueBookMonsterDetail': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "F2 Monster Statistics detail panel (drawn by "
    "RunClueBookMonsterCategory). Labeled fields confirmed via message "
    "dump: EXPERIENCE:, GOLD:, MAGIC ORE:, NUORE: (loot), HEALTH-, "
    "ACCURACY-, DEXTERITY-, ABSORPTION-, DAMAGE-, RANGED ACC.-, "
    "RANGED DAM.- (combat), POISON:/DISEASE:/PARALYSIS:/FREEZING:/"
    "HEXING:/CURSING:/FIRE:/COLD:/ELECTRIC:/POWER: (resistances/"
    "vulnerabilities). Individual field offsets not traced yet.",
    False,
)
