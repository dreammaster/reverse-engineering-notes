"""
Names sub_26C22, called twice from sub_2621C (a 230-line function
called from the main input loop, not traced this round). Checks
whether a currently-open container (selected via the [+0x15C] flag
bits and the [+0x17C]/[+0x1A2]/[+0x1C8] container-marker fields --
the same fields documented as the "3 alternate bags" system) is of a
type compatible with what the caller wants to place into it
(word_3293E, an allowed-type bitmask passed in). Rejects with
FlashStatusWarning + errorCode=1 if the container's own catalog type
flags don't match any bit in the allowed mask.

-> IsContainerTypeCompatible

Run via:
    .\run_ida_script.ps1 name_check_container_type.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x26C22
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "IsContainerTypeCompatible", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'IsContainerTypeCompatible': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Checks whether the currently-open container (one of the '3 "
    "alternate bags', selected via [+0x15C]/[+0x17C]/[+0x1A2]/[+0x1C8]) "
    "matches an allowed-type bitmask (word_3293E). Rejects with "
    "FlashStatusWarning if not. Called from sub_2621C.",
    False,
)
