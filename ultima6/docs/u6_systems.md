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
| Combat | `combat.*`, `combat_tables.inc` | Nuvie (hit check, damage, armour, experience) with the original's wounded texts (seg067) | hand-to-hand only; no ranged weapons, no combat mode / initiative |
| Creatures | `creatures.*` | eggs as Nuvie's egg manager; hunting is our own | eggs, hatching, hunting, despawn; no worktype behaviours, no flee, no spell casters |
| Magic | `magic.*`, `spell_table.inc` | Nuvie's spell table and costs (circle = magic points) | 11 spells work (Harm, Heal, Great Heal, Light, Great Light, Infravision, Magic Arrow, Poison, Fireball, Lightning, ...), the rest say "No effect"; no reagent mixing |
| Time | `session.cpp`, `commands.cpp` | Nuvie's advance_time: timers, hourly magic points (+ level), torches burn, poison, fire, swamps | no hunger, no alcohol |
| Light | `map_renderer.*` | our approximation (torch 4, Light 5, Great Light 7, lamps 3; night 19-5, dungeons always) | approximate |
| Save/load | `world.*` (`save`), `ObjList::save` | original file layout (OBJLIST, OBJBLK??) written to our own directory (`u6save`, `--save`) | round trip tested, byte identical |
| New game | `world.*` (`load` with a character), `front_end.*` | original (`LZOBJBLK`, `LZDNGBLK`, `write_new_game_objlist`) | complete |
| Front end | `front_end.*` | menu and gypsy from `U.EXE` data | no intro, transfer, acknowledgements |

Non-original conveniences: `1`-`8` choose the member shown in the panel, `I` opens the inventory view (arrows select, Enter readies or unreadies, `D` drops), the cursor of
Look and Cast moves with the arrows and is confirmed with Enter. The original's mouse interface is not implemented.
