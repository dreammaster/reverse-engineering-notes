"""
Round 1 of high-confidence-tier spot-checking: corrections and new
finds discovered by diffing each matched function's called-target
sequence between yendor2.asm and yendor3.asm. See
docs23/engine-diffs.md for the full writeup of what each of these
means and the evidence behind it.

- sub_28C1A -> ErrorCheck: BinDiff had weakly matched this address to
  FileEntry_Close (0.04 similarity) instead, which was wrong -- read
  side-by-side against yendor2's ErrorCheck, it's a byte-for-byte
  structural match (same dispatch-table jump, same cleanup-trio calls,
  same mouse-reset/video-mode/print/exit sequence), just using a raw
  ds:0x53E0 address instead of the (not-yet-created) errorCode symbol.
  This one rename fixes the apparent "ErrorCheck missing, replaced by
  FileEntry_Close" pattern seen across many functions in this round
  (InitGame, InitializeNewGameWorldState, SpawnMonsterInFacingDirection,
  TriggerSoundEvent, and more) -- they were never actually missing
  ErrorCheck, BinDiff just mislabeled its yendor3 address.
- sub_128F4 -> TryHandleCatalogSlotClick: confirmed the same function
  as yendor2's (RunShopScreen's catalog-slot-click handler: gate on
  HitTestCatalogSlot, bail if no hit or slot empty, else confirm the
  purchase), with a new feature inserted before the confirm prompt:
  LoadItemCatalogRecord + ComputeBarterPricingPreview, so the player
  now sees a barter-adjusted price preview before confirming. The
  *other* yendor2 call site for TryHandleCatalogSlotClick (from
  RunPartyInventoryScreen) now goes through a structurally different
  function in yendor3 (sub_144D4) with its own hit-test-table loop --
  not renamed here, needs its own identification later.
- sub_1A5C5 -> RefreshMultiStatEffects: a new function (no yendor2
  equivalent) called from UseTrainingItem. Walks a party record's two
  multi-stat-effect field ranges (offset 0x13A x6 @ 4-byte stride,
  0x152 x5 @ 2-byte stride) removing every currently-applied effect via
  RemoveMultiStatEffect, calls SyncPartyRecordStagedStats, then re-walks
  the same ranges reapplying via ApplyMultiStatEffectForItem -- a full
  stat-effect recalculation pass.
- dword 0xFA00 -> g_soundDriverFarPtr: confirmed via TriggerSoundEvent,
  which calls this exact address the same way yendor2's
  TriggerSoundEvent calls the already-named g_soundDriverFarPtr global
  (a far call through a function-pointer variable to the sound driver).

Run via:
    .\run_ida_script.ps1 apply_round1_corrections.py
"""
import idc
import ida_name

RENAMES = [
    (0x28C1A, "ErrorCheck"),
    (0x128F4, "TryHandleCatalogSlotClick"),
    (0x1A5C5, "RefreshMultiStatEffects"),
    (0xFA00, "g_soundDriverFarPtr"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
