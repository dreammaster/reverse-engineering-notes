"""
Names sub_16EFA, called from sub_17032, UseAbilityCommand, and
RestPartyAndAdvanceClock: clears a VGA video-memory region (fill
pattern 0x0404) sized differently depending on combat state
(word_328CA bit 0x1000) -- a larger area in combat, a smaller one (at
two adjacent offsets, 0x78F0 and 0x6DB1) otherwise. A generic
"clear the message/status box background" utility reused across
several different action-confirmation flows.

-> ClearMessageBoxArea

Run via:
    .\run_ida_script.ps1 name_clear_message_box.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16EFA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ClearMessageBoxArea", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ClearMessageBoxArea': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clears a VGA video-memory region (fill 0x0404), sized by combat "
    "state (word_328CA bit 0x1000). Generic message/status-box clear "
    "reused by sub_17032, UseAbilityCommand, and "
    "RestPartyAndAdvanceClock.",
    False,
)
