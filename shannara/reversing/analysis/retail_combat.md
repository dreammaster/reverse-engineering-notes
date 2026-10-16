# Retail combat engine (tenth and eleventh passes, 2026-10-16)

Region `0x13000-0x17C00` of `Shannara.idb`. Read from `Shannara.asm`; names are working
names (`?`). Applied by `scripts/apply_retail_objects.py`. Confidence is stated per item;
**nothing here is validated against a running game**.

## 1. Shape of the system

Combat is **real-time with per-combatant action timers**, not strict turns: each combatant carries
the time of its next action (`+46h`), and the loop repeatedly asks "who is ready?". Encounter scripting is
**compiled event hooks** (one per scripted encounter, `enc_hook_NN?`), in the same spirit as the object
handlers (see `retail_game_layer.md` section 2.4); the *default* rules (target choice, damage, hit/death
reactions) are generic engine code and the hook only overrides them. Enemy "AI" is therefore just the
per-combatant targeting strategy word `+18h`.

```
start_encounter?(id 0..25)                         0x17310, 2,440 bytes
  snapshot _char_stats (0x15E bytes) -> heap          (restored for "Try again")
  music entries 1, 2;  party-template tweaks by story state (combat_stats_set_a..d?)
  switch(id): clone_combatant?(template) ... ; add_combatants?(0, c1, c2, ..., 0) ; pick event hook
  combat_loop?(hook) -> result
  result == -1 (defeat): choice_dialog?("You have fallen in battle. What would you like to do?")
        -> Try again (re-enter at the snapshot) / Restore (load_game?) / ...
  music entry 3; return result  (1 = victory, 0 = no fight, -1 = defeat)
```

## 2. Data

| Name | Address | Content |
|---|---|---|
| `_char_stats` | `0xADC4C` | **0x15E bytes = 25 x 14-byte** per-character stat records (also save region 10). Word `+2` = current HP, word `+4` = max HP (read by `combat_init_combatant?`). Snapshotted/restored around a fight. |
| `_combatant_templates` | `0xA8FD4` | **19 x 0x3F8 (1,016) bytes**: 11 enemy templates (`A8FD4, A93CC, A97C4, A9BBC, A9FB4, AA3AC, AA7A4, AAB9C, AAF94, AB38C, AB784`), then **8 party templates** (`ABB7C, ABF74, AC36C, AC764, ACB5C, ACF54, AD34C, AD744`). The `"a1.Q"`-style string comments IDA shows beside these are false string matches. |
| `_combatants` | `0xCCA44` | 23 slots of pointers to live 0x3F8-byte combatant blocks (0 = free; 23 is also the hard limit enforced in `add_combatants?`). |

**Combatant block (0x3F8 bytes) - fields seen so far**

| Off | Meaning (confidence) |
|---|---|
| `+00` w | combatant id. `< 100` = party member, index into `_char_stats` (`combatant_id_is_party?`); `>= 100` = monster (encounter code writes `0x3EE` for some) - high |
| `+02` w | action period (added to "now" when the combatant acts, `combat_pick_ready?`) - medium |
| `+04` d | pointer to this combatant's array of 22-byte animation records - high |
| `+0A/+0C/+0E` w | **hit % / dodge % / attack power** (section 4) - medium; set by `combat_stats_set_*?` and by encounter 1 (`+0Ah = 100`, `+0Eh = 18`) |
| `+18` w | targeting strategy (see section 4) - high |
| `+1A` d | current animation record pointer (`combat_set_anim?`) - high |
| `+1E..+25`, `+26..+2D` | two screen rectangles (x1,y1,x2,y2 words), copied from slot tables `unk_A7FF8`/`unk_A7F20` (8 bytes per slot) - medium |
| `+2E` w | constant `0x50B` at init - unknown |
| `+30/+32` w | from tables `word_A8050/A8052`, `A8080/A8082` (4-byte stride) - unknown |
| `+38` w | **1 = monster/enemy side, 0 = party** - high |
| `+3A` w | **current HP** (0 = dead; every liveness test uses it) - high |
| `+3C` w | max HP (100 default for monsters; encounters override `+3A`, e.g. `0x1F4`) - high |
| `+3E` w | 1 while defending (doubles dodge), 0 when attacking - medium |
| `+40` w | **animation state = index into the 22-byte animation records** at `[+4]`: 0 idle, 1-3 attack, 4 defend, 5 hit, 6 dying - high (section 4) |
| `+44` w | cleared at init |
| `+46` d | **next-action time** (compared with the game clock) - high |
| `+4A` w | 1 = in use (set by `clone_combatant?`) - medium |
| `+4C` w | initialised flag (0 until `combat_init_combatant?` ran) - medium |
| `+4E` d | pointer to the current opponent/target combatant - medium |

