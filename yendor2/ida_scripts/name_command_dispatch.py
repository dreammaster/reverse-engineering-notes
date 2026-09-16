"""
Names sub_295A8, fully read this round: the core gameplay command
dispatcher. Called directly from `start` (repeatedly, presumably once
per input event). Dispatches on word_32974 (already-decoded command
code, set elsewhere) across ~20 specific handlers -- RunConversation,
the item-icon dispatcher sub_2AE3C, and many others -- then, for
command codes that don't match anything specific, falls back to
*context-sensitive interaction*: inspects flag bits on the record
pointed to by word_2E548 (the currently-targeted object -- NPC, item,
container, whatever's under the cursor/player) to decide what action
fits: conversable -> RunConversation, container-like flags -> sub_2D65A,
another object-type flag -> sub_2D60A, otherwise falls through to the
item-icon dispatcher. This is the classic "player pressed something,
figure out what it means for whatever's currently targeted" pattern --
matches the manual's "SPACE uses the space you are standing on".

-> HandleGameCommand

Run via:
    .\run_ida_script.ps1 name_command_dispatch.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x295A8
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleGameCommand", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleGameCommand': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Core gameplay command dispatcher, called from `start`. Dispatches "
    "on word_32974 (an already-decoded command code) across ~20 "
    "specific handlers. For codes that don't match anything specific, "
    "falls back to context-sensitive interaction with the "
    "currently-targeted object (word_2E548): conversable flags -> "
    "RunConversation, container-like flags -> sub_2D65A, another "
    "object-type flag -> sub_2D60A, else falls through to the "
    "item-icon dispatcher sub_2AE3C. Matches the manual's 'SPACE uses "
    "the space you are standing on'.",
    False,
)
