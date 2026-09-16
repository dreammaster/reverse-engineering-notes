"""
Names 5 functions in the EMS screen-cache family found while
investigating the top-ranked naming candidates (all share the
MapUnmapPages(dx=_emsPointer1?, bx=0x55D8) + rep movsw/stosw shape
already established for RestoreXFromEMS/SaveXToEMS siblings):

sub_223D4 (0x223D4, called from HandleMovementInput): restores the
*entire* video buffer from EMS page 0x55D8 (0x7D00 words = one full
VGA screen). -> RestoreFullScreenFromEMS

sub_191FC (0x191FC, called from the still-unnamed sub_1869D): restores
a large rectangular area from EMS page 0x55D8 (bx=0xA08, 0x88=136 rows
x 0x70=112 words/row, row stride 0x140) -- a big panel, exact identity
not confirmed. -> RestoreLargePanelFromEMS

sub_1B47A (0x1B47A, called from RunShopScreen and RefreshPartyPortraits):
does NOT restore -- it blanks (fills with a fixed 0x0404 pattern) the
*same* screen region in both the EMS page-0x55D8 cache and the live
video buffer (offset 0xC8F0, 0x23=35 rows x 0x24=36 words/row, stride
0xF8), at a location page 0x55D8 is already documented (via
RestorePortraitAreaAtPosition) as portrait-sized -- and one of its two
callers is literally RefreshPartyPortraits. -> ClearPortraitPanelAreas

sub_225F1 (0x225F1, called only from sub_22402): draws a fixed picture
(id = word_2E530, category 0x60 -- the same picture directory
DrawPartyMemberPortrait uses) at a fixed position (0xF8, 9).
-> DrawFixedStatusIcon

sub_22402 (0x22402, 6 call sites incl. `start`): restores a small area
from EMS page 0x55D8 (bx=0xAF0, 0x3A=58 rows x 0x25=37 words/row,
stride 0x140), then calls DrawFixedStatusIcon with picture id 0x15 --
restore-then-redraw for whatever fixed HUD icon that is (identity not
confirmed). -> RestoreAndRedrawFixedStatusIcon

Run via:
    .\run_ida_script.ps1 name_ems_restore_cluster.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x223D4, "RestoreFullScreenFromEMS"),
    (0x191FC, "RestoreLargePanelFromEMS"),
    (0x1B47A, "ClearPortraitPanelAreas"),
    (0x225F1, "DrawFixedStatusIcon"),
    (0x22402, "RestoreAndRedrawFixedStatusIcon"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x223D4,
    "Restores the entire video buffer from EMS page 0x55D8 (0x7D00 "
    "words = one full VGA screen). Called from HandleMovementInput.",
    False,
)
ida_bytes.set_cmt(
    0x191FC,
    "Restores a large rectangular area (136 rows x 112 words/row) from "
    "EMS page 0x55D8. Called from sub_1869D; exact panel identity not "
    "confirmed.",
    False,
)
ida_bytes.set_cmt(
    0x1B47A,
    "Blanks (fills with 0x0404) the same screen region in both the EMS "
    "page-0x55D8 cache and the live video buffer -- at a location page "
    "0x55D8 is elsewhere documented as portrait-sized, and one of the "
    "two callers is RefreshPartyPortraits.",
    False,
)
ida_bytes.set_cmt(
    0x225F1,
    "Draws a fixed picture (id=word_2E530, category 0x60 -- same "
    "directory DrawPartyMemberPortrait uses) at position (0xF8, 9). "
    "Called only from RestoreAndRedrawFixedStatusIcon.",
    False,
)
ida_bytes.set_cmt(
    0x22402,
    "Restores a small area from EMS page 0x55D8 (58 rows x 37 words/"
    "row), then redraws a fixed status icon (DrawFixedStatusIcon, "
    "picture id 0x15). Exact icon identity not confirmed. 6 call sites "
    "incl. `start`.",
    False,
)
