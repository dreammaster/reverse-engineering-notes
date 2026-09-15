"""
Names sub_15267: the finalize/cleanup step of character creation,
called from RunCharacterCreation's loc_15245 regardless of which step
was reached (ESC-cancelable at each of the 3 prior steps, this always
runs). RunCharacterCreation's own pre-existing comment already
described it as "a finalize step" -- this just gives it a name.

Loads a transition palette (LoadMasterPalette, 0x442A), reads file
entry #3, frees a temp memory block (word_328BE) via DOS int 21h/49h if
allocated, clears the screen (sub_152E1, full black-fill), and -- if
the driver/music flags say so -- stops the character-creation music
and waits ~20 ticks before returning to the caller (RunTitleScreen or
InitGame). -> FinalizeCharacterCreation

Run via:
    .\run_ida_script.ps1 name_finalize_char_creation.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x15267
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "FinalizeCharacterCreation", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'FinalizeCharacterCreation': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Character creation's finalize/cleanup step (always runs, even on "
    "ESC-cancel from any of the 3 prior steps -- see RunCharacterCreation). "
    "Loads a transition palette, reads file entry #3, frees a temp memory "
    "block if allocated, clears the screen, and stops the "
    "character-creation music before returning.",
    False,
)
