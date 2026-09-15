"""
Names sub_1A3F0 and sub_1A4C5 -- the party travel/teleport system,
called from `start` and from the already-named ExamineTarget.

sub_1A3F0 (-> TravelToDestination): given a destination id (ax),
looks up a 16-byte-stride destination table at 0xD40B. If the
destination's [+0xE] flags have bit 0x2000 set, first checks
eligibility via sub_1A4C5 -- bails if not eligible. Otherwise shows an
optional message ([+6]), sets a music/mode flag bit in word_36C79
based on which of bits 0x8000/0x4000/0x1000 is set (0x4000 also calls
sub_1A582 and resets word_36CBF), sets the new position/facing
(word_36CF7/36CF9/36CF5 from [+0]/[+2]/[+4] -- with a hardcoded
override to (0x17A,0xE3) for one specific destination near (0x62,0x53)
within a coordinate range, if [+0xE] bit 0x800 is set), sets
word_36CB1/36CB3 from [+0xA]/[+0xC], then reveals cells around the
player and redraws the dungeon screen/minimap. The party
teleport/fast-travel handler.

sub_1A4C5 (-> IsDestinationUnlocked): looks up a 22-byte-stride
eligibility table at 0xDFBB by destination id (word_2E516, set by
TravelToDestination); if found, tests a flag word ([+2]) against a
bitmask ([+4]) -- if set, eligible (returns 1); else shows a
rejection message ([+8], if nonzero) and returns 0 (not eligible). A
"have you unlocked/discovered this destination" gate.

Run via:
    .\run_ida_script.ps1 name_travel_to_destination.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1A3F0: "TravelToDestination",
    0x1A4C5: "IsDestinationUnlocked",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1A3F0,
    "Party teleport/fast-travel handler: given a destination id, looks "
    "up table 0xD40B (16-byte stride), gates on IsDestinationUnlocked "
    "if flagged, sets a music/mode flag, sets new position/facing "
    "(with a hardcoded landing-spot override for one destination), and "
    "reveals/redraws the map. Called from `start` and ExamineTarget.",
    False,
)
ida_bytes.set_cmt(
    0x1A4C5,
    "Destination-eligibility gate: looks up table 0xDFBB (22-byte "
    "stride) by destination id (word_2E516); tests a flag word against "
    "a bitmask, returning eligible (1) or not (0, with an optional "
    "rejection message). Called from TravelToDestination.",
    False,
)
