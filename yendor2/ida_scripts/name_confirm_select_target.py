"""
Names sub_2AD94, called from ApplyMultiStatEffect and RestCharacter:
shows a confirm prompt (msg 0x12 via ShowConfirmPrompt). If declined,
refreshes the material/gold HUD and returns 0 (cancelled). If
confirmed, stores the selection into word_32990 (a party-record id,
same field sub_1B2BD/ShowConfirmPrompt-style flows use), resolves it
to a record pointer (sub_25B14), and returns word_328D6.

-> ConfirmAndSelectPartyTarget

Run via:
    .\run_ida_script.ps1 name_confirm_select_target.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2AD94
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ConfirmAndSelectPartyTarget", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ConfirmAndSelectPartyTarget': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shows a confirm prompt (msg 0x12); if declined, refreshes the "
    "material/gold HUD and returns 0. If confirmed, resolves the "
    "selected party record (word_32990 -> sub_25B14) and returns "
    "word_328D6. Called from ApplyMultiStatEffect and RestCharacter.",
    False,
)
