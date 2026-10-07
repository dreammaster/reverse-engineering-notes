# Porting guide: from `src23` to a ScummVM engine

`src23/` is a decode-and-reimplement of everything the two games (Yendorian Tales Book I, Chapters 2 and 3) do that does not depend on DOS. This
page says how the pieces fit together so an engine can be assembled from them; the formats are in [file-formats.md](file-formats.md), per-game
differences in [engine-diffs.md](engine-diffs.md), and what is still missing in [roadmap.md](roadmap.md).

## Conventions

* Every module takes a `GameKind` (`GameYendor2` / `GameYendor3`) where the games differ and is otherwise shared. Tables whose values differ
  are built in (generated from the executables by the `ida_scripts/dump_*.py` scripts, whose headers say how to rerun them).
* "Decide, don't apply": most gameplay functions take records (party, monster, save) as byte buffers with the original's offsets and apply the
  original's arithmetic exactly, including quirks (they are listed in engine-diffs.md). There is no hidden global state apart from
  `random.c`'s generator, which the caller owns.
* Files are read through `*_stdio.c` helpers or from memory images; nothing assumes stdio. Game data (`WORLD.DAT`, `PICTURES.VGA`, the saves)
  is never copied into the sources, only the tables that live in the *executables*.
* Tests print `PASS`/`FAIL` lines; `python src23/tests/run_all.py` builds and runs all of them (gcc). Real-data tests skip when the game files
  are absent (`YENDOR2_GAME_DIR` / `YENDOR3_GAME_DIR` override the default `yendorN/game`).

## Assets

| Asset | Module | Notes |
|-------|--------|-------|
| `PICTURES.VGA` | `pictures.c`, `pictures_stdio.c` | ten categories of same-sized 8 bpp pictures, colour 0xFF transparent |
| palette, fades, colour cycle | `pictures.c` (offset), `palette.c` | 6-bit DAC values in `WORLD.DAT`; block 2 drives the dawn/dusk fade |
| fonts | `font.c` | 6 x 6, four faces, tables built in |
| music / effects | `audio.c`, `cmf.c`, `voc.c` | CMF tracks (parsed to events + AdLib instrument patches) and VOC samples (8-bit PCM, 4-13 kHz) inside `WORLD.DAT`; ids per area in `music.c`; `opl.c` + `cmfplayer.c` are a small OPL2 FM synthesizer and CMF player (all 45 tracks render; `tools/cmf_wav.c` writes a WAV to listen to -- none of the tracks uses the rhythm mode); an engine would use ScummVM's own OPL emulator |
| world map, legends | `worldmap.c` | 800 x 168 cells; legends give floor/ceiling/wall/far-wall/minimap pictures |
| items, monsters, spells, dialog, documents, locks, travel, triggers | `item.c`, `monster.c`, `spellrecord.c`, `dialog.c`, `document.c`, `lockcatalog.c`, `travel.c`, `maptrigger.c` | catalogs read from `WORLD.DAT` |
| saves | `savegame.c`, `newgame.c` | `CURGAME`/`SAVGAMEn`; a new game is a 5000-byte template at the end of `WORLD.DAT` |
| text messages | `exedata.c` | the message strings and small tables live in the executables' data segment, which is stored verbatim (not packed): Chapter 2's `SW.EXE` at file offset `0x21660`, Chapter 3's `REGISTER.EXE` (the playable program the `yendor3` IDB is built from; `Yendor3-full.exe` is an unrelated 4.6 MB wrapper) at `0x21DB0`; a string at IDB `DS:0xNNNN` is at base + `0xNNNN`. The fixed UI labels used so far are in the screen modules (the text is copyrighted; read it from the player's files instead of shipping it); [messages.md](messages.md) lists, per game procedure, the offsets of the strings it loads, and [sound-events.md](sound-events.md) the sound effect ids it triggers |

## Rendering the main screen

