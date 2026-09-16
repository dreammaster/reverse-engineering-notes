"""
Names sub_17A65, called from UseAbilityCommand: shows
ShowResourceDepletedOverlay, then -- unless word_32DCE bit 1 is set
(an early-out, meaning-unconfirmed) -- plays a sound, increments a
counter at [word_32DC4+2] (plausibly a charge/uses counter for the
ability being used, given word_32DC0/32DC2 are already known as
effect-id/threshold parameters for the same "use ability" flow feeding
ApplySavingThrowEffect), then refreshes the dungeon screen and redraws
the cursor. -> ConsumeAbilityChargeAndRefresh

Run via:
    .\run_ida_script.ps1 name_consume_ability_charge.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x17A65
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ConsumeAbilityChargeAndRefresh", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ConsumeAbilityChargeAndRefresh': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shows ShowResourceDepletedOverlay; unless word_32DCE bit 1 is set "
    "(early-out), plays a sound, increments [word_32DC4+2] (plausibly "
    "a charge/uses counter), then RefreshDungeonScreen + "
    "DrawMouseCursor. Called from UseAbilityCommand.",
    False,
)
