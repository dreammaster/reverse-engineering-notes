"""
Names two functions in the clue book's "F2 MONSTER STATISTICS"
category (paralleling the already-named F5 Items cluster:
RunClueBookItemCategory/ShowClueBookItemDetail):

sub_1466E (-> LoadClueBookMonsterEntry): given the current entry id
(word_2E3EE[0]), reads WORLD.DAT block 0x32 (WorldDat_setBlock5(ax=
0x32)) into a freshly allocated buffer (fe), zeroing it first. Sets up
a couple of display-variant fields ([+2]/[+4]/[+0xA]) based on a flag
bit at [+0x92] (a monster-record-shaped field, consistent with the
combat monster-slot layout documented elsewhere), then draws the clue
book nav bar.

sub_132B5 (-> RunClueBookMonsterCategory): called directly from
ShowClueBook, structurally identical to RunClueBookItemCategory but
simpler (no region-table hit-testing): calls LoadClueBookMonsterEntry
once, then loops redrawing (sub_141D9 + DrawClueBookNavBar) whenever
word_328C4 bit 0x400 (dirty) is set, until ESC (sub_14D26 sets
word_2E40A).

sub_141D9 itself (the actual monster-stats detail panel, a 362-line
function drawing dozens of individually labeled fields) is left
unnamed this round -- confidently "the F2 detail screen" but its
individual stat fields aren't traced.

Run via:
    .\run_ida_script.ps1 name_cluebook_monster_category.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1466E: "LoadClueBookMonsterEntry",
    0x132B5: "RunClueBookMonsterCategory",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1466E,
    "F2 'MONSTER STATISTICS' clue-book entry loader: reads WORLD.DAT "
    "block 0x32 for the current entry (word_2E3EE[0]) into a fresh "
    "buffer, sets a couple of display-variant fields from a flag at "
    "[+0x92], draws the nav bar. Called once by "
    "RunClueBookMonsterCategory.",
    False,
)
ida_bytes.set_cmt(
    0x132B5,
    "F2 'MONSTER STATISTICS' clue-book category loop (called from "
    "ShowClueBook), structurally identical to RunClueBookItemCategory "
    "minus the region-table hit-testing: LoadClueBookMonsterEntry once, "
    "then redraw (sub_141D9 -- the detail panel, not yet named) + "
    "DrawClueBookNavBar when dirty, until ESC.",
    False,
)
