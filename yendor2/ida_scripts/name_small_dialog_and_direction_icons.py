"""
Names 3 small, single-purpose picture-draw helpers.

sub_1B8AB (called from UseItem and sub_1A5F6): draws a small
dialog-box frame (picture id 3, icon slot 0x10) at (0xF,0x17), then
the currently-processed item's own icon (id from 0xBCE+0, icon slot
0x70) at (0x15,0x1F) inside it -- an item-use confirmation dialog
showing the item's icon. -> DrawItemUseConfirmDialog

sub_1E522 (called twice from RunAlchemyScreen): draws the same
dialog-box-frame picture at the same position (0xF,0x17), but with
id 4 instead of 3 and no icon overlay -- the alchemy screen's own
confirm-dialog background. -> DrawAlchemyConfirmDialogBackground

sub_293C0 (called once from RevealMapRegion): draws a small facing-
direction icon (picture id 0-3, selected by word_36CF5's facing-tier
bits -- the same bits DrawMinimapCompassIcon remaps for the minimap
compass) at (0x74,0x48). -> DrawRevealMapDirectionIcon

Run via:
    .\run_ida_script.ps1 name_small_dialog_and_direction_icons.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, desc in [
    (0x1B8AB, "DrawItemUseConfirmDialog",
     "Draws a small dialog-box frame (picture 3) at (0xF,0x17), "
     "then the current item's own icon (0xBCE+0) at (0x15,0x1F) "
     "inside it -- an item-use confirmation dialog. Called from "
     "UseItem and sub_1A5F6."),
    (0x1E522, "DrawAlchemyConfirmDialogBackground",
     "Draws the same dialog-box-frame picture (id 4) at the same "
     "position (0xF,0x17) as DrawItemUseConfirmDialog, with no icon "
     "overlay -- the alchemy screen's confirm-dialog background. "
     "Called twice from RunAlchemyScreen."),
    (0x293C0, "DrawRevealMapDirectionIcon",
     "Draws a small facing-direction icon (picture 0-3, selected by "
     "word_36CF5's facing-tier bits, the same bits "
     "DrawMinimapCompassIcon remaps) at (0x74,0x48). Called once "
     "from RevealMapRegion."),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)
