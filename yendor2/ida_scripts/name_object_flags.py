"""
Follow-up to last round's global-flag family: the related per-object
flag-bit accessors, which compute their target word/mask relative to
a caller-supplied record pointer (si) instead of a fixed global base.

- sub_27A6E: si += 0x10C, then the same word/mask split
  GetGlobalFlagBitAndWord uses. -> GetRecordFlagBitAndWord_10C
- sub_27A3E: [si] |= mask (via the above). Traced a real caller
  (inside sub_1BBED, an item-use dispatcher branch gated on having
  enough of material 0x94B3): called with si=word_328D4 (the current
  party member) and ax=[a fixed item/quest record's +0x1A field] --
  i.e. this sets a flag in the *party member's own* record at +0x10C
  onward, plausibly per-character one-time-event flags (quest steps,
  items read/used, NPCs met). -> SetRecordFlag_10C

- sub_27AC1: si += 0xCA, same split, on a DIFFERENT fixed offset --
  presumably a different record type's flag bank (caller sub_27A4E's
  own context, from sub_1C123, not traced this round).
  -> GetRecordFlagBitAndWord_CA

Named on their offset since the exact record types/individual flag
meanings for both banks aren't confirmed -- deliberately not claiming
"party member" or "monster" in the name itself beyond what's evidenced
(only the +0x10C bank has a traced, party-member caller so far). The
matching Clear/Test accessors for these two banks weren't found this
round (one Clear-shaped code chunk exists as unreachable-looking
bytes at the very start of seg095 with no function boundary -- left
alone rather than force a name onto un-analyzed bytes).

Run via:
    .\run_ida_script.ps1 name_object_flags.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x27A6E: "GetRecordFlagBitAndWord_10C",
    0x27A3E: "SetRecordFlag_10C",
    0x27AC1: "GetRecordFlagBitAndWord_CA",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x27A6E,
    "Like GetGlobalFlagBitAndWord but relative to the caller's own "
    "si+0x10C instead of a fixed global base -- a per-record flag "
    "bank. Traced one caller (SetRecordFlag_10C, via sub_1BBED) using "
    "si=word_328D4 (current party member), suggesting this bank lives "
    "on the party-member record, plausibly per-character one-time-"
    "event flags. Not fully confirmed.",
    False,
)
ida_bytes.set_cmt(
    0x27A3E,
    "SetRecordFlag_10C(si=record, ax=flag index): [si+bank] |= mask "
    "via GetRecordFlagBitAndWord_10C. Real caller: sub_1BBED sets a "
    "flag on the current party member (si=word_328D4) using an index "
    "read from a fixed item/quest record's +0x1A field, inside an "
    "item-use branch gated on having >= some amount of material "
    "0x94B3.",
    False,
)
ida_bytes.set_cmt(
    0x27AC1,
    "Like GetGlobalFlagBitAndWord but relative to the caller's own "
    "si+0xCA -- a different per-record flag bank than "
    "GetRecordFlagBitAndWord_10C. Record type not confirmed (caller "
    "sub_27A4E, from sub_1C123, not traced).",
    False,
)
