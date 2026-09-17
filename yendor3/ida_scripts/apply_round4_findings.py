"""
Round 4: finishing the mid-confidence tier's last 11 functions, plus
resolving 2 of the earlier-deferred ones by direct reading. See
docs23/engine-diffs.md for the full writeup.

Straightforward same-function confirmations:
    ExtendDungeonFloorTexture, ShowHealingItemPercentInfo,
    ConfirmAndValidatePartyTarget, ClassifyFloorType, FadePaletteStep,
    TickStatusEffects, IsCellTypeImpassable, TickAilmentDuration

RunCharacterCreation gained 2 new setup calls before its original 4
(ComposeCharacterPortrait/PlayCharacterCreationIntroAnimation/
RunCharacterCreationSelectionStep/FinalizeCharacterCreation, all still
present) -- confirmed real via direct reading, named below:
- sub_2C2CA -> PlayCharacterCreationOpeningSetup: loads a new palette/
  picture pair, allocates and zeroes a ~64000-byte buffer, sets a
  couple of new flag bits, plays a music track. A new intro-visual
  setup step.
- sub_2BCD1 -> PlayCharacterCreationOpeningSequence: draws the picture
  from the setup above, then drives a two-phase sound+animation-frame
  loop (3 iterations then 5 more, incrementing sound-cue ids each
  time), checking a per-frame completion flag to bail early and
  polling via the function below between frames -- a new animated
  intro sequence played before the existing character-creation flow.

**Corrects a BinDiff match at 0.90 similarity** -- higher than any
other wrong match found this session, worth flagging prominently:
BinDiff labeled this address `PollForEscapeKeyOnlyAlt`, but its actual
body is `call WaitForSoundDriverIdle; if not idle or cx==0xFFFF,
return; else wait N ticks (cx) via the generic tick-wait primitive`.
Nothing to do with polling for Escape.
- sub_2C3B4 -> WaitForSoundDriverThenTicks

sub_2C2B9 (originally suggested as `WaitForTickAndDrawCreationFrame`)
is deliberately NOT renamed: reading it shows it's a bare tick-wait-
and-clear primitive with no frame-drawing call at all, matching the
shape of the already-named WaitForTickFlagAndClear/
WaitForTickFlagAndClearAlt overlay-segment duplicates rather than the
frame-drawing function BinDiff suggested. Yet another duplicate of
that same generic primitive, not confirmed to deserve its own distinct
name yet.

Run via:
    .\run_ida_script.ps1 apply_round4_findings.py
"""
import idc
import ida_name

RENAMES = [
    (0x202F7, "ExtendDungeonFloorTexture"),
    (0x1728F, "ShowHealingItemPercentInfo"),
    (0x16447, "ConfirmAndValidatePartyTarget"),
    (0x12EEA, "ClassifyFloorType"),
    (0x2C1E8, "FadePaletteStep"),
    (0x1FD70, "TickStatusEffects"),
    (0x12F13, "IsCellTypeImpassable"),
    (0x1EB7C, "TickAilmentDuration"),
    (0x2BB7C, "RunCharacterCreation"),
    (0x2C2CA, "PlayCharacterCreationOpeningSetup"),
    (0x2BCD1, "PlayCharacterCreationOpeningSequence"),
    (0x2C3B4, "WaitForSoundDriverThenTicks"),
]

for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
