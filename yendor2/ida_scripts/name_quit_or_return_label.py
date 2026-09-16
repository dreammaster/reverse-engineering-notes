"""
Names sub_25595, called from ShowCharacterSkills and
ShowCharacterInventory: draws a fixed bottom-left label, toggling
between two messages based on word_328CA bit 0x8000 -- 'QUIT "CREATE"'
(default, highlighting the 'Q') when clear, or "RETURN" (highlighting
its 2nd character) when set. Reads as a shared exit-label for
character-info screens reused during character creation: "QUIT
CREATE" when still mid-chargen (abandon character creation), "RETURN"
when viewing an already-created character's sheet (just go back).
-> DrawQuitOrReturnLabel

Run via:
    .\run_ida_script.ps1 name_quit_or_return_label.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25595
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawQuitOrReturnLabel", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawQuitOrReturnLabel': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws a fixed bottom-left exit label: 'QUIT \"CREATE\"' (default) "
    "or 'RETURN' (when word_328CA bit 0x8000 is set) -- shared by "
    "character-info screens reused during character creation vs. "
    "viewing an existing character. Called from ShowCharacterSkills "
    "and ShowCharacterInventory.",
    False,
)
