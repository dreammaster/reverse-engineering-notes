"""
Names sub_1E1A7 and sub_1E285, the alchemy screen's spell-list builder
and per-spell castability check.

sub_1E1A7 (0x1E1A7, called from RunAlchemyScreen): iterates word_3330C
spell ids through an eligibility check (sub_27A66) to build a filtered
list of known spells (id + icon-state 6) into buffer 0x565A. Unless
the character is incapacitated (+0x1C bits 0x1C40), calls
CheckSpellCastability (was sub_1E285) on each listed spell to update
its icon state. Computes pagination (13 entries/page) and finds the
current selection (+0xC8) in the list, defaulting to page 1.
-> BuildAlchemySpellList

sub_1E285 (0x1E285, called from BuildAlchemySpellList): loads the
spell's data (LoadClueBookSpellEntry), then checks context-gating
flags, and whether the character has enough NUORE (0x94BB), MAGIC ORE
(0x94B7), and MP (+0x54 vs the spell's MP cost) -- if every check
passes, sets the spell's icon-state to 0xF (castable), else leaves it
at the default 6. -> CheckSpellCastability

Run via:
    .\run_ida_script.ps1 name_alchemy_spell_list.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x1E1A7, "BuildAlchemySpellList"),
    (0x1E285, "CheckSpellCastability"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1E1A7,
    "Builds the alchemy screen's filtered list of known spells "
    "(sub_27A66 eligibility check) into buffer 0x565A, calling "
    "CheckSpellCastability on each (unless incapacitated), then "
    "computes pagination (13/page) and locates the current selection. "
    "Called from RunAlchemyScreen.",
    False,
)
ida_bytes.set_cmt(
    0x1E285,
    "Loads the spell (LoadClueBookSpellEntry) and checks context-gating "
    "flags plus NUORE (0x94BB)/MAGIC ORE (0x94B7)/MP (+0x54) "
    "affordability; sets the spell's icon-state to 0xF if all pass. "
    "Called from BuildAlchemySpellList.",
    False,
)
