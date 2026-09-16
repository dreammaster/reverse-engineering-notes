"""
Names two pieces of the dungeon/map object-lookup system, found while
tracing HandleGameCommand's callers in `start`'s main loop:

- sub_2186F: bounds-checks (ax, bx) against a map's valid coordinate
  range (word_32A00/32A02 = x max/min, word_32A08/32A0A = y max/min),
  then indexes a per-column array (word_2E55A, column = ax-0x28) to get
  a row of 6-byte entries, scans for one matching y=bx (0xFFFF
  terminates the row), and if found copies 3 words out to
  word_2E554/556/558 and returns si=0xCF4 (else si=0). This is a
  "what object is at this map position" lookup. -> FindObjectAtPosition
- sub_217ED: probes the map position facing the player (adjusts a
  coordinate by fixed deltas depending on facing-direction flags in
  word_36CF5) via FindObjectAtPosition, trying straight ahead first and
  a diagonal/adjacent fallback second; sets errorCode to 0/1/2
  depending on which probe (if either) found something.
  -> ProbeFacingTile

Run via:
    .\run_ida_script.ps1 name_map_lookup.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x2186F: "FindObjectAtPosition",
    0x217ED: "ProbeFacingTile",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x2186F,
    "Map object lookup: bounds-checks (ax=x, bx=y) against the current "
    "map's valid range, indexes a per-column array to a row of 6-byte "
    "entries, scans for y==bx (0xFFFF terminates). Found: copies 3 "
    "words to word_2E554/556/558, returns si=0xCF4. Not found/out of "
    "bounds: si=0.",
    False,
)
ida_bytes.set_cmt(
    0x217ED,
    "Probes the map position the player is facing (offsets the "
    "coordinate by fixed deltas per word_36CF5's direction flags) via "
    "FindObjectAtPosition, then an adjacent-tile fallback. errorCode: "
    "0=nothing found, 1=facing tile hit, 2=fallback tile hit.",
    False,
)
