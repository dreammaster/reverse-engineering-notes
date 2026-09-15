"""
Names sub_111C1, called from ProcessLevelMonsters (2 sites): the
WORLD.DAT-backed counterpart to ClassifyObstacleAtViewportRow -- reads
a map cell directly from WORLD.DAT (via PrepareWorldDatRead +
FileEntry_Read, indexed by word_2E406/word_2E402) rather than the live
dungeon-viewport scratch buffer, then classifies it with the *exact
same* type-range logic ClassifyObstacleAtViewportRow uses (cell type
2-15, or side-feature 0x11-0x14/0x3F-0x43) into errorCode: 0=clear,
1=wall, 2=door/feature. Used by ProcessLevelMonsters to check whether
a monster's target cell (anywhere on the level, not just what's
currently rendered) is blocked before it moves there.

-> ClassifyObstacleAtWorldPosition

Run via:
    .\run_ida_script.ps1 name_classify_worlddat_cell.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x111C1
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ClassifyObstacleAtWorldPosition", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ClassifyObstacleAtWorldPosition': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "WORLD.DAT-backed counterpart to ClassifyObstacleAtViewportRow: "
    "reads a map cell from WORLD.DAT (word_2E406/word_2E402 index) "
    "and classifies it with the same type-range logic into errorCode "
    "(0=clear, 1=wall, 2=door/feature). Called from "
    "ProcessLevelMonsters to check a monster's target cell anywhere "
    "on the level, not just what's currently rendered.",
    False,
)
