"""
Names the two flag-row siblings of DrawClueBookMonsterStatRow, both
called from ShowClueBookMonsterDetail with a caller-set bx=label and
ax=bitmask: draw the label via writeString, then test the loaded
monster record's flag word at a fixed offset ([si+0x96] or [si+0x98])
against ax; if any bit matches, draw a fixed word at x=0x107.

sub_1496B: tests [si+0x96], draws "IMMUNE" (confirmed via string
dump at 0x89DF) when set. -> DrawClueBookMonsterImmuneFlagRow

sub_14A5E: tests [si+0x98], draws "RESISTANT" (confirmed via string
dump at 0x89E6) when set. -> DrawClueBookMonsterResistantFlagRow

Each called twice from ShowClueBookMonsterDetail, plausibly once per
damage-type category row (exact bit-to-damage-type mapping not
traced). -> DrawClueBookMonsterImmuneFlagRow / DrawClueBookMonsterResistantFlagRow

Run via:
    .\run_ida_script.ps1 name_draw_monster_immune_resistant_rows.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, off, msg in [
    (0x1496B, "DrawClueBookMonsterImmuneFlagRow", "0x96", "IMMUNE"),
    (0x14A5E, "DrawClueBookMonsterResistantFlagRow", "0x98", "RESISTANT"),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(
        ea,
        f"Draws a caller-supplied label, then tests the loaded monster "
        f"record's flag word at [si+{off}] against the caller's ax "
        f"bitmask; if any bit matches, draws '{msg}' at x=0x107. "
        f"Called twice from ShowClueBookMonsterDetail, sibling of "
        f"DrawClueBookMonsterStatRow.",
        False,
    )
