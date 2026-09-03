"""Name the ON..GOSUB arms in out.idb that fix_on_gosub_tables.py exposed.

Two dispatch groups sit inline in out_entry (jumped over by
`jmp loc_10181`) and are reached from doMovement / creatureApproach:

  direction group (selector ds:1E1E, from promptDirection):
      badCommand + N/E/S/W step deltas on ds:208A (Y) / ds:208C (X)

  region group (selector ds:214A == ds:2182, the raw map tile type
  under the player; index = tile+1, 1-based, 8 entries -> 5 presets):
      stores an encounter (freq, gate1, gate2) triple into
      ds:208E / ds:2092 / ds:2096.

    .\run_ida_script.ps1 -Idb out -ScriptName name_out_gosub_arms.py
"""
import idc
import ida_funcs
import ida_auto

RENAMES = {
    0x1007E: ("outCmd_bad",        "ON-cmd default: print \"bad command\""),
    0x10088: ("move_north",        "N: dec playerY (ds:208A)"),
    0x1008D: ("move_east",         "E: inc playerX (ds:208C)"),
    0x10092: ("move_south",        "S: inc playerY (ds:208A)"),
    0x10097: ("move_west",         "W: dec playerX (ds:208C)"),
    0x1009C: ("regionPreset_A",    "tile encounter triple: freq .51 / gate1 .22 / gate2 .40"),
    0x100CD: ("regionPreset_B",    "tile encounter triple: freq .67 / gate1 .25 / gate2 .50"),
    0x100FE: ("regionPreset_C",    "tile encounter triple: freq .90 / gate1 .35 / gate2 .55"),
    0x1012F: ("regionPreset_D",    "tile encounter triple: freq 1.25 / gate1 .40 / gate2 .60"),
    0x10160: ("regionPreset_E",    "tile encounter triple: freq .40 / gate1 -1.0 (no encounters); gate2 untouched"),
}


def main():
    for ea, (name, cmt) in RENAMES.items():
        if idc.get_wide_byte(ea) == 0xFF and idc.get_wide_byte(ea + 1) == 0xFF:
            print("  %#06x not code?" % ea)
        f = ida_funcs.get_func(ea)
        if f is None:
            ida_funcs.add_func(ea)
        idc.set_name(ea, name, idc.SN_NOWARN | idc.SN_CHECK)
        idc.set_func_cmt(ea, cmt, 0)
        print("  %#06x  %-16s  %s" % (ea, name, cmt))
    # selector annotations
    idc.set_cmt(0x117E2, "ON (tileType+1) GOSUB regionPreset_{B,A,B,C,D,C,D,E}"
                         "  -- selector ds:214A (raw map tile 0..7)", 0)
    ida_auto.auto_wait()


main()
