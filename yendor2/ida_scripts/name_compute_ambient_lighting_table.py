"""
Names sub_2784A, called from RedrawDungeonScreen (the core first-
person dungeon render) and AdvanceDayNightPaletteFade -- computes the
current ambient dungeon lighting table used by the render passes.

Two-stage selection into a 7-word working buffer (0x5086):

Stage 1 -- pick a base lighting record: if word_36C79 (the confirmed
global environmental-timer bits) has any of bits 4/2/1 set, copies a
fixed override record (one of 3, at 0x76CA/0x772C/0x773A). Otherwise
scans a day/night cycle table at 0x7228 (32-byte entries, 0xFFFF
terminator that wraps word_36D01 -- the confirmed clock/tick value --
back to 0) for the first entry whose threshold is >= word_36D01, and
copies its embedded 7-word lighting sub-record. This is the day/night
lighting-cycle lookup.

Then refines by facing/region: tests word_36CF5's facing-tier bits
(0x8000/0x4000/0x1000/0x2000) against a 9-entry table at 0x6E98
(indexed by word_329F0) to pick a region variant divisor.

Stage 2 -- weather darkening: tests several word_36C79 bit pairs
(rain/storm/fog-style flags) to select one of several delta rows from
a table at 0x76D8, then subtracts each delta from the corresponding
working-buffer entry (clamped to 0) -- darkens the lighting table
under adverse weather conditions.

Reads as: compute the dungeon's current ambient lighting gradient
table from time-of-day, weather, and facing/region, for the render
passes to consume. -> ComputeAmbientLightingTable

Run via:
    .\run_ida_script.ps1 name_compute_ambient_lighting_table.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2784A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ComputeAmbientLightingTable", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ComputeAmbientLightingTable': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Computes the dungeon's current ambient lighting table (working "
    "buffer 0x5086) from time-of-day (word_36D01 against a day/night "
    "cycle table at 0x7228), environmental override flags "
    "(word_36C79), and facing/region (word_36CF5 vs table 0x6E98), "
    "then darkens it under weather conditions via delta table "
    "0x76D8. Called from RedrawDungeonScreen and "
    "AdvanceDayNightPaletteFade.",
    False,
)
