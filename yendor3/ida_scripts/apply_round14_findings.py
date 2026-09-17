"""
Round 14: resolved the character-creation-region cluster flagged since
round 9 (0x2BD4A/0x2BBB5/0x2BC75, all near the confirmed
`RunCharacterCreationSelectionStep` at 0x2BF44).

Read `RunCharacterCreation`'s full yendor3 call sequence directly:
`PlayCharacterCreationOpeningSetup` -> `sub_2BC75` ->
`PlayCharacterCreationOpeningSequence` -> `sub_2BD4A` ->
`PlayCharacterCreationIntroAnimation` -> `RunCharacterCreationSelectionStep`
-> `FinalizeCharacterCreation`. Notably, yendor2's `ComposeCharacterPortrait`
does NOT appear anywhere in this sequence -- it looks to have been
dropped/replaced by extra opening-screen content instead:

- `sub_2BC75`: draws picture id 8, then an escape-pollable wait loop --
  no sound sequence. Same "one static picture + wait" shape as
  `PlayCharacterCreationOpeningSetup`, positioned right after it.
  Renamed `PlayCharacterCreationOpeningPicture`.
- `sub_2BD4A`: draws picture id 0xA, then a `TriggerSoundEvent`/
  `WaitForSoundDriverThenTicks` sound-cue loop, byte-for-byte the same
  shape as `PlayCharacterCreationOpeningSequence` right before it (just
  different picture/sound ids and loop counts) -- a near-duplicate,
  matching this project's established `...Alt` naming convention for
  structural clones. Renamed `PlayCharacterCreationOpeningSequenceAlt`.
  (Initially guessed this might be a rewritten `ComposeCharacterPortrait`
  since it sits in that general position -- ruled out once the full
  body was read: nothing here resembles portrait composition, it's
  just another title-card screen.)
- `sub_2BBB5` (the escape-poll gate both of the above call repeatedly):
  byte-for-byte identical to yendor2's `PollForEscapeKeyOnly` (poll
  keyboard, discard anything but ESC, return ZF on ESC). Renamed.

Run via:
    .\run_ida_script.ps1 apply_round14_findings.py
"""
import idc
import ida_name

renames = [
    (0x2BC75, "PlayCharacterCreationOpeningPicture"),
    (0x2BD4A, "PlayCharacterCreationOpeningSequenceAlt"),
    (0x2BBB5, "PollForEscapeKeyOnly"),
]

for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
