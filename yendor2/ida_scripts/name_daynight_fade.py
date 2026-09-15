"""
Traced sub_1FFE4, AdvanceGameClock's dawn/dusk event handler (fired
exactly at 6:00 AM and 6:00 PM). It's a gradual palette-fade trigger,
not an instant lighting flip -- a genuine day/night ambient lighting
system.

Guarded by word_3295A bit 0x2000 (already-fading guard). Sets up a
113-step transition (word_36DF5=0x71) through a palette-snapshot table
at 0x4A5C: at dusk (word_36D01==0x438) walks it backward from offset
0x14D by -3 bytes/step (word_36D91/word_36D93); otherwise (dawn) walks
it forward from 0 by +3/step. Each call copies one 96-byte chunk (32
RGB triples) from that table into a staging buffer (0x9535), then into
the actual VGA palette entries 0xE0-0xFF via SetPaletteRange -- the
last 32 palette slots, plausibly a dedicated sky/ambient-light color
ramp. Stops advancing (clears the 0x2000 guard) once the 113 steps are
exhausted. Also calls sub_2784A (not traced) each time.

-> AdvanceDayNightPaletteFade

Run via:
    .\run_ida_script.ps1 name_daynight_fade.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1FFE4
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "AdvanceDayNightPaletteFade", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'AdvanceDayNightPaletteFade': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "AdvanceGameClock's dawn/dusk handler (fired at exactly 6:00 AM/"
    "6:00 PM). Sets up (or continues) a gradual 113-step palette fade "
    "through a snapshot table at 0x4A5C -- backward from dusk, "
    "forward from dawn -- writing 32-RGB-triple chunks into VGA "
    "palette entries 0xE0-0xFF (the last 32 slots, plausibly a sky/"
    "ambient-light ramp) via SetPaletteRange. Guarded by word_3295A "
    "bit 0x2000 so it only initializes once per transition.",
    False,
)
