# The `ags-archives` resource, and pinning Rob Blanc 1 to AGS 2.4b

## What this is

The user added `ags-archives/` (not tracked before this session) at the repo
root: a collection of official AGS release archives from version 2.00
through 3.30+, one subfolder per version (`ags200`, `ags201`, ... `ags330`).
Versions up through roughly 2.7x are **docs-only** (a `DOCS`/`docs`
subfolder containing `CHANGES.TXT`, `TECHINFO.TXT`, license files, etc. --
no source code); later versions (3.00+) add `AGS.Types.xml` and richer
`Docs` trees. No source code for the early-2000s engine itself is present
anywhere in the archive -- this is NOT a substitute for the `Engine`/
`Common` 2011 reference source. What it DOES provide, uniquely, is:

1. **`CHANGES.TXT`** in every version folder -- a single CUMULATIVE
   revision history (each version's file contains that version's own
   entry at the top, plus every earlier version's entry below it, all the
   way back to 1.0). This means `ags240/docs/CHANGES.TXT` (1107 lines) is
   itself a complete history through July 2002, and is the single most
   valuable file in the whole archive for this project: it dates, by
   month, exactly when every script function, dialog-script command, GUI
   feature, and internal limit was added, changed, or removed.
2. **`TECHINFO.TXT`** -- a short technical document (unchanged, byte-for-
   byte, from `ags207` through `ags256`; a slightly expanded second
   revision appears in `ags261`/`ags262`; trimmed again by `ags270`) that
   gives the EXACT on-disk byte layout of two structures directly
   relevant to this project's own reverse-engineered structs: the
   `.CHA` character file format (matching `CharacterInfo` byte-for-byte)
   and, from 2.61 onward, the `.DLG` dialog file format (matching
   `DialogTopic`). These are Chris Jones's own contemporary documentation
   of the SAME structs this project has been reverse-engineering purely
   from the disassembly -- an independent, authoritative confirmation
   source that predates even the 2011 reference source by ~9-10 years,
   putting it MUCH closer to Rob Blanc 1's own era than `Engine`/`Common`
   is.

**How to use it going forward**: when a struct field, function, or
behavior is uncertain, `grep -rn` for a relevant keyword across
`ags-archives/*/docs/CHANGES.TXT` (or `DOCS/CHANGES.TXT` for the older,
all-caps folder naming used through `ags240`) before assuming it needs
fresh disassembly work -- the changelog will often say, in plain English,
exactly which version added/removed/changed the exact behavior in
question, letting this project's own "confirmed absent" findings be
independently dated rather than just inferred from silence in the
disassembly.

## Pinning Rob Blanc 1's actual AGS version: **2.4b, July 2002**

This is the single biggest payoff of this detour. Cross-referencing
`ags240/docs/CHANGES.TXT`'s version headers against this project's own
already-confirmed findings pins Rob Blanc 1 down almost exactly:

**Lower bound (must be `>= 2.4b`)**: `IsMusicVoxAvailable` is confirmed
present in Rob Blanc 1 (already correctly named via exact linker-symbol
match; used in this project's own `GameState.seperate_music_lib`
correction). `ags240/docs/CHANGES.TXT` line 3-4: `"VERSION 2.4b, July
2002 - Added IsMusicVoxAvailable text script function."` -- this function
did not exist before 2.4b.

**Upper bound (must be `< 2.5`)**: three independent, already-established
"confirmed absent" findings all match features `ags250/docs/CHANGES.TXT`
(`"VERSION 2.5, September 2002"`) introduces:
- `MYOGG`/`MYSTATICOGG` confirmed absent (exhaustive zero-"ogg" grep) --
  2.5 adds `"OGG Vorbis support for music, speech and sound effects"`.
- `DialogTopic.optionflags`/`optionnames` capacity confirmed at 15, not
  2011's 30 -- 2.5 says `"Increased maximum options per dialog topic
  from 15 to 30."` -- an exact match to both the OLD and NEW values this
  project independently derived purely from disassembly evidence.
