"""
Names sub_25ED1, sub_25F10, and sub_25E5E -- a major find resolving
the long-flagged "third stat bar" (+0x118/+0x56, previously "not
identified") in DrawPartyMemberStatusPanel's documentation.

sub_25F10 (0x25F10, called from sub_25B34): draws three labeled,
threshold-colored stat rows for the current character (si) --
dumping the message strings gives their exact labels: "HEALTH:"
(+0x52/+0x92, confirmed HP), "MAGIC:" (+0x54/+0x94, confirmed MP), and
"WEIGHT:" (+0x118/+0x56) -- CONFIRMING +0x118 is current carried
weight and +0x56 is max carry capacity (already suspected as a
Strength-derived stat via RollCharacterAttributes). If the character's
+0x1C bit 0x40 is set, shows "DEAD" instead of the HEALTH fraction --
CONFIRMING that bit as the character's dead/incapacitated flag.
-> DrawThreeStatBars

sub_25E5E (0x25E5E, called 3 times from DrawThreeStatBars): formats
two numbers via FormatNumber, applies some digit-trimming/adjustment
(sub_256F0, and conditionally sub_2570C+sub_16262 when word_2E4AC is
set), then joins them as "<num1>/<num2>" and writes it -- a generic
"draw a current/max fraction" utility. -> FormatAndDrawFraction

sub_25ED1 (0x25ED1, called from ShowLevelUpMessage and sub_25CFA too):
draws a fixed-width blank/padding label (msg 0x7955, 12 spaces) then
the character's name (+0x0) at a fixed position -- a small header draw
shared by several character-info screens. -> DrawCharacterNameHeader

Run via:
    .\run_ida_script.ps1 name_three_stat_bars.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x25F10, "DrawThreeStatBars"),
    (0x25E5E, "FormatAndDrawFraction"),
    (0x25ED1, "DrawCharacterNameHeader"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x25F10,
    "Draws 'HEALTH:' (+0x52/+0x92), 'MAGIC:' (+0x54/+0x94), and "
    "'WEIGHT:' (+0x118/+0x56 -- confirms carried weight / max carry "
    "capacity) as three threshold-colored stat rows. Shows 'DEAD' "
    "instead of the HEALTH fraction when +0x1C bit 0x40 is set -- "
    "confirms that bit as the dead/incapacitated flag. Called from "
    "sub_25B34.",
    False,
)
ida_bytes.set_cmt(
    0x25E5E,
    "Formats two numbers (FormatNumber + sub_256F0, optionally "
    "sub_2570C+sub_16262) and joins them as '<num1>/<num2>' for "
    "display. Called 3 times from DrawThreeStatBars.",
    False,
)
ida_bytes.set_cmt(
    0x25ED1,
    "Draws a fixed-width blank label (12 spaces) then the character's "
    "name (+0x0) at a fixed position. Shared header draw used by "
    "ShowLevelUpMessage, DrawThreeStatBars's caller chain, and "
    "sub_25CFA.",
    False,
)