Compose a 320 x 200 8-bit surface, then apply the palette (`pictures.h`, `palette.h`):

1. frame: category 0 picture 1 at (1, 1) (`pictures.h`);
2. first-person view: `viewportBuild` -> `viewportComputeVisibility` -> `lightingComputeGradient` -> `viewRender` (`viewrender.h`), with monsters in
   `ViewScene.cellMonsters` / `combatMonsters` (`viewDrawMonster`); the view is 224 x 136 at (8, 8);
3. minimap: `minimapBuild`/`minimapDraw` at (240, 8); party panels: `statusPanelDraw` x4; combat: `monsterPanelDraw` x3;
4. text uses `font.c` at the positions the screen modules document; click handling uses `uiregions.c` (`uiRegionHit`).

`src23/tools/render_view.c` does exactly this and writes a PNG (env vars `RENDER_HUD`, `RENDER_MONSTER`, `RENDER_DOLLS`, `RENDER_SHEET`,
`RENDER_ROSTER`, `RENDER_DIALOG`, `RENDER_PICK` add the party panels, a monster, the inventory dolls, a character sheet, the roster, the pause dialog or a creation step); it is the quickest way to check a change.

`src23/tools/walk.c` walks the new-game party through a real map with `movementApply` / `movementClassifyCell` (doors, void, bumps, special cells) and
writes the view after every step as a contact sheet (`walk 3 yendor3/game FFFFFR... out.png`): a headless check that movement, the grid, lighting
and the renderer agree.

Other screens implemented as draw functions: inventory dolls (`paperdoll.c`), character sheet (`statsheet.c`), party roster (`roster.c`). Their
layouts come from the click-region tables.

## Game flow

The original runs one loop (`start` -> `RunDungeonGameLoop`); everything below is called from it and has a C counterpart:

* input to action: `movement.c` (passability, bump), `interact.c` (doors/locks/keys), `travel.c` (teleports and the typed-password system),
  `mapview.c` (which maps the party may open), `explore.c` (fog of war), `mount.c`;
