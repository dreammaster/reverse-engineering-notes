# Approximations, guesses and borrowed rules

Everything in the stand-alone engine that is **not** a faithful port of `GAME.EXE` / `U.EXE`. The final phase of the project is to replace each entry with the original's
exact behaviour (or, where that cannot be done, to hand the details over for further analysis). Keep this file current: add an entry whenever something is approximated,
and delete it (with the commit that fixes it) when the exact version is in. Places in the code are also tagged with the comment `APPROX(n)` where practical
(`grep -rn "APPROX(" src`).

See also [nuvie_differences.md](nuvie_differences.md): where Nuvie and the original disagree, the original is followed and the difference is logged there.

Legend — **Source**: *own* = invented by me, *Nuvie* = taken from Nuvie's reimplementation (which may itself differ from the original), *partial* = original read but incomplete.
**Original** = routine(s) in the IDA databases (`ultima6/u6/u6_game.asm`, `u6_u.asm`, `u6_end.asm`) to analyse. Addresses are linear offsets as in the docs.

## A. Movement, people and time

| # | What | Where | Source | What the original does / where to look |
|---|---|---|---|---|
| 1 | NPC path finding: breadth-first search in a 41 x 41 square, 8 directions, diagonal squeeze rule | `actors.cpp` `ActorManager::step` | own | `sub_2868C` (path finder), `sub_28AA7` (move an actor towards its schedule place; teleports when outside the 40 x 40 cache window, `word_3EFF5` / `word_3EFF7` limit how many actors path-find per turn), `sub_2A3BA` (the per-turn loop over actors with worktype 0x81), `sub_28F37`, `sub_29134`. Worktype handlers are the 0x80+ and 0-0x10 values in `sub_13305`'s per-actor loop. |
| 2 | "Near the party => walk, else teleport" rule (|dx|,|dy| <= 19), at most 12 walkers a minute | `actors.cpp` `ActorManager::tick` | own (shape taken from `sub_28AA7`) | exact windows and counters in `sub_28AA7` / `sub_2A3BA`; placement search radius 2 is own (`sub_1B26F` places objects). |
| 3 | Actors only have the schedule worktype 0x81 behaviour; all other worktypes (wander, sleep, guard walks, play lute, face directions, attack, flee, follow, berserk ...) do nothing | `actors.cpp`, `creatures.cpp` | own | the worktype dispatch inside `sub_13305` (825 lines, per-actor loop at 133B5-13759) and the helpers it calls (`sub_2A0DA`, `sub_29B97` ...). Nuvie's list: `actors/u6_work_types.h`, `script/u6/worktype.cpp`. |
| 4 | Party following: each member steps into the square the one ahead left, else BFS, else teleport | `commands.cpp` `Game::followLeader` | own | `sub_275BD` (followers), `sub_28F67`, `sub_26CC4`, `sub_263D4` (set actor facing/frame), `sub_26F03` (after-move processing), `sub_1B26F` (move object). Also the swap-places rule in `sub_2787E` (27B32-27C11) is only partly reproduced. |
| 5 | Walking: no "Hic!" drunkenness (`byte_404C0`), no mouse-driven sliding along walls (`word_3DD0A`), no sound on a blocked step, no vehicle movement (`sub_26CC4`, wind) | `session.cpp` `Game::step` | partial | `sub_2787E`. Time cost: I advance exactly one minute per step; check what `sub_13305`'s argument is for each command (`command_loop`, 13C64). |
| 6 | Facing/walk frames of sprites: `direction * 4 + {0,1,0,2}[walk]` for frames < 16 | `actors.cpp` `ActorManager::face` | Nuvie | `sub_263D4` and the tables near it; animals and other layouts differ. |
| 7 | Game clock: 28-day months, 13 months a year, day of week `(day-1)%7+1` | `session.cpp` `GameClock::advance` | Nuvie / partly checked | check the month and year rollover in `sub_13305` (inc. minute) and `sub_1312A`. Day of week is read from `update_actor_schedules` (exact). |
| 8 | Time of day thresholds: night when hour < 5 or >= 19; "$T" is morning < 12, afternoon < 18, else evening | `map_renderer.cpp`, `talk.cpp` | own / Nuvie | `sub_1EA97` (conversation variables, sets the time word), the lighting code `sub_3AFCF` and `byte_3DA20`. |
| 9 | Per-minute hazards: poison 1 hp with chance 1/8, swamp tiles 2-5 and 1165, fire tiles, traps, force fields, torch burning (50 %, quantity as burn time, initial 100) | `commands.cpp` `Game::partyHazards` | Nuvie | the per-actor loop of `sub_13305` (poison/fire/torches/alcohol/hunger), `sub_2B4D1`-area helpers. Hunger/food and alcohol are **missing**. The initial burn time of a lit torch is own. |
| 10 | Magic points return `+level` every hour | `session.cpp` `Game::advanceTime` | Nuvie | `sub_13305` hour branch (`loc_137CA`). Timers decrementing once a minute **is** exact (`ds:2C5C`, 16 bytes). |
| 11 | Commands that take no time (Look, Talk, Get, Use, Attack takes 1 minute, Cast takes 1 minute, inventory 0) | `commands.cpp` | own | call sites of `sub_13305` in `command_loop` and each `cmd_*`. |

