"""
Names sub_1119A and sub_11160, both called from HandleMovementInput
(the party's normal movement handler) and reused by
TryTravelToClickedMapCell -- two cell-type classification checks
shared between walking and click-to-travel.

sub_1119A (-> IsCellTypeImpassable): given a cell type (ax), returns
errorCode=3 if it falls in a blocking range (0x15-0x23, 0x25,
0x27-0x2A), else 0 (passable). A simple movement-blocking check.

sub_11160 (-> ClassifyFloorType): given a cell type (ax), returns
errorCode=2 for ax<=1 (a narrow low range -- plausibly open floor/
door), errorCode=1 for several other ranges (2-5, a `_val32.._val31`
band, 0xB-0xF, up to 0x39 -- plausibly special terrain requiring
different footstep/interaction handling), else 0 (normal floor).

Run via:
    .\run_ida_script.ps1 name_movement_cell_checks.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1119A: "IsCellTypeImpassable",
    0x11160: "ClassifyFloorType",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1119A,
    "Given a cell type (ax), returns errorCode=3 if blocking "
    "(0x15-0x23, 0x25, 0x27-0x2A), else 0. Called from "
    "HandleMovementInput and TryTravelToClickedMapCell.",
    False,
)
ida_bytes.set_cmt(
    0x11160,
    "Given a cell type (ax), classifies it into errorCode 0/1/2 "
    "(normal / special terrain / a narrow low range, plausibly open "
    "floor or door). Called from HandleMovementInput and "
    "TryTravelToClickedMapCell.",
    False,
)
