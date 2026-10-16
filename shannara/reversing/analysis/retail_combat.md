# Retail combat engine (tenth pass, 2026-10-16)

Region `0x13000-0x17C00` of `Shannara.idb`. Read from `Shannara.asm`; names are working
names (`?`). Applied by `scripts/apply_retail_objects.py`. Confidence is stated per item;
**nothing here is validated against a running game**.

## 1. Shape of the system

Combat is **real-time with per-combatant action timers**, not strict turns: each combatant carries
the time of its next action (`+46h`), and the loop repeatedly asks "who is ready?". The behaviour of
the enemy side is not data-driven but **compiled AI callbacks** (one per scripted encounter), in the
same spirit as the object handlers (see `retail_game_layer.md` section 2.4).

```
start_encounter?(id 0..25)                         0x17310, 2,440 bytes
  snapshot _char_stats (0x15E bytes) -> heap          (restored for "Try again")
  music entries 1, 2;  party-template tweaks by story state (combat_stats_set_a..d?)
  switch(id): clone_combatant?(template) ... ; add_combatants?(0, c1, c2, ..., 0) ; pick AI hook
  combat_loop?(aiHook) -> result
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
| `+04` d | pointer used to derive the sprite/anim frame (`+16h + rand%3 * 14h`) - low |
| `+0A/+0C/+0E` w | per-template numeric stats set by `combat_stats_set_*?` and by encounter 1 (`+0Ah = 100`, `+0Eh = 18`) - meaning unknown |
| `+18` w | set to 1 at init - unknown |
| `+1E..+25`, `+26..+2D` | two screen rectangles (x1,y1,x2,y2 words), copied from slot tables `unk_A7FF8`/`unk_A7F20` (8 bytes per slot) - medium |
| `+2E` w | constant `0x50B` at init - unknown |
| `+30/+32` w | from tables `word_A8050/A8052`, `A8080/A8082` (4-byte stride) - unknown |
| `+38` w | **1 = monster/enemy side, 0 = party** - high |
| `+3A` w | **current HP** (0 = dead; every liveness test uses it) - high |
| `+3C` w | max HP (100 default for monsters; encounters override `+3A`, e.g. `0x1F4`) - high |
| `+3E` w | set 0/1 by `combat_loop?` around states 1/4 - unknown |
| `+40` w | **action state**: 0 idle -> asks the AI hook (code 7) via `combat_hook_notify?`; 1 an action (probably attack: `sub_14808` + `sub_14B68`); 4 other action (`sub_14774`); 5/6 targeting/finish - medium |
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
| `0x14010` | `combat_loop?(aiHook)` | main loop; `aiHook` is stored in `_old_keyboard_vector` (a leftover name) |
| `0x14B34` | `combat_hook_notify?` | calls the hook with code 7 ("pick action") |
| `0x132EC` | `choice_dialog?` | **generic modal button dialog** (used far outside combat: Quit?, Restore?, Help, Overwrite/Delete the saved game?...) |
| `0x15B08, 15B4C, 15B90, 15BD0` | `combat_stats_set_a..d?` | rewrite party template 0 (`0xABB7C`) or 5 (`0xACF54`) depending on whether object `0x2F` is inside object 3 / object `0x39` inside object 5; *a* was the false name `reset_bevel_style?` |
| `0x15CF8..0x170E4` | `enc_ai_00?..enc_ai_14?` | 15 per-encounter AI callbacks, reached only through `start_encounter?` (data xrefs) |

### Encounter table (id -> setup)

ids 0-14 pass their own `enc_ai_NN?`; ids 15-23 and 25 (all call `sub_171BC` first) run with **no AI hook**; id 24 starts no
fight (returns 0). Enemy mix per id is visible in `start_encounter?` as the `clone_combatant?` calls
(e.g. id 8 = five copies of template `A97C4`; id 9 = a single `AB784` plus one party template).
Among its callers are `play_cutscene?` and `sub_2E4C0`, i.e. fights are triggered from story code.

## 4. Open questions

- The AI callbacks themselves (`enc_ai_00?..14?`, ~4 KB) and the action routines (`sub_14808` attack, `sub_14774`,
  `sub_14438` 826 B, `sub_14ED0` 590 B, `sub_16208` 953 B, `sub_16BBC` 1192 B) are **not read yet** - damage formula,
  hit chance, magic and item use all live there.
- `sub_171BC` (shared preamble for ids 15-25), `sub_13C80`/`sub_13F0C` (pre/post fight), and the many small
  `sub_15xxx` UI helpers (hit-point bars, cursor) are unnamed.
- `_old_keyboard_vector` should be renamed `_combat_hook` (it is a leftover demo name; left alone to avoid touching
  every reference in this pass).
