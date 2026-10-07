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
| music / effects | `audio.c`, `cmf.c`, `voc.c` | CMF tracks (parsed to events + AdLib instrument patches) and VOC samples (8-bit PCM, 4-13 kHz) inside `WORLD.DAT`; ids per area in `music.c`; no playback yet |
| world map, legends | `worldmap.c` | 800 x 168 cells; legends give floor/ceiling/wall/far-wall/minimap pictures |
| items, monsters, spells, dialog, documents, locks, travel, triggers | `item.c`, `monster.c`, `spellrecord.c`, `dialog.c`, `document.c`, `lockcatalog.c`, `travel.c`, `maptrigger.c` | catalogs read from `WORLD.DAT` |
| saves | `savegame.c`, `newgame.c` | `CURGAME`/`SAVGAMEn`; a new game is a 5000-byte template at the end of `WORLD.DAT` |
| text messages | `exedata.c` | the message strings and small tables live in the executables' data segment, which is stored verbatim (not packed): Chapter 2's `SW.EXE` at file offset `0x21660`, Chapter 3's `REGISTER.EXE` (the playable program the `yendor3` IDB is built from; `Yendor3-full.exe` is an unrelated 4.6 MB wrapper) at `0x21DB0`; a string at IDB `DS:0xNNNN` is at base + `0xNNNN`. The fixed UI labels used so far are in the screen modules (the text is copyrighted; read it from the player's files instead of shipping it); [messages.md](messages.md) lists, per game procedure, the offsets of the strings it loads |

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

`src23/tools/explore_sdl.c` is the smallest engine built from the modules: an SDL2 window (320x200 palette indices expanded to ARGB, the DAC values scaled `(v << 2) | (v >> 4)`) in which the new-game
party walks a real map. Keys go through `mainCommandForKey` with the original scan codes, steps through `movementApply` / `movementClassifyCell`, the fog reveal is
`exploreRevealAroundPlayer`, and the screen is the frame picture, `viewRender`, `minimapDraw`, the four `statusPanelDraw` panels, plus the local area map (M) and the pause dialog (D)
as overlays and the clock keys (+ / -) to watch the lighting. Each rebuild of the window is `dungeonGridBuild` + `dungeonGridBakeMarkers` (windowbake.c: the door / marker bits from the world objects) + `monsterPoolRefreshWindow`; the view's monsters come from `monsterPoolEncounterScan` and take their turns (`monsterPoolTakeTurn`) after each step; a monster that reaches the party starts a combat (`combatBuildTurnOrder` / `combatProcessMonsterTurn` / `combatPlayerMeleeAttack` / `combatProcessRound`, A attacks, victory awards the loot with `monsterRewardsAward`, a wipe is `partyWipedOut`) -- the whole cycle of walking, meeting a spider, fighting and winning runs on the real data of both games. F1-F4 (a hero's detail sheet), K / S (unlock / loot the chest ahead) and P (the paper dolls) open more screens; `EXPLORE_RANDOM=<n>` plays n random keys (a soak test: 3000 keys on five starts of both games ran without a fault), `EXPLORE_KEYS` / `EXPLORE_SHOT` replay a key string headlessly (`SDL_VIDEODRIVER=dummy`) and write the final screen as a PNG.
A ScummVM engine replaces the SDL calls with `OSystem` ones and keeps everything else.

## Opening story

`intro2.c` is Chapter 2's story cinematic: `introCellsInit` / `introCellsFrame` are the seven animated cells (flags, stepping, top and bottom clipping, and the original's
over-read quirk for a cell cut off at the top), `introCards` / `introCardLine` the nine text cards (read from SW.EXE; with sound effects on the original plays the voice and
does not draw the text). The rest is a script of fades and waits listed step by step in file-formats.md ("The opening story of Chapter 2 and Chapter 3", including the shorter,
different Chapter 3 opening); `src23/tools/intro_sheet.c` renders four panned frames of the backdrop with the cells, `intro_cards.c` the nine story cards with their text read from the executable.

## Input

`maininput.c` has the main loop's key map (both jump tables of `start`, dumped by `dump_main_key_table.py`) and the combat-turn keys
(`HandleDungeonInput`): `mainCommandForKey` / `combatCommandForKey` return what the original would run, `mainCommandRequiredItem` the item a
command needs (map, clock, key). Mouse clicks go through `uiregions.c`. Chapter 2's debug keys are listed but not decoded.

## What is still missing

See [roadmap.md](roadmap.md). In short, what is not in `src23` is: playback of the animated title / opening / credits sequences (their data, timelines and
primitives -- `intro2.c`, `palettefade.c`, `palette.c` -- are decoded), the debug commands of Chapter 2, music and sound playback and the OPL / mixer glue (the files
are located and parsed), the platform glue of an engine, and Chapter 1's engine (a different, earlier engine with no analysis yet).
Text is read from the executables (`exedata.c`), not shipped.
