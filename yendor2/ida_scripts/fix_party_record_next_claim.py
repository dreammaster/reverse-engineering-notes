"""
CORRECTION: file-formats.md's party-member record section has said
since early in the session that word_328D4 is "traversed via a +0x10
'next' link". Went looking for that traversal while documenting the
new g_partyRecords array (previous round) and couldn't find it
anywhere -- grepped every `[si+10h]` write near the ShowPartyMembers
loop and found only unrelated uses (e.g. ShowPartyMembers+0x21 sets
[si+0x10]=1 as a parameter for ShowCharacterEquipment, not a link
write). The "+0x10 next link" description turns out to trace back to a
comment that was attached to the wrong call site: it was describing
ShowPartyMembers' own per-member pipeline (ShowCharacterSkills ->
Equipment -> Stats -> Inventory -> EditCharacterName -> Summary), but
got attached to its call to sub_25862 (a small, unrelated, generic
screen helper also called from ShowClueBook, with no party-related
code in its body at all).

Traced how word_328D4 actually gets set between ShowPartyMembers'
loop iterations instead: sub_25544 (called at loop start and after
each member) calls sub_250E5, which linearly scans the newly-named
g_partyRecords array (base 0x95F3/linear 0x36E53, stride 0x1F4,
cx=9 -- confirms up to 9 slots exist in the record array, more than
the 4 UI/effect slots from the previous round) for the first record
whose +0xE field (RestCharacter's "time-of-day-like value") is zero,
and sets word_328D4 to that address (or 0 if none found). Whether
+0xE==0 really means "unused/empty slot" or something else isn't
fully confirmed -- named the function on its mechanism, not a
guessed interpretation.

Fixes:
  - file-formats.md: replace the wrong "+0x10 next link" claim.
  - Moves the stale/misattached pipeline-description comment off
    ShowPartyMembers's call to sub_25862 and onto ShowPartyMembers
    itself, where it actually belongs.
  - Names sub_250E5 -> SelectDefaultPartyRecord.

Run via:
    .\run_ida_script.ps1 fix_party_record_next_claim.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x250E5
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SelectDefaultPartyRecord", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SelectDefaultPartyRecord': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Linearly scans g_partyRecords (base 0x95F3, stride 0x1F4, up to "
    "9 slots) for the first record whose +0xE field is 0, and sets "
    "word_328D4 to it (0 if none found). Whether +0xE==0 means "
    "'unused slot' or something else isn't confirmed -- named on "
    "mechanism, not a guessed interpretation. Called by sub_25544 to "
    "establish/refresh word_328D4 between ShowPartyMembers iterations.",
    False,
)

# The misattached comment describing ShowPartyMembers' own pipeline is
# on the CALL instruction to sub_25862 (ShowPartyMembers' first real
# instruction), not on the function entry point itself. Find it by
# scanning forward from the function start for the stale text.
spm_ea = idc.get_name_ea_simple("ShowPartyMembers")
print(f"ShowPartyMembers @ {spm_ea:#x}")

func_end = idc.find_func_end(spm_ea)
cur = spm_ea
misplaced_ea = None
while cur != idc.BADADDR and cur < func_end:
    c = idc.get_cmt(cur, False)
    if c and "Iterates the party-member list" in c:
        misplaced_ea = cur
        break
    cur = idc.next_head(cur, func_end)

PIPELINE_CMT = (
    "Iterates the party roster: sub_25544/SelectDefaultPartyRecord "
    "establish word_328D4 between iterations (a linear scan of "
    "g_partyRecords, NOT a '+0x10 next' link field -- that description "
    "was wrong, see fix_party_record_next_claim.py), then runs a "
    "pipeline of per-member display steps (ShowCharacterSkills -> "
    "ShowCharacterEquipment -> ShowCharacterStats -> "
    "ShowCharacterInventory -> EditCharacterName -> "
    "ShowCharacterSummary) -- 'Q' aborts at any stage. "
    "RunTitleScreen's only caller (its 'C' option) -- resolves 'C' as "
    "viewing the party's characters."
)

if misplaced_ea is not None:
    print(f"found stale comment at {misplaced_ea:#x}, clearing it")
    ida_bytes.set_cmt(misplaced_ea, "", False)
else:
    print("stale comment not found by scan (may already be gone)")

ida_bytes.set_cmt(spm_ea, PIPELINE_CMT, False)
print("set corrected pipeline comment on ShowPartyMembers itself")
