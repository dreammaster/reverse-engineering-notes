"""
Traced sub_2AF2E (item-icon-dispatch handler, word_32974==0x258) and
dumped its message strings -- a special potion that only works at one
specific map location.

Checks the player's exact position (word_36CF7==0x68, word_36CF9==0x6E
-- a single map cell). At that location: plays a sound, shows "THE
POTION WORKED SUCCESSFULLY", confirms item 0x258 is present
(IsItemRangeAvailable), then sets global flag 0x48 (SetGlobalFlag) --
a quest-milestone flag. Anywhere else: shows "YOU CAN NOT USE THAT
HERE!" instead.

-> UseLocationBoundPotion

Run via:
    .\run_ida_script.ps1 name_use_location_potion.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2AF2E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseLocationBoundPotion", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseLocationBoundPotion': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Item-icon-dispatch handler (word_32974==0x258). A potion that "
    "only works at one specific map cell (word_36CF7==0x68, "
    "word_36CF9==0x6E): there, shows 'THE POTION WORKED "
    "SUCCESSFULLY', confirms item 0x258 is present "
    "(IsItemRangeAvailable), and sets global quest flag 0x48 "
    "(SetGlobalFlag). Elsewhere: 'YOU CAN NOT USE THAT HERE!'.",
    False,
)
