"""
Names sub_25B14, a foundational, extremely widely-called function
(HandleMovementInput, BuildCombatTurnOrder, and dozens of other sites
throughout this session's work): resolves a 1-based party-record id
(ax) into the actual record pointer.

Sets word_328D6 = ax (the input id) and word_328D4 = 0 if ax==0, else
(ax-1)*0x1F4 + 0x95F3 -- confirming the party-member record table
base (0x95F3) and stride (0x1F4 = 500 bytes) already inferred from
DrawPartyMemberStatusPanel's per-slot lookup, and confirming
word_328D4/word_328D6 (used constantly throughout this session as
"the current party record pointer"/"its id") are set by this one
function.

-> SelectPartyRecordById

Run via:
    .\run_ida_script.ps1 name_resolve_party_record.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25B14
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SelectPartyRecordById", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SelectPartyRecordById': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Resolves a 1-based party-record id (ax) to its pointer: "
    "word_328D6=ax, word_328D4=(ax-1)*0x1F4+0x95F3 (or 0 if ax==0) -- "
    "the party-member record table, 500 bytes/record. Called "
    "extremely widely throughout the codebase.",
    False,
)
