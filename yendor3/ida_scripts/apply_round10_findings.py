"""
Round 10: fixed the long-standing "segmented addressing" global-rename
blocker (rounds 1-3), then used it to resolve HandleRangedOrCombatAction
-- one of the last two major open threads from the low-confidence tier.

**The blocker was a data-entry bug, not a real IDA limitation.** The
three addresses used in rounds 1-3 (0xFA00 for g_soundDriverFarPtr,
0xCF81 for g_partySlotAssignment, 0xCF63 for g_driverStateFlags) were
never valid linear addresses in *either* game -- 0xFA00/0xCF63 turned
out to be unrelated literal constants/raw unresolved `ds:`-relative
offsets misread as addresses, and 0xCF81 was simply the wrong offset.
The real yendor2 addresses (found via idc.get_name_ea_simple, where
they're already correctly named) are 0x3CC78 / 0x36E4B / 0x36CE5.

**The real yendor3 addresses**, found via `idc.to_ea(seg133.sel,
offset)` on the raw `ds:0NNNNh` operands still showing at their
reference sites (ShutdownAudioDrivers, HandleMovementInput -- both
already correctly named via the high-confidence bulk import, but
their DS-relative operands were never resolved to symbols because
IDA's `assume ds:seg133` from `start` doesn't propagate into other
code segments):
- g_driverStateFlags: raw offset 0xCF63 -> ea 0x3AD13
- g_soundDriverFarPtr: raw offset 0xFA00 -> ea 0x3D7B0
- g_partySlotAssignment: raw offset 0xD0C9 (NOT round 2's 0xCF81
  guess) -> ea 0x3AE79

Each was cross-validated two ways: (1) direct reference-site bit
masks/call patterns matching yendor2's usage exactly, and (2) a
relative-offset check -- yendor2's g_partyFacing/g_partyRoleAssignment3/
g_partySlotAssignment sit at fixed byte deltas from g_driverStateFlags
(+0x10/+0x22/+0x166), and yendor3's raw DS-offsets at the matching
code positions sit at the *exact same* deltas from 0xCF63 (+0x10 ->
0xCF73, +0x22 -> 0xCF85, +0x166 -> 0xD0C9) -- strong evidence this
near-data cluster's layout is byte-identical between the two games.

**HandleRangedOrCombatAction resolved**: with g_uiScratchFlags4's real
yendor3 address now needed, found it even more directly -- it's
already a resolved symbol (word_33120) inside `start`, where `assume
ds:` *is* tracked. Its two `test word_33120, 1000h` gates (yendor3
`start` lines ~799 and ~818) sit at the exact positions of yendor2's
two `test g_uiScratchFlags4, 1000h` / `call HandleRangedOrCombatAction`
sites, both calling the same `sub_1C13E` -- the address round 9 had
already flagged as a confirmed-bad `ComputeAlchemyRefinementYield`
match with a mysteriously huge combat/projectile call list. A direct
diff confirms it: 292/292 instructions (delta 0), call list identical
except for a few new `sub_286D8` sound-driver-hook calls (already a
documented Chapter 3 engine change) and one new `wait`. Renamed.

**Bonus find while tracing `start`**: yendor3's `sub_11E56` (round 9's
confirmed-bad `ErrorCheck` guess, empty call list) sits in the exact
`start` call slot yendor2 uses for `ParseCommandLineSwitches`, right
after the DS segment setup. Direct diff: calls_match=True. Renamed.

Open thread narrowed to just one: `DrawShadowedTextAlt`'s real
yendor3 identity. Its only caller in yendor2 is `PlayStudioCreditsIntro`
(5 call sites) -- but that caller's own yendor3 identity is *also*
still unresolved (round 7/9 confirmed-bad), so the caller-structure
trick can't be applied here without first finding the real
`PlayStudioCreditsIntro`, left for a future round.

Run via:
    .\run_ida_script.ps1 apply_round10_findings.py
"""
import idc
import ida_name
import ida_segment

seg = ida_segment.get_segm_by_name("seg133")

global_renames = [
    (0xCF63, "g_driverStateFlags"),
    (0xFA00, "g_soundDriverFarPtr"),
    (0xD0C9, "g_partySlotAssignment"),
]
for offset, name in global_renames:
    ea = idc.to_ea(seg.sel, offset)
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"offset={offset:#x} ea={ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

func_renames = [
    (0x1C13E, "HandleRangedOrCombatAction"),
    (0x11E56, "ParseCommandLineSwitches"),
]
for ea, name in func_renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
