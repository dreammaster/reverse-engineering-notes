"""
Names sub_255C7, called from sub_23C18 (multiple sites, not traced): a
generic "wait for a click or ESC" loop, ticking UpdateAmbientMusic
each iteration. Loops on PollKeyboardInput until errorCode is 1 (ESC,
returns ax=0xFFFF), 3, or 7 (both call HitTestRegionTable against a
region table selected by which key/click type fired -- returning the
hit-test result in ax).

-> WaitForClickOrEscape

Run via:
    .\run_ida_script.ps1 name_wait_click_or_escape.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x255C7
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "WaitForClickOrEscape", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'WaitForClickOrEscape': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Generic 'wait for a click or ESC' loop, ticking UpdateAmbientMusic "
    "each iteration. Returns ax=0xFFFF on ESC, else the "
    "HitTestRegionTable result for the click. Called from sub_23C18.",
    False,
)
