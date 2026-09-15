"""
Followed DeductHPClamped's death-handling callee (also called from the
0x600-bit branch of ApplyEffectAndDrawIconBar) and found sub_1AB26: it
scans the 4 party slots (g_partySlotAssignment), skips invalid
(status-flag 0x1C40) members, and averages 3 fields across the valid
ones -- [+0x64], [+0x66], [+0x58] -- into word_36CA5/word_36CA7/
word_36CA9. The first average (word_36CA5) is compared against 5
ascending thresholds (0x2D/0x32/0x3C/0x46/0x50) to progressively set
bits in word_36C7F (0x400/0x8000/0x200/0x800/0x100); a final check
converts "lit but very low" (bit 0x4000 set, 0x8000 not) into "out"
(clears toward bit 0x1000, flags word_328C4 bit 0x400 as changed).

word_36C7F's bits are read right inside the minimap/dungeon-view code
(DrawMinimap/BuildMinimapTileData): bit 0x1000 skips the minimap draw
entirely and calls sub_2704C (the same "resource depleted" hook
SpendMaterialCounterClamped uses) instead; bit 0x2000 draws an
alternate view (sub_22402) instead of the normal minimap; bit 0x4000
gates whether BuildMinimapTileData gathers data at all; bit 0x400
(checked by a helper, sub_21530) shows one of several status messages
selected by word_36CF5's bits. This is consistent with a light-source/
torch-fuel system (word_36CA5, from party field +0x64) -- "out of
light" blanks the dungeon view -- but isn't fully confirmed (sub_22402
and sub_21530's exact messages aren't traced).

The other two averages have their own separate consumers, found but
not fully traced: word_36CA7 (party field +0x66) gates a 4-tier
overlay effect via sub_22402 in a weather/ambient-effect-looking
function (sub_28CFF); word_36CA9 (party field +0x58) gates
progressively-revealed detail icons in sub_234D3, a per-object info
display that reads as a bestiary/clue-book "identify" mechanic (three
2-bit icon-quality selectors on the object's own +0xC field, unlocked
as the party's average stat crosses 3 thresholds).

Named only the well-understood piece (the averaging+tiering function
itself); the party-record fields +0x64/+0x66/+0x58 and word_36C7F are
left unnamed/undocumented-per-bit pending more tracing of sub_22402/
sub_21530/sub_28CFF/sub_234D3 -- flagged as open leads instead of
guessed.

-> sub_1AB26 = UpdatePartyAverageStatTiers

Run via:
    .\run_ida_script.ps1 name_party_avg_tiers.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1AB26
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UpdatePartyAverageStatTiers", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UpdatePartyAverageStatTiers': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Averages 3 party-record fields across valid (non-dead/paralyzed) "
    "members: [+0x64] -> word_36CA5 (compared against 5 ascending "
    "thresholds to set tiered bits in word_36C7F -- consumed by "
    "DrawMinimap/BuildMinimapTileData, plausibly a light/torch-fuel "
    "level: bit 0x1000 blanks the dungeon view entirely), [+0x66] -> "
    "word_36CA7 (consumed by sub_28CFF, a 4-tier overlay effect, "
    "plausibly weather), [+0x58] -> word_36CA9 (consumed by "
    "sub_234D3, a per-object progressively-revealed-detail display, "
    "plausibly a bestiary/identify mechanic). None of the three field "
    "identities are confirmed -- see docs/file-formats.md.",
    False,
)
