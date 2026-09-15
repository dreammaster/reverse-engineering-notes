"""
Traced sub_12ECD -- a ~16-way dispatcher on word_2E3F6 (a validation-
failure-type selector), each branch calling a different lookup/check
(state 1 calls the newly-named CheckWorldDatCompatibility; state 2
calls sub_14B85; etc.) and building a detail string via StrCat before
jumping to a common tail. Reads as composing a detailed error message
for whichever specific save/load validation failure occurred (level
mismatch, map mismatch, and others not individually traced).

-> BuildLoadValidationMessage

Run via:
    .\run_ida_script.ps1 name_build_load_error_message.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12ECD
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "BuildLoadValidationMessage", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'BuildLoadValidationMessage': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Dispatches on word_2E3F6 (a validation-failure-type selector, "
    "~16 states) to compose a detailed error message for a specific "
    "save/load validation failure -- state 1 calls "
    "CheckWorldDatCompatibility (level/map mismatch); other states "
    "call different checks (sub_14B85, etc.), not individually "
    "traced. Each builds its detail text via StrCat before a common "
    "tail.",
    False,
)