## B. Passability and the map (ported, with known gaps)

| # | What | Where | Source | Notes |
|---|---|---|---|---|
| 12 | Order in which objects on a square are examined (original walks a per-row linked list, `sub_197BA`/`sub_199FF`; I walk position order) and the three-byte template at `ds:06F8` used for the east/south/south-east neighbour search is assumed to hold the query position | `movement.cpp` `touching` | partial | can change the answer only where two blocking rules disagree. |
| 13 | Globals that the original keeps and I default: `word_3F002` (members may swap) = true, `word_3F004` (avoid damage) = false, `byte_3EFFD` handling of the Sacred Quest message, per-actor movement flags (`actor_movement_flags_ptr`, bit 0x10) not maintained | `movement.h` `MoveRules` | partial | find where they are set (`sub_2787E`, `sub_13305`, combat code). |
| 14 | Map drawing order of objects within a square (original sorts with `sub_1BE84`), two passes for "top" tiles, no tile animation (ANIMDATA), no per-tile tinting, no line of sight | `map_renderer.cpp` | own | `redraw_screen` = `sub_1297E`, `sub_128E1`, `sub_3AFCF`, driver entry 22 (tint flags), `sub_39866`. |
| 15 | Light: torch radius 4, Light spell 5, Great Light 7, lit lamps/campfires radius 3, black/dithered ring | `map_renderer.cpp` `computeDarkness` | own | the original's light levels: `byte_3DA20`, `sub_3AFCF`, `word_48FCD..48FD1`, driver tint flags; light values per object type. |

## C. Combat, creatures, magic

