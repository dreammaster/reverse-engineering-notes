"""
Names sub_2D809, called from InteractWithContainer: shows the
held-item cursor as icon 0xF, calls ShowConfirmPrompt with message id
0x12 (a yes/no confirmation; ShowConfirmPrompt returns 5 on confirm
per its own pre-existing comment), stores the result in word_3331A and
the current slot selection (word_32924) in word_3331C for the caller
to act on, then restores the cursor. Reads as staging a confirmed
container interaction (result + target slot) for InteractWithContainer
to follow up on. -> ConfirmContainerInteraction

Run via:
    .\run_ida_script.ps1 name_confirm_container_interaction.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D809
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ConfirmContainerInteraction", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ConfirmContainerInteraction': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shows a yes/no confirm prompt (message id 0x12), storing the "
    "result in word_3331A and the current slot (word_32924) in "
    "word_3331C for the caller to act on. Called from "
    "InteractWithContainer.",
    False,
)
