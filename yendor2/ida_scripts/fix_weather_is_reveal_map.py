"""
CORRECTION: last round's file-formats.md/overview.md entries hedged
sub_28CFF as "plausibly weather, not traced" (the only evidence at the
time was a 4-tier overlay effect gated by a party-average stat). Read
the rest of the function this round and it's clearly wrong -- this is
a map-reveal special ability, not weather.

Full picture: sub_28CFF(ax=2..5, i.e. one of 4 special-ability slots)
first checks whether the party has that ability learned (a bitmask at
the current party member's +0xB4 field) and has enough charge/level
for it (a per-slot threshold table at 0x77C6 vs. 4 charge values at
+0xB6/+0xB8/+0xBA/+0xBC) -- new party-record fields, not previously
documented. If gated open, it computes a tier-sized (word_328FA x
word_32900, picked by the word_36CA7 party-average threshold from the
previous round) bounding box centered on the player's position
(word_36CF7/word_36CF9), then calls sub_291A3 per row, which reads
directly from WORLD.DAT (FileEntry bx=0x9043) and CURGAME (FileEntry
bx=0x8FFB) for each cell, walks the same bit-packed "explored" bitmap
PersistExploredCell writes, and calls sub_29259 (not traced) to
reveal/draw each newly-uncovered cell -- classic "reveal a radius of
the map around you" mechanics (Locate/Scout/Magic-Mapping-style),
scaled by the party's collective skill level. -> RevealMapRegion

sub_291A3 (the per-row worker) -> RevealMapRegionRow.

The exact class/spell name, the 0x77C6 threshold table, and
sub_28C94/sub_28CB1/sub_29259 (a further gating/drawing chain,
possibly related to the previously-flagged-but-unconfirmed "transport-
check table") are still open -- named only the two pieces whose role
is now solidly evidenced by the file I/O and explored-bitmap access.

Run via:
    .\run_ida_script.ps1 fix_weather_is_reveal_map.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x28CFF: "RevealMapRegion",
    0x291A3: "RevealMapRegionRow",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x28CFF,
    "CORRECTED from a 'plausibly weather' guess. Special ability "
    "(ax=2..5 selects one of 4 slots): gated on the party member's "
    "+0xB4 learned-ability bitmask and a per-slot charge/level "
    "threshold (0x77C6 table vs. party fields +0xB6/+0xB8/+0xBA/"
    "+0xBC). If open, computes a tier-sized box (word_328FA x "
    "word_32900, from the word_36CA7 party-average tier) centered on "
    "the player, then calls RevealMapRegionRow per row -- reads "
    "WORLD.DAT and CURGAME directly and walks the explored-cell "
    "bitmap (same one PersistExploredCell writes). Reads as a "
    "Locate/Scout/Magic-Mapping-style ability, not weather.",
    False,
)
ida_bytes.set_cmt(
    0x291A3,
    "Per-row worker for RevealMapRegion: reads a WORLD.DAT block and "
    "a CURGAME block (FileEntry 0x9043/0x8FFB), walks the bit-packed "
    "explored-cell bitmap byte-by-byte, and for each not-yet-explored "
    "cell that passes a further gate (sub_28C94/sub_28CB1, not "
    "traced -- possibly related to the unconfirmed 'transport-check' "
    "table) calls sub_29259 (not traced) to reveal it.",
    False,
)
