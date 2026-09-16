"""
Names sub_12624, called directly from `start` -- releases both EMS
(Expanded Memory) handles the game uses via INT 67h AH=0x45 (LIM EMS
"release handle and memory"): the primary handle (_emsPointer1?) and
a second one (word_2E504), each skipped if already 0xFFFF (not
allocated). Classic shutdown/cleanup routine.
-> ReleaseEmsHandles

Run via:
    .\run_ida_script.ps1 name_release_ems_handles.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x12624
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ReleaseEmsHandles", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ReleaseEmsHandles': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Releases both EMS handles (_emsPointer1?, word_2E504) via "
    "INT 67h AH=0x45, skipping each if already 0xFFFF. Shutdown "
    "cleanup. Called from start.",
    False,
)
