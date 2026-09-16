"""
Names sub_1E3AF and sub_1E340, the alchemy screen's spell-list drawer
-- the visual counterpart to BuildAlchemySpellList.

sub_1E3AF (0x1E3AF, called from RunAlchemyScreen): draws the current
page (word_33316) of up to 13 spell rows from buffer 0x565A -- each
entry's name (via LoadClueBookSpellEntry) colored by its castability
icon-state, with the currently-selected spell (word_3330E) highlighted
differently -- plus its MP/NUORE/MAGIC ORE costs (word_332D2/332D4/
332D6, the same fields CheckSpellCastability checks) via sub_1E340 at
3 fixed x-positions. -> DrawAlchemySpellList

sub_1E340 (0x1E340, called 3 times per row from DrawAlchemySpellList):
draws one cost value at the given x position. -> DrawSpellCostValue

Run via:
    .\run_ida_script.ps1 name_draw_alchemy_spell_list.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x1E3AF, "DrawAlchemySpellList"),
    (0x1E340, "DrawSpellCostValue"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1E3AF,
    "Draws the current page (13 rows) of the alchemy spell list "
    "(buffer 0x565A), each row's name colored by its castability "
    "icon-state and highlighted if selected, plus its MP/NUORE/MAGIC "
    "ORE costs via DrawSpellCostValue. Called from RunAlchemyScreen.",
    False,
)
ida_bytes.set_cmt(
    0x1E340,
    "Draws one spell cost value at the given x position. Called from "
    "DrawAlchemySpellList.",
    False,
)
