# Game systems of the stand-alone engine

Where the rules come from and how complete each is. "Original" means read off `GAME.EXE`; "Nuvie" means taken from its U6 scripts (which were written from the
original's behaviour) because the original's code has not been traced for it yet. All of it is in `src/ultima6/`, with tests in `tests/`.

| System | Files | Source of the rules | State |
|---|---|---|---|
| Passability | `movement.*` | original `sub_25D7F`, class table from `seg066` | complete |
| Schedules and walking | `actors.*` | original `update_actor_schedules` (2AED5); walking is our own breadth-first search | hourly update complete, path finding approximate |
| Conversations | `conversation.*`, `talk.*` | Nuvie's interpreter; texts and prompts from `cmd_talk` | all CONVERSE.A scripts run; Resurrect, Sleep, Horse are stubs |
| Naming (Look) | `describe.*` | original `get_object_name`, `sub_2FED1`, `sub_2FCAE`, stackable table at ds:0765 | complete for objects; no creature "health" text |
| Inventories | `world.*` | original quantity kinds; weights = TILEFLAG block 3 | containers nested; no slots (a readied object just has the READIED location) |
| Combat | `combat.*`, `combat_tables.inc` | original hit check, wound texts, tables (`nuvie_differences.md`); damage and experience as Nuvie | hand-to-hand and bows/crossbows with a cursor; no combat modes, no spell casters |
| Creatures | `creatures.*` | eggs as Nuvie's egg manager; turns follow `wt_wander`, the assault range test and `wt_immobile` of the original | eggs, hatching, hunting, wandering, despawn; no other worktype behaviours |
| Magic | `magic.*`, `spells.cpp`, `spell_table.inc` | spell table, costs and `cmd_cast` texts are the original's; effects own (Nuvie-like) | about 30 spells work (see approximations #56); the rest say "No effect"; no reagent mixing |
| Time | `session.cpp`, `commands.cpp` | Nuvie's advance_time: timers, hourly magic points (+ level), torches burn, poison, fire, swamps | no hunger, no alcohol |
| Light | `map_renderer.*` | our approximation (torch 4, Light 5, Great Light 7, lamps 3; night 19-5, dungeons always) | approximate |
| Save/load | `world.*` (`save`), `ObjList::save` | original file layout (OBJLIST, OBJBLK??) written to our own directory (`u6save`, `--save`) | round trip tested, byte identical |
| New game | `world.*` (`load` with a character), `front_end.*` | original (`LZOBJBLK`, `LZDNGBLK`, `write_new_game_objlist`) | complete |
| Front end | `front_end.*` | menu, gypsy and story text from `U.EXE` data | menu, character creation, the introduction as a slide show (#53); no transfer, acknowledgements |

Non-original conveniences: `1`-`8` choose the member shown in the panel, `I` opens the inventory view (arrows select, Enter readies or unreadies, `D` drops), the cursor of
Look and Cast moves with the arrows and is confirmed with Enter. The original's mouse interface is not implemented.

Added since: `use.cpp` (the Use command as the original's `cmd_use` table: doors, chests, containers, food, lights, potions, keys and lock picks, buckets, churn, beehive,
fishing, powder keg, sextant, shovel, moonstones, Orb of the Moons, shrines, fountains; the Move command), `moongate.cpp` (moon phases, blue and red gates), `ending.*` (the
ending after the Codex has gone), books and signs from `BOOK.DAT`. Approximations 49-59 in [approximations.md](approximations.md) say what is not exact.