## 3. Functions named

| Address | Name | Role |
|---|---|---|
| `0x17310` | `start_encounter?` | above |
| `0x13CDC` | `add_combatants?(initNow, c1, ..., 0)` | insert into first free `_combatants` slot; `initNow != 0` initialises immediately (mid-fight reinforcements) |
| `0x15758` | `clone_combatant?(template)` | heap copy, `+4Ah = 1` |
| `0x14C94` | `combat_init_all?` | init every table slot |
| `0x14CC8` | `combat_init_combatant?` | rectangles, HP from `_char_stats` (party) or 100 (monster), UI notify, `+4Ch = 1` |
| `0x14CBC` | `combatant_id_is_party?` | `id < 100` |
| `0x14C50` | `combat_pick_ready?(table, now)` | first living combatant with `now >= +46h`; reschedules it |
| `0x14BD0` | `combat_check_outcome?` | 0 fight on / -1 defeat / 1 victory (counts living by `+38h`) |
| `0x14010` | `combat_loop?(hook)` | main loop; `hook` is stored in `_old_keyboard_vector` (a leftover name) |
| `0x14B34` | `combat_hook_notify?` | calls the hook with code 7 ("pick action") |
| `0x132EC` | `choice_dialog?` | **generic modal button dialog** (used far outside combat: Quit?, Restore?, Help, Overwrite/Delete the saved game?...) |
| `0x15B08, 15B4C, 15B90, 15BD0` | `combat_stats_set_a..d?` | rewrite party template 0 (`0xABB7C`) or 5 (`0xACF54`) depending on whether object `0x2F` is inside object 3 / object `0x39` inside object 5; *a* was the false name `reset_bevel_style?` |
| `0x15CF8..0x170E4` | `enc_hook_00?..enc_hook_14?` | 15 per-encounter event hooks, reached only through `start_encounter?` (data xrefs) |

### Encounter table (id -> setup)

ids 0-14 pass their own `enc_hook_NN?`; ids 15-23 and 25 (all call `sub_171BC` first) run with **no AI hook**; id 24 starts no
fight (returns 0). Enemy mix per id is visible in `start_encounter?` as the `clone_combatant?` calls
(e.g. id 8 = five copies of template `A97C4`; id 9 = a single `AB784` plus one party template).
Among its callers are `play_cutscene?` and `sub_2E4C0`, i.e. fights are triggered from story code.

## 4. The hook protocol and the default rules (eleventh pass)

`hook(combatant = eax, code = edx)` returns **-1 = "do the default"**; any other value is an override. Codes
(`edx`), with the engine call site that sends each:

| Code | Sent by | Meaning / return |
|---|---|---|
| 0, combatant = NULL | `combat_loop?` | fight start (`edx` = 0) and then **round tick** with `edx` = number of completed rounds. Encounter scripts use ticks for scripted events (e.g. encounter 3: round 1 calls `add_combatants?(1, ...)` to bring in the whole party and shows dialog 0xC2B; later rounds add allies). |
| 1 | `combat_begin_turn?` | a combatant's turn starts: the hook may set `+40h` (state) itself, e.g. a helper that joins when the hero's HP < 45 |
| 2 | `combat_choose_target?` | pick a target |
| 3 | `combat_roll_damage?` | **damage override**: the return value is the damage (examples seen: 1000 = instant kill, 400) |
| 4 | `combat_apply_damage?` | after damage was applied (death dialogs, e.g. "boss at 0 HP -> show_dialog 0xC6E") |
| 5 / 6 / 7 | `combat_do_attack?` / `combat_do_defend?` / `combat_hook_notify?` | attack begins / defend / idle |
| 8 | `combat_player_input?` | the hero tries to **run**; a non-(-1) answer refuses (several boss fights show a "can't run" dialog); -1 = flee succeeds (fight result 0) |
| 10 | `combat_step_anim?` | the current animation finished |

