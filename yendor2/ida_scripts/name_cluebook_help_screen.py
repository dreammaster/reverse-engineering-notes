"""
Names sub_14BD5, called directly from ShowClueBook (twice). Draws a
message box with title "** PRESS TAB AT ANY TIME TO SEE THIS SCREEN **"
and a 13-line body listing the clue book's categories:

  F1 MAPS (WORLD, TOWNS, MINES, ETC.)
  F2 MONSTER STATISTICS
  F3 SPELLS (INFORMATION ON ALL SPELLS)
  F4 MAGIC USERS (SPELLS BY CLASS)
  F5 INVENTORY ITEMS (ARMOR, POTIONS, FOOD, ETC.)
  F6 COMPLETE WALK THROUGH OF THE GAME
  ESC RETURN TO GAME

then draws DrawClueBookNavBar. This is the clue book's TAB help/index
screen -- and it identifies the real names of (at least 6 of)
DrawClueBookNavBar's 7 category tabs, previously undetermined.
-> ShowClueBookHelpScreen

Run via:
    .\run_ida_script.ps1 name_cluebook_help_screen.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x14BD5
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowClueBookHelpScreen", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowClueBookHelpScreen': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clue book TAB help/index screen (called from ShowClueBook). "
    "Title: '** PRESS TAB AT ANY TIME TO SEE THIS SCREEN **'. Body "
    "lists the categories: F1 Maps, F2 Monster Statistics, F3 Spells, "
    "F4 Magic Users (spells by class), F5 Inventory Items, F6 Complete "
    "Walk Through, ESC Return to Game -- identifying (at least 6 of) "
    "DrawClueBookNavBar's 7 category tabs.",
    False,
)
