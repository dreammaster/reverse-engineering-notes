"""
Round 5 continued: a second batch of the low-confidence tier. This
batch turned up far more confirmed-bad BinDiff matches than good ones
(12 of ~20 checked) -- see docs23/engine-diffs.md for the full list of
what's bad and why. Only the confirmed-real ones are renamed here.

- ShowStatusPanelMessage, ClassifyObstacleAtWorldPosition,
  ApplyStatusEffect: clean confirmations (calls match).
- TickPartyAilmentIconBar: real match, but restructured -- the
  individual TickDiseasePoisonSickAilmentSlot/TickCurseHexJinxAilmentSlot
  calls appear consolidated behind a new helper (sub_1B085, not yet
  examined), similar to round 5's TickTravelResourceAilments
  consolidation.
- PlayCharacterCreationIntroAnimation: real match (confirmed via a
  legitimate WaitForSoundDriverThenTicks call, matching round 4's
  finding that this primitive paces character-creation animations),
  heavily restructured/shortened (124->104 instructions). One resolved
  call in the diff (InstantKillActiveMonster) is noise from that
  address's own bad match, not a real call to that function.

Run via:
    .\run_ida_script.ps1 apply_round5b_findings.py
"""
import idc
import ida_name

RENAMES = [
    (0x236B5, "ShowStatusPanelMessage"),
    (0x12F2B, "ClassifyObstacleAtWorldPosition"),
    (0x1FDFE, "ApplyStatusEffect"),
    (0x1AF19, "TickPartyAilmentIconBar"),
    (0x2BDB0, "PlayCharacterCreationIntroAnimation"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