Fight results: `combat_check_outcome?` 1 = victory, -1 = defeat; fleeing yields 0.

**Combatant state `+40h` is an animation index.** Each combatant's `+4` points at 22-byte (0x16)
animation records: 0 idle, 1-3 attack variants (`combat_do_attack?` picks `1 + rand()%3`), 4 defend, 5 hit
reaction, 6 death. `combat_set_anim?` installs a record as current (`+1Ah`); the low nibble of a record's
byte 0 is its **type: 2 = picture sequence, 4 = FLC animation (player state embedded at combatant `+52h`),
8 = Q video (`q_open`/`q_load`/`q_expand`, flag bits in combatant `+3F6h`)**, bit 0x80 = loop; record `+4` =
frame count, `+6` = impact frame, `+8` = file/resource id, `+0Ah` = sound id, `+10h` = current frame. So **the
Q codec traced in the demo is used by the retail game for combat animations**, in addition to FLC.

**Default rules:**
- *Frame loop:* `combat_pick_ready?` yields the next ready combatant; the loop steps its animation with
  `combat_step_anim?`; at the **impact frame** (bit 0) damage is rolled and applied to the target
  (`combat_roll_damage?`, `combat_apply_damage?`); at animation end (bit 2) the combatant goes back to idle
  (`combat_hook_notify?`, code 7) and its next action is scheduled `+2` ticks later.
- *Target choice* (`combat_choose_target?`, monsters only): the strategy `+18h` is re-rolled from `rand()%9`
  (0-5 -> 1, 6-7 -> 10, 8 -> 7): 1/3/5/6 nearest living opponent by rectangle-centre distance, 2/4 first
  living opponent, 7 weakest, 9 whatever the hero is attacking, 10 strongest. Strategy 3 on a party member
  means "defend" (`combat_begin_turn?`).
- *Damage* (`combat_roll_damage?`): attacker hit chance = word `+0Ah` percent (100 = never misses), target
  dodge = word `+0Ch` percent (doubled while defending, `+3Eh`), damage = attacker word `+0Eh` + `rand()%5`.
  `sub_254B8(pct)` is the engine's random-percent test (also used for the 50/50 enemy variants in
  `start_encounter?`). HP floors at 0; 0 HP plays the death animation.
- *Hero input* (`combat_player_input?`, only combatant id 0): keys a/d/o/r and screen buttons 0x501-0x504
  (attack, defend, orders menu `sub_158AC` = "Give orders to...", run); clicking a monster rectangle targets it.
  Other party members and all monsters act automatically.
- *HP bar* (`combat_draw_hp_bar?`) is plain rectangles through `gxSaveColors`/`gxDirtyDisplay`.

For a port the combat core is small (a few hundred lines); each of the 15 scripted fights needs its hook
ported by hand.

## 5. Open questions

- Hook bodies are only sampled (`enc_hook_00..03`, `14`); `sub_15C24` (default shared by hooks 0 and 3),
  `sub_16208` (953 B), `sub_16BBC` (1,192 B) and `sub_158AC` (the "Give orders to..." menu, 511 B) hold more rules.
- Template `+8`: low word is a picture-id offset (0x518-0x51E for party portraits), high word `+0Ah` is hit %.
- `sub_171BC` (shared preamble of ids 15-25: puts every party member present, `party_has_member?` ids
  4, 0xB, 8, 5, 7, 9, ..., into the fight), `sub_13C80`/`sub_13F0C` (pre/post fight), `sub_14250` (UI event
  handler called from `dispatch_event?`) are unnamed.
- `_old_keyboard_vector` should be renamed `_combat_hook` (left alone to avoid touching every reference).