| # | What | Where | Source | What to analyse |
|---|---|---|---|---|
| 16 | Hit check, damage, armour, experience, "wounded" text thresholds (maximum hp taken as level * 30), no "missed" text in the original (my wording "X missed!" is **invented**) | `combat.cpp` | Nuvie (+ original strings from seg067); tables now exact | combat code in `GAME.EXE` (`cmd_attack`, strings "grazed", "killed", "wounded" in seg067 at 0x3D/0x4A/0x32). |
| 17 | No combat mode / turn order (move points `actor_moves`, `byte_40513/4`), no ranged weapons or ammunition, no spells in combat, no fleeing/berserk, no level gain by experience | `combat.*` | own | `cmd_attack` and the combat loop; "Begin combat!" (seg067 0x22E); level-up rules unknown (Nuvie has none in its scripts I read). |
| 18 | Attack command: asks for a direction and hits an adjacent actor; the original prints "Attack with %s-" / "what?" and uses a cursor target and weapon ranges | `commands.cpp` `Game::attackAt` | own | `cmd_attack`, seg067 0xB4. |
| 19 | Creatures: egg rules (hour window 6-19, quality ranges, chance, count) | `creatures.cpp` | Nuvie | the original egg code (search `sub_` that tests object type 0x14F near the party; called from `update_object_blocks` / `sub_1B...`). |
| 20 | Creatures hunt the nearest party member within 12 squares if their alignment is evil or chaotic; others wander 1 time in 3; despawn beyond 40 squares; spawn placement ring of radius 3; stats are the table's, not randomised | `creatures.cpp` | own | original AI worktypes (see 3), `actorInit` equivalent (Nuvie randomises stats `actorRandomiseStat`), egg `toss_actor`. |
| 21 | ~~Creature statistics, weapon damage and armour tables~~ | `combat_tables.inc` | **exact** (generated from `GAME.EXE`, see nuvie_differences #11-12) | done; creature inventory/spell columns not used yet. |
| 22 | Companions fight back automatically (adjacent, or walk up to 5 squares) | `creatures.cpp` | own | the party-member worktypes 2-0xB in `sub_13305`. |
| 23 | Casting: entering circle + syllable letters, then a cursor; the original selects the spell in the spellbook view (panel mode 0xA1, `sub_213E1`, `sub_21671`, `sub_21833`) and the target by cursor. Messages, order and payment now follow `cmd_cast` (nuvie_differences #14); the effect function `sub_25495` / `sub_223BC` is not yet read | `commands.cpp` `castKey`, `castAt` | own | `cmd_cast`, `check_spellbook`, the spell views (`word_3DD10` 0x92), seg067 "That spell is not in thy spellbook!". |
| 24 | Spell costs (circle), level requirement, reagent bit order, which spells work and their damage numbers (Harm 5, Heal 1-30, Fireball 1-30, Lightning 1-30, Magic Arrow 1-10, Poison check) ; no projectiles/animations, range, line of sight; time cost 1 minute | `magic.cpp`, `spell_table.inc` | Nuvie | the spell tables and handlers in `GAME.EXE` (find via the strings "Heal", "Mass Sleep"; `sub_30B6A`-area); reagent mixing command is **missing**. |
| 25 | Death: Avatar death prints "Thou art dead!" and ends the program; the original returns to the throne room (`actorAvatarDeath`). Party members that die leave the party (karma not changed) | `session.cpp`, `combat.cpp` | own | death handling in `GAME.EXE` (search "dead" strings). |

## D. Objects, inventory, use

| # | What | Where | Source | What to analyse |
|---|---|---|---|---|
| 26 | Get now follows `cmd_get` (nuvie_differences #15); still missing: cursor selection and range ("Out of range!"), stealing ("Stop Thief!!!", `sub_2F7E3`), free-hand rule for lit torches, stack merge details (`sub_1B701`), special pick-up conversions beyond the four listed | `commands.cpp` `getAt` | partial | `cmd_get` lines after 3138F, `sub_2F7E3`, `sub_1B59D`, `sub_1B701`. |
| 27 | Quantities: gold word, other stackables byte (exact); merging stacks ignores readied/frame cases other than lit torch | `world.cpp` | partial | `sub_1B687`, `sub_1B6BD`. |
| 28 | Inventory view: grid of tiles, own key set, "readied" = location 0x18 without slots (head, neck, hands, ...), several readied weapons allowed, no armour slot rules | `stats_view.cpp`, `commands.cpp` `inventoryKey` | own | `sub_15371`, `sub_1520E`, `sub_16125` (inventory/paper-doll drawing and slot layout), `cmd_ready` ("torch" string), the slot tables. |
| 29 | Look: only the top object, weight line wording, no actor health/state text, no "searching here" rule, cursor keys | `session.cpp` `Game::look` | partial | `cmd_look` (68342, 600 lines), `sub_2F969`, `sub_2D04C`. |
| 30 | Use: doors, ladders (formulas from Nuvie), containers (**I take all contents at once** - invented), torches (own), skiff/raft boarding (**invented**: the original makes actor 0 the vehicle) | `commands.cpp` | Nuvie / own | the 85-type Use table (`cmd_use_types` 0x35AF7, handlers at +0x2F690), `sub_26CC4`, vehicles code. Ships, horses, balloons, levers, bells, wells, instruments, runes, keys, food, books, signs etc. are **missing**. |
| 31 | Rest: asks hours, then heals `1 + level/2` hp per hour; none of the original's rules ("Who will guard?", "Mealtime!", interruptions, "Not while ...!", camping, bed use) | `commands.cpp` `restKey` | own | `cmd_rest` (strings at seg/dseg 211830+). |
| 32 | Dropping puts the object on the party leader's square; no container/drop target rules | `commands.cpp` | own | `cmd_drop`. |

## E. Conversations

| # | What | Where | Source | What to analyse |
|---|---|---|---|---|
| 33 | The interpreter follows Nuvie's, not `conv_execute_command` (201EB) / `conv_read_value` (1F1BE) line by line; Resurrect, Sleep, Horse are stubs; "weight", "can carry" use my formulas; Join: party limit 8, the new member's name from the script header | `conversation.cpp`, `talk.cpp` | Nuvie / own | the original handlers listed in `docs/u6_conversation.md`; `sub_1EA97` (variable set-up, exact list of the 0x25 variables), `sub_1EE03`, `sub_1EF3B`, party join code. |
| 34 | Keyword matching: prefix comparison up to the keyword's length (Nuvie); the original's rule is unverified | `conversation.cpp` `matchKeywords` | Nuvie | the `0xEF` keyword handler in `conv_execute_command` and `sub_2F895`. |
| 35 | Text escapes handled: `$G $N $P $T $Y $Z $n #n / \ + ~ {`; the original's scroll codes (`# $ & * / < > @ \ ^ \``, `scroll_print_wrapped` 17A1D, `scroll_printf` 15637) are only partly the same; `*` page break shows the star and erases it | `conversation.cpp`, `text_window.cpp` | Nuvie / own | exact meaning of each code (`&` toggles?, `<`/`>` bold, `@`, `^`, `` ` ``), word wrap rules, scrolling and "more" behaviour. |
| 36 | Talk target is an adjacent actor chosen by arrow key; the original uses the cursor (`sub_146A9`) and checks "Not on screen", "Not in solo mode", "Talking to yourself?", "You hear a deep moan" | `commands.cpp` `talkAt` | own | `cmd_talk` (35130). |
| 37 | Look command typed in a conversation, empty input -> "bye" | `conversation.cpp` | Nuvie | `cmd_talk` input loop (35483 region). |

## F. Screens and files

| # | What | Where | Source | What to analyse |
|---|---|---|---|---|
| 38 | Main menu, name/sex/portrait/gypsy screens: positions, text, selection marker, key handling, palette choices (menu 0, gypsy 6, portraits on U6PAL) are my own layout; the Introduction, Transfer and Acknowledgements items are stubs; random seed from the clock | `front_end.cpp` | own | `U.EXE`: `main_menu` 113A5, `create_character` 152C9, `enter_name_and_sex`, `show_text_page` 19A0B, `title_sequence` 17A36, `intro_story` 1C678, `acknowledgements` 16766, `transfer_*`. The gypsy's vial-pouring animation and story pages are missing. |
| 39 | Portrait numbering of the six faces per sex (female 0-5, male 6-11 as in MONTAGE.SHP order) | `front_end.cpp` | own | `create_character`'s use of the portrait number and `PORTRAIT.Z`. |
| 40 | Party panel buttons (the four tiles) do nothing; panel modes 0x91/0x92/0x9E (spell, inventory, other views) are not the original's; the status line below the map is missing | `stats_view.cpp` | own | `sub_16125`, `sub_13A67`, `sub_1D25C`, `sub_1E2B5`, `sub_2F969`. |
| 41 | Save files: OBJLIST layout and OBJBLK record layout are the original's; **which block file holds inventory objects and contained objects, and their order, is my choice**; the original unloads/loads blocks as the party moves and keeps modified blocks as `.tmp` files; no `SAVED.GAM`-style names, no save slots | `world.cpp` `World::save` | partial | `save_objlist`, `update_object_blocks` (25911), `commit_objblk_tmp_files` (1C88A), `write_objblock`. |
| 42 | Text window: simple word wrap at 17 columns and scrolling by whole lines | `text_window.cpp` | own | `scroll_print_wrapped`, `scroll_printf`, `sub_1753A`, `sub_17099` (input). |
| 43 | Sound, music, the mouse, the animated intro and the ending are not implemented | - | - | see `u6_music.md`, `u6_drivers.md`, `u6_ending.md`. |

## G. Things that were checked and are believed exact

Passability rules (apart from #12, #13), the object-list and savegame layouts, schedule selection (`update_actor_schedules`), the stackable table, quantity kinds, the
conversation script grammar and file layer, LZW, map/chunk/object block loading, tile and shape formats, the stats panel contents, maximum hit/magic point formulas
(`sub_2B519`, `sub_2B549`), the timer array decrement, the gypsy question logic and new-game objlist patch.
