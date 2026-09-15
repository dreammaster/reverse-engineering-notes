"""
Names sub_20D2F and sub_20E12, a separate rendering pass from
RenderDungeonViewport/RenderDungeonViewRow -- called just before it by
both RedrawDungeonScreen and RefreshDungeonScreen, this pass draws the
sky/ceiling and floor backdrop, then extends the floor texture across
cells sharing the same floor type.

sub_20D2F (-> DrawDungeonFloorAndCeiling): reads the current cell's
type from the 0xE551 table, picks a ceiling picture (word_2E498, with
a special-case fallback to picture 1 when facing certain directions
and no explicit ceiling is set) and a floor picture (word_2E4A0),
draws both as fixed backdrop images (via DrawPicture + sub_29FF6, a
tiling helper, not traced), then calls sub_20E12 six times with the
same row-pointer/count pattern as RenderDungeonViewport.

sub_20E12 (-> ExtendDungeonFloorTexture): much simpler than
RenderDungeonViewRow -- for each cell in one row, if its type shares
the same floor picture as the current position (IsPairedValueMatch
against word_2E4A0), draws that floor picture at z-layer 1. A
"seamless floor" pass, not full wall/object rendering.

Run via:
    .\run_ida_script.ps1 name_dungeon_floor_ceiling.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x20D2F: "DrawDungeonFloorAndCeiling",
    0x20E12: "ExtendDungeonFloorTexture",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x20D2F,
    "Draws the dungeon backdrop: ceiling (word_2E498) and floor "
    "(word_2E4A0) pictures from the current cell's 0xE551 table entry, "
    "then calls ExtendDungeonFloorTexture 6x (same row-pointer pattern "
    "as RenderDungeonViewport) to extend the floor texture across "
    "matching cells. Called from RedrawDungeonScreen/"
    "RefreshDungeonScreen just before RenderDungeonViewport.",
    False,
)
ida_bytes.set_cmt(
    0x20E12,
    "For each cell in one row, draws the current floor picture "
    "(word_2E4A0) if the cell's type shares that same floor "
    "(IsPairedValueMatch) -- a seamless-floor pass, simpler than "
    "RenderDungeonViewRow's full wall/object rendering. Called 6x by "
    "DrawDungeonFloorAndCeiling.",
    False,
)
