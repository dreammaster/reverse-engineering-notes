"""
Names sub_1A5CC, moderate confidence: plays a sound effect
(PlaySoundEffect-style call, ax=3), briefly shows a picture (id 0xE,
saving/restoring the previous picture id around it) for 7 ticks, then
restores. Called from HandleGameCommand when a status effect is
already active (word_36C79 bit1 set, the flag ApplyStatusEffect sets
and TickStatusEffects clears) -- reads as a periodic warning flash for
an ongoing status effect. -> FlashStatusWarning

Run via:
    .\run_ida_script.ps1 name_flash_warning.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1A5CC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "FlashStatusWarning", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'FlashStatusWarning': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Moderate confidence: plays a sound, briefly shows picture id 0xE "
    "for 7 ticks (saving/restoring the previous picture id), then "
    "restores. Called from HandleGameCommand when a status effect is "
    "already active -- a periodic warning flash.",
    False,
)
