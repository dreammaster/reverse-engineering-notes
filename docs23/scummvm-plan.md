# ScummVM engine plan (Yendorian Tales Book I, Chapters 2 and 3)

What an `engines/yendorian/` module would be made of, given what `src23/` already holds. Nothing here is built against ScummVM yet (no ScummVM tree is
checked in); every item names the `src23` module that supplies the logic, so the engine is mostly glue. The playable proof of the approach is
`src23/tools/explore_sdl.c` (walk, fog, view, monsters, melee combat, rest, local map, dialogs), which uses the modules exactly as an engine would.

## Detection

Both games are plain DOS directories; detection needs `WORLD.DAT`, `PICTURES.VGA` and the executable. The files this project was developed against:

| game | file | size | MD5 |
|------|------|------|-----|
| Chapter 2 | `SW.EXE` | 199292 | d464f6847b9ea4296e9ce1b251f92788 |
| | `WORLD.DAT` | 1761397 | 97c71a50635cbf85f1cf853899931d88 |
| | `PICTURES.VGA` | 12550618 | ac02107de9680636fbfa43cb02ed5b00 |
| Chapter 3 | `REGISTER.EXE` | 202676 | 22bc83d3592e68b0b1d5a2462991256d |
| | `WORLD.DAT` | 4350901 | c3abfe959e5c28747109bd6e453f21fc |
| | `PICTURES.VGA` | 17221076 | a58eddc7ebd2f5152c42ef5bbc9e7fc1 |

(Chapter 3's folder also carries `Yendor3-full.exe`, a 4.6 MB wrapper that is not the game; `exedata.c` checks the real executable by its `QUIT "CREATE"`
label.) The registered and shareware versions differ (the shareware limit after clue-book page 5 is a flag in `cluepaged.c`); other releases need their own
checksums.

## Pieces

| engine part | uses | notes |
|-------------|------|-------|
| asset loading | `pictures.c`, `worldmap.c`, `item.c`, `monster.c`, `spellrecord.c`, `dialog.c`, `document.c`, `lockcatalog.c`, `worldobjects.c`, `exedata.c` | everything parses from memory images (`*_stdio.c` are only the file front ends), so `Common::File` / `SeekableReadStream` slots straight in |
| screen | `viewrender.c`, `minimap.c`, `statuspanel.c`, `monsterpanel.c`, `paperdoll.c`, `statsheet.c`, `roster.c`, `gamedialog.c`, `charcreate.c`, `clue*.c`, `localmap.c`, `font.c` | all draw into a 320 x 200 8-bit buffer given as `ViewRenderer.screen`; `OSystem::copyRectToScreen` + `setPalette` (6-bit DAC values: `(v << 2) \| (v >> 4)`) |
| palette effects | `palette.c` (fade rounds, dawn/dusk window, colour cycle) | the fades are 63 rounds; the intro scripts are in file-formats.md ("The opening story") |
| world window | `dungeongrid.c`, `windowbake.c`, `monsterpool.c`, `viewport.c`, `lighting.c`, `explore.c` | rebuild after every position change: build, bake, relink, then `monsterPoolEncounterScan` while drawing |
| input | `maininput.c`, `uiregions.c`, `movement.c`, `textfield.c` | key tables keyed by the original scan codes; click regions per screen |
| turn logic | `movement.c`, `interact.c`, `travel.c`, `monsterpool.c`, `combat.c`, `party.c`, `rest.c`, `gameclock.c`, `ailment.c`, `lightsource.c` | `RunDungeonGameLoop`'s order: input, `ProcessCombatRound`, then `ProcessLevelMonsters` and side traps |
| saves | `savegame.c`, `newgame.c` | `CURGAME` and `SAVGAMEn` are byte-identical 77,509 (Ch2) / 81,037 (Ch3) byte images (the slot name is a 25-character field inside the game-state block, `saveGetName`); keep that format so original saves load, and wrap it in the ScummVM save-file header only if the user wants metadata |
| music | `audio.c`, `cmf.c`, `music.c` | CMF tracks (AdLib instrument patches plus a standard-MIDI-like event stream): feed them to ScummVM's AdLib MIDI driver (`MidiDriver_ADLIB` / `Audio::MidiPlayer`) after converting the events; `music.c` has the track per area and time of day |
| effects | `audio.c`, `voc.c` | 221 VOC samples, 8-bit PCM at 4-13 kHz: `Audio::makeVOCStream` or the parsed PCM into `Audio::RawStream`; the intro voice cues are sound-event ids |
| timing | `gameclock.c` | the original's animation tick is a timer flag (UI flag `0x400`) at the animation-speed setting (`gamedialog.c`: 1 fast, 5 medium, 9 slow); the game clock advances once per real minute-timer tick |

## Engine loop

1. Title screen (`titlemenu.c`), character creation (`charcreate.c`, `chargen.c`, `newgame.c`) or a loaded save.
2. Main loop: draw the screen; poll keys; `mainCommandForKey` / `mainCommandForClick` give the action; apply it with the module for that action
   (movement -> `movementApply` + `movementClassifyCell` + `interact`, rest -> `restParty`, attack -> `combatPlayerMeleeAttack`, spells -> `spellcast.c`,
   inventory -> `inventory.c`, shops -> `shop.c`, clue book -> the `clue*.c` pages).
3. After a movement or turn: window rebuild, `monsterPoolTakeTurn` per live monster, combat if one engaged the party, `partyWipedOut`.

## Not done (see roadmap.md)

The opening and credits as scripts (their data and timeline are decoded), the clue sub-icon table (runtime-filled), music and effect playback,
the platform glue (`OSystem`, `Engine` subclass, `MetaEngine`, detection tables), and Chapter 1, which is a different, unanalysed engine.
