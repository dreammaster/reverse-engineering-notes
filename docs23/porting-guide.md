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
| music / effects | `audio.c` | CMF tracks and VOC samples inside `WORLD.DAT`; ids per area in `music.c` |
| world map, legends | `worldmap.c` | 800 x 168 cells; legends give floor/ceiling/wall/far-wall/minimap pictures |
| items, monsters, spells, dialog, documents, locks, travel, triggers | `item.c`, `monster.c`, `spellrecord.c`, `dialog.c`, `document.c`, `lockcatalog.c`, `travel.c`, `maptrigger.c` | catalogs read from `WORLD.DAT` |
| saves | `savegame.c`, `newgame.c` | `CURGAME`/`SAVGAMEn`; a new game is a 5000-byte template at the end of `WORLD.DAT` |
| text messages | -- | the game's message strings live in the executable (not extracted); the fixed UI labels used so far are in the screen modules |

## Rendering the main screen

Compose a 320 x 200 8-bit surface, then apply the palette (`pictures.h`, `palette.h`):

1. frame: category 0 picture 1 at (1, 1) (`pictures.h`);
2. first-person view: `viewportBuild` -> `viewportComputeVisibility` -> `lightingComputeGradient` -> `viewRender` (`viewrender.h`), with monsters in
   `ViewScene.cellMonsters` / `combatMonsters` (`viewDrawMonster`); the view is 224 x 136 at (8, 8);
3. minimap: `minimapBuild`/`minimapDraw` at (240, 8); party panels: `statusPanelDraw` x4; combat: `monsterPanelDraw` x3;
4. text uses `font.c` at the positions the screen modules document; click handling uses `uiregions.c` (`uiRegionHit`).

`src23/tools/render_view.c` does exactly this and writes a PNG (env vars `RENDER_HUD`, `RENDER_MONSTER`, `RENDER_DOLLS`, `RENDER_SHEET`,
`RENDER_ROSTER` add the party panels, a monster, the inventory dolls, a character sheet or the roster); it is the quickest way to check a change.

Other screens implemented as draw functions: inventory dolls (`paperdoll.c`), character sheet (`statsheet.c`), party roster (`roster.c`). Their
layouts come from the click-region tables.

## Game flow

The original runs one loop (`start` -> `RunDungeonGameLoop`); everything below is called from it and has a C counterpart:

* input to action: `movement.c` (passability, bump), `interact.c` (doors/locks/keys), `travel.c` (teleports and the typed-password system),
  `mapview.c` (which maps the party may open), `explore.c` (fog of war), `mount.c`;
* time: `gameclock.c` (calendar, rest), `lighting.c`, `lightsource.c`, `ailment.c`, `music.c`;
* party: `party.c` (records, equipment, containers, stats), `chargen.c`, `inventory.c` (what an item may do/where it fits), `shop.c`, `repair.c`,
  `consumable.c`, `relics.c`, `thrown.c`, `spellcast.c`, `spelljump.c`;
* monsters and combat: `monsterpool.c` (spawn, ambush, walk, rewards: `monsterPoolTakeTurn`), `monster.c`, `combat.c`, `effect.c`;
* world state: `globalflags.c`, `worldobjects.c`, `savegame.c`.

## What is still missing

See [roadmap.md](roadmap.md). In short: the screens not yet drawn (title menu, character creation, shop, alchemy, clue book, pause dialog), the
monster damage splash, input/timing glue (the original is event driven from DOS interrupts), the text messages from the executables, music
and effect *playback* (the files are located but not decoded), and Chapter 1's engine (a different, earlier engine).