- `run_dialog_script`'s `DCMD_SETGLOBALINT`(13) opcode confirmed absent
  (exhaustive opcode-switch read) -- 2.5 says `"Added 'set-globalint'
  dialog script command."`

**Consistent with, not just silent on, versions in between**: Rob Blanc 1
also has several functions/behaviors 2.4 (not 2.4a/2.4b specifically, the
July 2002 series as a whole) introduced: `GetTranslation`,
`PlayAmbientSound`, `StopAmbientSound`, `SetMouseBounds`, `StartCutscene`/
`EndCutscene` are all functions this project independently matched, and
`ags240`'s own 2.4 entry (line 39-44) lists ALL of them as new in that
release. The 2.4 entry also says `"Increase limit to 6000 sprite slots,
300 views"` -- matching this project's own independently-confirmed
`GameSetupStructBase.spriteflags[6000]` capacity (via
`prepare_characters_for_drawing`'s literal `0x1770`(6000) bounds check)
exactly. And `"Graphical scripts have been removed [from the EDITOR]...
Rooms with graphical scripts will continue to work until you modify the
room"` -- directly explaining WHY Rob Blanc 1's compiled engine still
contains a fully-functional `EventBlock`/`AnimationStruct` interpreter
(this project's own `AnimationStruct`/`FullAnimation` correction, and the
still-live `EventBlock`-based room interaction system) even though the
2011 reference source has long since dropped it: the RUNTIME support for
old graphical-script rooms was kept through (at least) 2.4, even after
the EDITOR stopped letting you create new ones.

**Conclusion**: treat "AGS 2.4b, July 2002" as the working ground-truth
version for Rob Blanc 1's actual engine -- not a certainty (no source for
2.4b itself exists in this repo to diff against), but an extremely
tightly-bounded inference (the binary's own link date, 2002-07-21, sits
inside 2.4b's own release month) that should inform every future
"is this feature present/absent" question: check `CHANGES.TXT` for
`< 2.4b` (present, unless later removed) vs `>= 2.4b` (likely absent,
unless it's part of 2.4b's own additions) before assuming a fact needs
fresh disassembly investigation.

## Direct confirmation: the binary's own embedded version/build-date string

Found while investigating `script_debug`'s `cmdd==1` "engine info" debug
command (a fresh-survey round, not itself part of the `ags-archives/`
detour, but a decisive upgrade to everything above). The literal format
string this build's `script_debug` passes to `sprintf` (confirmed via
`aAdventureGameS_0` in `rob_blanc_1.asm`'s own `.data` section) reads,
verbatim:

```
Adventure Game Studio run-time engine|ACI version 2.40.325|Compiled
on Jul 20 2002 at 18:56:30|Running %d x %d at %d-bit %s|Sprite cache
size: %d KB (limit %d KB)
```

This turns the previous section's "extremely tightly-bounded inference"
into a near-certainty: the binary states its own exact ACI version
(`2.40.325`, not just "2.4b") and its own exact compile timestamp
(`Jul 20 2002 at 18:56:30`) directly, one day before the PE header's
own 2002-07-21 link date -- fully consistent with a same-day-or-next-day
compile-then-link. No inference chain needed for this one fact; it's
printed by the engine itself. The format string is also simpler than
2011's own equivalent debug-info string (`AC.CPP:21062-21063`): no
`gfxDriver->GetDriverName()`/`filter->GetVersionBoxText()` fields and
only 2 sprite-cache stats (size/limit) rather than 3 (size/limit/
locked) -- consistent with this build predating `gfxDriver` and
per-sprite lock-tracking entirely, both already established repeatedly
elsewhere in this project.

## `TECHINFO.TXT`'s CHARACTER FILE format confirms `CharacterInfo`

`ags240/docs/TECHINFO.TXT` section 1.2 (dated "26 December 2001",
unchanged through `ags256`) documents the on-disk `.CHA` character file's
"character information block" byte-for-byte:

```
offset size  description
+00h  DWORD  default view number for character (normal walking view)
+04h  DWORD  talking view number
+08h  DWORD  current view number (=default view number)
+0Ch  DWORD  starting room number
+10h  DWORD  [used internally by AGS]
+14h  DWORD  X-coordinate of starting location
+18h  DWORD  Y-coordinate of starting location
+1Ch  DWORD  [used internally by AGS]
+20h  DWORD  character flags (bit-field)
+24h 28BYTEs [used internally by AGS]
+40h  WORD   movement speed
+42h  WORD   animation speed
+44h 204BYTEs [used internally by AGS]
+xxh 30BYTEs character name (zero-padded ASCIIZ string)
+xxh 16BYTEs script name (zero-padded ASCIIZ string)
+xxh  BYTE   [reserved]
```

This lines up with this project's own independently-confirmed
`CharacterInfo` fields with zero contradiction: `defview`@`+0x00`,
`talkview`@`+0x04` (just confirmed THIS session via `run_dialog_script`'s
`DCMD_SETSPCHVIEW` handler -- direct, independent confirmation from an
official 2001 document), `view`@`+0x08`, `room`@`+0x0C`, `x`@`+0x14`
(the doc's "+0x18h Y-coordinate" implies `y`@`+0x18`, not yet
individually confirmed in this project but consistent), `animspeed`@
`+0x42`. Arithmetic also lands exactly on two more already-confirmed
fields: `+0x44 + 204(0xCC) = +0x110` is EXACTLY `CharacterInfo.name`'s
own confirmed offset (via `GetLocationName`'s `GetTranslation` call), and
`+0x110 + 30(0x1E) = +0x12E` is EXACTLY `CharacterInfo.scrname`'s own
confirmed offset (via `compile_room_script`'s `strcat` call). Four
independently-confirmed fields (`talkview`, `defview`/`view`/`room`
positionally, `name`, `scrname`) all land on this 2001-vintage document's
own declared offsets with zero slack -- about as strong a cross-
validation as this project has had from any single source, and it
predates even the `OldCharacterInfo` ancestor struct in the 2011
reference by roughly a decade.

Also confirms (not yet individually verified in this project, but now a
strong, dateable lead for a future round): `+0x10` = `prevroom`
(the doc's own "[used internally by AGS]" combined with `ags240`'s own
CHANGES.TXT 2.15 entry, `"Fixed prevroom text script variable for
following characters"` -- an official name for this exact offset,
matching this project's own already-TENTATIVE `prevroom`@`+0x10` guess);
`+0x1C` = plausibly `following`/`followinfo` (per this project's own
`FollowCharacterEx`-based confirmation of those fields existing nearby,
not yet pinned to this exact offset); `+0x20` = the character `flags`
bit-field.

## `TECHINFO.TXT`'s DIALOG FILE format confirms `DialogTopic` (2.61-era, still structurally informative)

`ags261/docs/TECHINFO.TXT` (added in this revision, "Updated 12 April
2004" -- i.e. AFTER Rob Blanc 1, so this documents the LATER 30-option
layout, not Rob Blanc 1's own 15-option one directly) declares the
`.DLG` file's `DialogTopic` struct explicitly:

```c
struct DialogTopic {
    char          optionNames[30][150];
    DWORD         optionFlags[30];    // 1 = "Show",  4 = NOT "Say"
    DWORD         hasCompiledScript;
    WORD          entryPoints[30];
    WORD          startupEntryPoint;
    WORD          codeSize;
    DWORD         numOptions;
    DWORD         topicFlags;
};
```

Field ORDER and TYPE match this project's own independently-confirmed
Rob Blanc 1 `DialogTopic` exactly, field for field: `optionnames`
(char array), `optionflags` (confirmed as a 4-byte-per-entry array via
`SaveGameSlot`'s own literal `ElementSize=4`, matching `DWORD` exactly),
`optionscripts` (this project's name for `hasCompiledScript`'s
presence-flag-that-becomes-a-pointer idiom), `entrypoints` (confirmed
`short`/2-byte array, matching `WORD` exactly), `startupentrypoint`
(`WORD`/`short`), `codesize` (`WORD`/`short`), `numoptions` (`DWORD`/
`int`). Only `topicFlags` (the trailing field here) is CONFIRMED ABSENT
from Rob Blanc 1's own struct (it ends exactly at `numoptions` with zero
slack) -- consistent with `topicFlags` being a feature added sometime
between Rob Blanc 1's 2.4b and this document's 2.61/3.61-era snapshot,
not yet dated more precisely. The DRIFT already established
independently (30->15 options, 150->70 chars/option) is exactly what
2.5's own changelog entry (`"Increased maximum options per dialog topic
from 15 to 30"`) predicts.

## Other dated confirmations worth recording

- `ags261/docs/CHANGES.TXT`'s `"VERSION 2.6, December 2003"` entry adds
  `SetGUIZOrder`, `SetGUIClickable`, `SetGUIObjectSize`, and `"GUI
  Z-order support, so that you can choose which order overlapping GUIs
  are drawn in"` -- directly dating this project's own "confirmed
  absent" findings for `GUIMain.zorder`, the `GUIObject.guin`/`.objn`
  architectural lead, and the (searched-for-but-absent)
  `SetGUIClickable` API, all to December 2003 -- over a year after Rob
  Blanc 1.
- `ags222/docs/CHANGES.TXT`'s `"VERSION 2.22, December 2001"` entry adds
  `GetMIDIPosition`, `SeekMIDIPosition` (both matched this session) and
  the dialog-script commands `"add-inv, set-speech-view"` -- exactly
  dating this project's own confirmed `DCMD_ADDINV`(10)/
  `DCMD_SETSPCHVIEW`(11) opcodes to December 2001. The same entry says
  `"Upped GlobalInts to 300"` -- an exact match to this project's own
  independently-confirmed `GameState.globalscriptvars[300]` capacity
  (drift vs. 2011's later `MAXGSVALUES=500`).
- `ags230/docs/CHANGES.TXT`'s `"VERSION 2.3, January 2002"` entry adds
  `RunCharacterInteraction`/`RunHotspotInteraction`/`RunObjectInteraction`,
  `RawSetColor`, `GetGUIAt`, `StopMusic`, and `"new-room dialog script
  command"` -- dating this project's confirmed `DCMD_NEWROOM`(12) opcode
  and several already-matched EventBlock-adjacent functions to January
  2002. The same entry's `"Upped limit to 19 hotspots (sorry, quick fix,
  more later)"` reconciles cleanly with this project's own confirmed
  `MAX_HOTSPOTS=20`: 19 USABLE hotspots plus hotspot 0 (reserved for
  "no hotspot"/background) is exactly a 20-element array, not a
  contradiction.
- `ags200`-era entries (`"narrator" as a character name to dialog
  script`, 2.12/August 2000) confirm the `999` sentinel this project
  found in `run_dialog_script`'s `DCMD_SAY` handler is specifically the
  documented "narrator" convention, not an arbitrary magic number.

## What this does NOT resolve

No source code for 2.4b (or any pre-3.0 version) exists anywhere in this
archive -- only compiled-release documentation. This does not replace
the disassembly-driven confirmation standard this project otherwise uses
(a changelog line naming a feature is strong DATING evidence, but still
not the same as an access-site confirmation for an unconfirmed struct
offset) -- treat it as a powerful cross-check and a fast way to date
"present vs. absent" questions, not a substitute for reading the
disassembly when a field's exact BYTE OFFSET (as opposed to its
existence/era) is still in question.
