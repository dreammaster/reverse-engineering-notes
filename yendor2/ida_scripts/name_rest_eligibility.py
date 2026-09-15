"""
Names sub_1EA18 and sub_1A04B -- RestPartyAndAdvanceClock's "can the
party rest here" eligibility check and a shared position-list lookup.

sub_1EA18 (-> IsRestingAllowedHere): rejects (errorCode 1-4) if a
global flag is set (word_36C79 bit 1), the current map/level id
(word_34748) falls in specific forbidden ranges/values, or (falling
through to sub_1A04B) the current position is in a special list --
confirmed by RestPartyAndAdvanceClock's rejection message "YOU CAN NOT
REST HERE".

sub_1A04B (-> IsPositionInTriggerList): given the current cell
(es:si=word_328D2), scans an 8-byte-stride, 0xFFFF-terminated table at
0xD1C9 for a match (either the cell's x or y coordinate, selected per
entry by a flag bit) -- errorCode=1 if found. Also called from the
already-named ApplyMapTriggerEffect, suggesting this is a general
"is the party standing on one of these designated special cells"
check (map triggers, or here, a rest-forbidden zone).

Run via:
    .\run_ida_script.ps1 name_rest_eligibility.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1EA18: "IsRestingAllowedHere",
    0x1A04B: "IsPositionInTriggerList",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1EA18,
    "'Can the party rest here' check: rejects on a global flag "
    "(word_36C79 bit 1), forbidden map/level id ranges (word_34748), "
    "or a special-cell match (IsPositionInTriggerList). Confirmed by "
    "RestPartyAndAdvanceClock's rejection message 'YOU CAN NOT REST "
    "HERE'.",
    False,
)
ida_bytes.set_cmt(
    0x1A04B,
    "Scans an 8-byte-stride table (0xD1C9, 0xFFFF-terminated) for the "
    "current cell's x or y coordinate (selected per entry by a flag "
    "bit) -- a general 'is this a designated special cell' check. "
    "Called from ApplyMapTriggerEffect and IsRestingAllowedHere.",
    False,
)
