"""
Names sub_1DA42, called from HandleRangedOrCombatAction (several
sites) with a message id in ax: if sub_2827E (not traced, plausibly
"is speech/sound currently playing") reports busy, just waits 6 ticks
instead of showing anything; otherwise, if ax is nonzero, shows the
message via sub_28412 (the message-display helper already used
throughout this session, e.g. "not enough room" messages).

-> ShowCombatMessageOrWait

Run via:
    .\run_ida_script.ps1 name_show_combat_message.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1DA42
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowCombatMessageOrWait", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowCombatMessageOrWait': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shows a combat message (ax, via sub_28412) unless sub_2827E "
    "reports speech/sound busy, in which case it just waits 6 ticks "
    "instead. Called from HandleRangedOrCombatAction.",
    False,
)
