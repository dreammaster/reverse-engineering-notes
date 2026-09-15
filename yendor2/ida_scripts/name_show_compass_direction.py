"""
Names sub_21530, called from RunAlchemyScreen (multiple sites). Gated
on word_328CA bit 0x1000 clear and word_36C7F bit 0x400 set (a
"compass active" mode, not otherwise identified), draws one of 4
tiered messages (9-byte stride table at 0x78CE) selected by
word_36CF5 bits 0x8000/0x4000/0x1000/none -- dumped as "   NORTH" /
"   SOUTH" / "    EAST" / "    WEST". A compass/facing-direction HUD
readout, drawn at the same screen position (0xF1,0x57) as
ShowMaterialCounterHud/RedrawPartyGoldDisplay.

-> ShowCompassDirection

Run via:
    .\run_ida_script.ps1 name_show_compass_direction.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x21530
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowCompassDirection", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowCompassDirection': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Compass/facing-direction HUD readout ('NORTH'/'SOUTH'/'EAST'/"
    "'WEST', selected by word_36CF5 tier bits), gated on word_328CA "
    "bit 0x1000 clear and word_36C7F bit 0x400 set. Drawn at the same "
    "screen position as ShowMaterialCounterHud/RedrawPartyGoldDisplay. "
    "Called from RunAlchemyScreen.",
    False,
)