* time: `gameclock.c` (calendar), `rest.c` (the R command: hourly slices, interruption, calendar, food-driven regeneration), `lighting.c`, `lightsource.c`, `ailment.c`, `music.c`;
* party: `party.c` (records, equipment, containers, stats), `chargen.c`, `inventory.c` (what an item may do/where it fits), `shop.c`, `chest.c` (a lock record's eight slots: loot a chest or stock a shop), `repair.c`,
  `consumable.c`, `relics.c`, `thrown.c`, `spellcast.c`, `spelljump.c`;
* monsters and combat: `monsterpool.c` (spawn, ambush, walk, rewards: `monsterPoolTakeTurn`), `monster.c`, `combat.c`, `effect.c`;
* world state: `globalflags.c`, `worldobjects.c`, `savegame.c`.

See [scummvm-plan.md](scummvm-plan.md) for how the pieces map onto an engine (detection checksums, loop, audio, saves), and [module-index.md](module-index.md) for every module with a one-line description.

## Robustness

`src23/tools/fuzz_parsers.c` feeds truncated and byte-corrupted copies of the real WORLD.DAT (ending at a no-access page, so any over-read crashes)
to every memory-image parser (items, monsters, spells, world map, dialog, documents, locks, world objects, the new-game builder, the window marker pass over the damaged tables, and a real SAVGAME1 through saveGameLoad); 300-400 rounds per game pass without a fault.

## A playable slice

`src23/session.c` is the game core: a `GameSession` owns the map, catalogs, save, monster pool and combat, and composes the modules into the commands of the original's
main loop -- `sessionMove` (passability, fog reveal, window rebuild = grid + `dungeonGridBakeMarkers` + `monsterPoolRefreshWindow`, then every live monster's turn
with `monsterPoolTakeTurn`; a monster that reaches the party starts a combat), `sessionAttack` (`combatBuildTurnOrder` / `combatPlayerMeleeAttack` /
`combatProcessMonsterTurn` / `combatProcessRound`; victory awards the loot with `monsterRewardsAward`, a wipe is `partyWipedOut`), `sessionRest` (`restParty`),
`sessionCast` (a combat attack spell: known, `spellCanCast`, costs, `combatResolveSpellAttack` / `combatApplySpellAttackToActiveSlots`), `sessionUseItem` (the restoratives of consumable.h from a member's inventory), `sessionUnlock` (`interactUnlockFacing`), `sessionLoot` (`chestTake`) and `sessionScene` (the view cells and the monsters to draw via `monsterPoolEncounterScan`).
Nothing in it draws, plays or waits; the other spell kinds, other item uses, items on the cursor, inventory, shops and dialogs are decision modules a front end wires in. `test_session.c` plays both games
(random soak, a combat from the first spider to the end, a chest looted once).

`src23/tools/explore_sdl.c` is a front end for it: an SDL2 window (320x200 palette indices expanded to ARGB, the DAC values scaled `(v << 2) | (v >> 4)`), keys through
`mainCommandForKey` with the original scan codes, the screen drawn from the frame picture, `viewRender`, `minimapDraw` and the four `statusPanelDraw` panels, with the local
area map (M), the pause dialog (D), hero sheets (F1-F4) and paper dolls (P) as overlays, the clock keys (+ / -) to watch the lighting, and sound -- the area's music
(`musicRegionChanged` -> `cmfplayer.c` through SDL's audio callback, looping) and Chapter 2's bump sound (effect 6, `_val33`). `EXPLORE_RANDOM=<n>` plays n random keys (a soak
test: 3000 keys on five starts of both games ran without a fault), `EXPLORE_KEYS` / `EXPLORE_SHOT` replay a key string headlessly (`SDL_VIDEODRIVER=dummy`,
`SDL_AUDIODRIVER=dummy`) and write the final screen as a PNG. A ScummVM engine replaces the SDL calls with `OSystem` ones and keeps the session.

## Opening story

`intro2.c` is Chapter 2's story cinematic: `introCellsInit` / `introCellsFrame` are the seven animated cells (flags, stepping, top and bottom clipping, and the original's
over-read quirk for a cell cut off at the top), `introCards` / `introCardLine` the nine text cards (read from SW.EXE; with sound effects on the original plays the voice and
does not draw the text). The whole Chapter 2 opening -- the title fade-in, the spark, the plaques lighting, then the story -- is a script plus a player (`introstory.c`: `introOpeningScript` + `introStoryScript` over a host interface of present / sound / music / tick / escape; `tools/intro_story.c ... all` renders stills); the same is described step by step in file-formats.md ("The opening story of Chapter 2 and Chapter 3", including the shorter,
different Chapter 3 opening); `src23/tools/intro_sheet.c` renders four panned frames of the backdrop with the cells, `intro_cards.c` the nine story cards with their text read from the executable.

## Input

`maininput.c` has the main loop's key map (both jump tables of `start`, dumped by `dump_main_key_table.py`) and the combat-turn keys
(`HandleDungeonInput`): `mainCommandForKey` / `combatCommandForKey` return what the original would run, `mainCommandRequiredItem` the item a
command needs (map, clock, key). Mouse clicks go through `uiregions.c`. Chapter 2's debug keys are listed but not decoded.

## What is still missing

See [roadmap.md](roadmap.md). In short, what is not in `src23` is: playback of the animated title / opening / credits sequences (their data, timelines and
primitives -- `intro2.c`, `palettefade.c`, `palette.c` -- are decoded), the debug commands of Chapter 2, sound-effect playback and the engine's mixer glue (the music files are parsed and synthesised in `opl.c`; the VOC effects are parsed), the platform glue of an engine, and Chapter 1's engine (a different, earlier engine with no analysis yet).
Text is read from the executables (`exedata.c`), not shipped.
