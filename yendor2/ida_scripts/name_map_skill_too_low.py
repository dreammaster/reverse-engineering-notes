"""
Names sub_222BD, called from ShowLocalAreaMap and ToggleMapViewMode
(both already named): switches the draw target to the video buffer
and shows a 3-line message, dumped as "YOUR SKILL IS NOT HIGH ENOUGH!"
(msg 0x7D7E) -- a mapping/cartography skill gate.

-> ShowMapSkillTooLowMessage

Run via:
    .\run_ida_script.ps1 name_map_skill_too_low.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x222BD
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowMapSkillTooLowMessage", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowMapSkillTooLowMessage': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "'YOUR SKILL IS NOT HIGH ENOUGH!' (msg 0x7D7E, cx=3) -- a mapping/"
    "cartography skill gate rejection, called from ShowLocalAreaMap "
    "and ToggleMapViewMode.",
    False,
)
