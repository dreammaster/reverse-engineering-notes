"""
Names sub_1B2BD, called from UseItem (the "BUY "-named item branch,
bit 0xC000 of the item's [+0xE] flags). Selects and validates a party
target, caching the choice.

If no target is cached yet (word_36D03==0): clears the message box,
shows a confirm prompt (msg 0xA) to pick one; if declined, errorCode=1
(cancelled). Otherwise caches the pick (word_32990/word_36D03),
resolves the record, and checks status flags ([+0x1C] bits 0x1C40 --
the same incapacitated mask HandleRangedOrCombatAction's weapon scan
uses); if the target is incapacitated, shows a warning, clears the
cache, and loops back to prompt again.

-> ConfirmAndValidatePartyTarget

Run via:
    .\run_ida_script.ps1 name_confirm_validate_target.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1B2BD
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ConfirmAndValidatePartyTarget", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ConfirmAndValidatePartyTarget': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Selects and validates a party target (caching the pick in "
    "word_32990/word_36D03): confirm prompt if none cached yet, then "
    "rejects (warns and re-prompts) if the target is incapacitated "
    "([+0x1C] bits 0x1C40). Called from UseItem's 'BUY '-named item "
    "branch.",
    False,
)
