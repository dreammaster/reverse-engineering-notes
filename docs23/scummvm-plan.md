# ScummVM engine plan (Yendorian Tales Book I, Chapters 2 and 3)

What an `engines/yendorian/` module would be made of, given what `src23/` already holds. Nothing here is built against ScummVM yet (no ScummVM tree is
checked in); every item names the `src23` module that supplies the logic, so the engine is mostly glue. The playable proof of the approach is
`src23/session.c` is the engine core (movement, fog, monsters, combat, rest, locks, chests) and `src23/tools/explore_sdl.c` a front end for it, which uses the modules exactly as an engine would.

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

## The platform seam: `gameui.c`

`src23/gameui.h` is the whole game as a platform-independent front end: `gameUiKey` (a key press; letters and digits are lower-case ASCII, the arrows and F-keys are `UiKey`
values), `gameUiDraw` (a 320 x 200 frame of palette indices in `ui->screen`, the palette `gameUiPalette`), `gameUiStatus` (a line for a title bar) and a `GameUiHost` of five
callbacks (sound effect, music track, the ending cinematic, store / fetch a save image). A ScummVM `Engine::run` therefore needs only: parse the pictures into a
`ViewPictureFn`, build a `GameSession` from `WORLD.DAT` (`sessionNew`), map `Common::KeyCode` to `UiKey`, copy `ui->screen` through `OSystem::copyRectToScreen` with the palette
(six-bit DAC values), call `sessionAdvanceTime` on a minute timer and `sessionTickAilments` as `gameUiKey` already does, feed the CMF tracks (`cmfplayer.c`, or ScummVM's AdLib
driver) and VOC effects to the mixer, and run the cinematics of `introstory.h` (`introOpeningScript`, `introStoryScript`, `introEndingScript`, `introOpening3Script`) against its host
interface. `tools/explore_sdl.c` is exactly that glue for SDL2, about 300 lines.

The clue book (F8) opens when the platform hands the UI the executable with `gameUiSetExe` (its texts and entry lists live in the data segment; `ExeData`, `clueentries.h`). It has:

- the walk through (F6; I / Q turn the pages) and the help screen (Tab);
- the category lists F1 maps, F2 monsters, F3 spells, F4 magic users (a class, then its spells: `clueSpellClassUse`, the rule behind the class rows of the spell page), F5 items (one of the
  eight groups, then its items with the "+N" variant row; group 7 is the transportation page). Lists page with Up / Down / I / Q, Enter opens an entry, Escape backs out one level, Left /
  Right pick a map's legend marker or an item's variant.

The lists are the executable's own. Their entries carry a flag for what the shareware program opens: the others are dim and answered with the registration reminder unless
`clueRegistered` is set (a ScummVM front end would set it; Chapter 3's REGISTER.EXE is the registered program and has no locked entries; `explore_sdl` sets it unless `EXPLORE_SHAREWARE` is
given). Not done: item group 2 (the attribute enhancers: the original's screen only waits for a key), the item pages' ability-info overlay, the monster animation, and Chapter 3's list backdrop
is assumed to be picture 6.

## Engine loop

1. Title screen (`titlemenu.c`), character creation (`charcreate.c`, `chargen.c`, `newgame.c`) or a loaded save.
2. Main loop: draw the screen; poll keys; `mainCommandForKey` / `mainCommandForClick` give the action; apply it with the module for that action
   (movement -> `movementApply` + `movementClassifyCell` + `interact`, rest -> `restParty`, attack -> `combatPlayerMeleeAttack`, spells -> `spellcast.c`,
   inventory -> `inventory.c`, shops -> `shop.c`, clue book -> the `clue*.c` pages).
3. After a movement or turn: window rebuild, `monsterPoolTakeTurn` per live monster, combat if one engaged the party, `partyWipedOut`.

## Not done (see roadmap.md)

The title screen's own animations are not scripted (Chapter 2's and Chapter 3's openings and Chapter 2's ending are `introstory.c`; Chapter 3's ending, if there is one, is not found),  effect playback (music: `opl.c` is a reference synthesizer to compare against),
the platform glue (`OSystem`, `Engine` subclass, `MetaEngine`, detection tables), and Chapter 1, which is a different, unanalysed engine.
