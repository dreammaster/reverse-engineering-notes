"""
Round 11: resolved sub_156C9 -- the "small item-range-check loop"
flagged as unidentified since round 7. Its body is a verbatim
structural match to yendor2's `CheckAndTickAvailableAilment`
(identical cx-loop shape, identical `IsItemRangeAvailable` call
followed by a `g_currentActionId`-gated `TickStatusEffects` call,
identical early cx==0 return), and it's called from the same position
(right after `TickTravelResourceAilments`'s own body, with a matching
`cx = <counter global>; ax = '#' (0x23)` setup). Renamed.

Run via:
    .\run_ida_script.ps1 apply_round11_findings.py
"""
import idc
import ida_name

ea = 0x156C9
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CheckAndTickAvailableAilment", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CheckAndTickAvailableAilment': {'ok' if ok else 'FAILED'}")
