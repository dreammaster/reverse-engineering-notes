"""
Names the F3/F4 "SPELLS"/"MAGIC USERS" clue-book category cluster
(F3 = all spells, F4 = a class picker then this same loop filtered by
class -- both routed through the same functions in ShowClueBook).

sub_13216 (-> RunClueBookSpellCategory): called from ShowClueBook (2
sites, F3 and F4). Loads the spell id (word_2E3EE[0]) into
word_3330A via sub_1D198, draws a message box + DrawClueBookNavBar,
calls sub_13B3F to draw the detail, loops on input until ESC
(sub_14D26 / word_2E40A) -- the same shape as
RunClueBookMonsterCategory/RunClueBookItemCategory.

sub_13B3F (-> ShowClueBookSpellDetail): the spell detail panel.
Message dump confirms labeled cost fields "MP:"/"NUORE:"/"ORE:" drawn
from word_332D2/332D4/332D6 via FormatNumber (spells cost MP plus the
same two alchemy ore counters used elsewhere), a "CLASS:"/"LEVEL:"
header, and "AFFECTS:"/"WHEN:"/"EFFECT:" section labels followed by
descriptive value strings (TRAINING, SCROLL, ALL, ONE, MONSTER,
CHARACTER, IN HAND TO HAND, IN A STRAIGHT LINE, IN A 3X3 AREA, ...).
Also draws a 6-class eligibility marker row (icons via sub_23BA4 or a
value via sub_13C86, selected by word_332FE bits) -- individual class
identities not traced.

Run via:
    .\run_ida_script.ps1 name_cluebook_spell_cluster.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x13216: "RunClueBookSpellCategory",
    0x13B3F: "ShowClueBookSpellDetail",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x13216,
    "F3 'SPELLS' / F4 'MAGIC USERS' clue-book category loop (called "
    "from ShowClueBook at 2 sites). Loads the spell id via sub_1D198, "
    "draws message box + nav bar + ShowClueBookSpellDetail, loops "
    "until ESC.",
    False,
)
ida_bytes.set_cmt(
    0x13B3F,
    "Spell detail panel: 'CLASS:'/'LEVEL:' header, 'MP:'/'NUORE:'/"
    "'ORE:' cost fields (word_332D2/332D4/332D6), 'AFFECTS:'/'WHEN:'/"
    "'EFFECT:' description sections, and a 6-class eligibility marker "
    "row (word_332FE bitmask). Called from RunClueBookSpellCategory.",
    False,
)
