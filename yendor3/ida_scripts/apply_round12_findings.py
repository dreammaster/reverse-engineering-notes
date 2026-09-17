"""
Round 12: investigated the address BinDiff suggested for
`RunCharacterDetailOverlay` (0x11778, 0.15 similarity) -- confirmed it
is NOT that function (the real `RunCharacterDetailOverlay` was already
found at 0x237BA in round 7). Traced the whole call chain it belongs
to and found it's part of a genuinely new Chapter 3 quest mechanic
with no yendor2 equivalent, called from `HandleScriptedStoryEventTrigger`
(the new 5-artifact quest event handler, round 6):

- `sub_1161C` (top-level entry, ds:53EEh=target item id,
  ds:53F0h=replacement item id): first checks a 6-entry shared table
  (`ds:0xCFF7`) for the target item and swaps in place if found;
  otherwise loops all 4 party members (via the now-named
  `g_partySlotAssignment`) calling `sub_116EF` on each, tallies the
  weight delta between old/new item via their catalog records
  (`ds:53F2`). `HandleScriptedStoryEventTrigger` calls this in a loop
  (item 0xC6 -> 0xCD) counting conversions -- reads as "convert every
  instance of item X the party is carrying into item Y," used for a
  quest-artifact-collection/transformation step. Renamed
  `SwapItemInstanceEverywhere`.
- `sub_116EF` (per-member worker): loops that member's 14-slot main
  inventory (`+0x11A`) plus one special equipped-item slot (`+0x13E`)
  for the target item id, removing/reapplying multi-stat effects on a
  match (`RemoveMultiStatEffect`/`ApplyMultiStatEffectForItem`/
  `RecomputeEquipmentStatBonuses`), or -- for non-matching *catalogued
  limited* items (catalog flag bit `0x2000`) -- delegates to
  `sub_11778`. Renamed `SwapItemInMemberInventory`.
- `sub_11778` NOT renamed -- its exact purpose is still uncertain.
  Manages an 8-slot table (base `ds:0xA5A6`, used widely elsewhere in
  the codebase for what's very likely an already-named global,
  cross-referenced but not pinned down this round) that looks like a
  registry of "limited/unique" item instances currently in play
  (matching the catalog's `0x2000` flag), including persisting it to
  disk via `FileEntry_Write`. Needs the `ds:0xA5A6` table's existing
  yendor2 identity found first before a confident name/verification is
  possible -- left as a documented open lead rather than guessed.

Run via:
    .\run_ida_script.ps1 apply_round12_findings.py
"""
import idc
import ida_name

renames = [
    (0x1161C, "SwapItemInstanceEverywhere"),
    (0x116EF, "SwapItemInMemberInventory"),
]

for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
