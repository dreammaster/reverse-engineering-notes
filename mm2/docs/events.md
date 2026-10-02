# Map events (scripts)

Loaded per map from `EVENTSI.DAT` / `EVENTSO.DAT` into `DGROUP:6052` (`load_map_events`
`16008`, chunk = map number).  Interpreter: 2PLAY `evt_run_script` (`1A606`); trigger scan:
`evt_check_cell_triggers` (`1A8C4`).  All lengths/format below were verified by parsing every map
chunk (0-59): the script area always ends exactly on a command boundary.

```
chunk (after LZW):
  trigger table   3-byte entries { cell = y<<4|x , script number , facing mask }   ... 00 00 00
  u16 size        of the script area, counted from this word
  scripts         each script = commands ... FFh
  messages        FFh-terminated strings (chars & 7Fh; 40h = newline)
```

* A trigger fires when the party is on `cell` and `facing_mask & direction_bit` (direction bits: **W = 10h, S = 20h, E = 40h, N = 80h**, `F0h` = any
  direction; table `DGROUP:16D2[byte_23217]`).  Script *n* is the n-th `FFh`
  terminated block (`evt_run_script` skips `n` blocks).
* If the cell has none of its triggers armed for this facing, the "no trigger" path runs a random
  encounter (`start_combat`).
* Message commands take a **message number**; `evt_seek_message` walks the message area.

## Command set

`length` = total bytes including the opcode (DGROUP `15E6` table, `EVENT_OPCODE_LEN` in
`tools/mm2_data.py`).  Register `cond` = `byte_1DC7F` (result of the last check).
Semantics are from reading the handlers; names in the IDA databases: `evt_opNN_*`.

| Op | Len | Handler | Meaning |
|---:|---:|---|---|
| 1 | 2 | `1905E` | show message *n* |
| 2 | 2 | `19074` | show message *n* at text row 13h |
| 3 | 2 | `190F2` | message in the framed lower window |
| 4 | 2 | `19110` | centred title line |
| 5 | 2 | `19160` | message in a small box |
| 6 | 2 | `191EC` | message in a framed box |
| 7, 8 | 1 | `193B8`, `1940E` | wait for a key (8 keeps the music running) |
| 9, 10 | 1 | `1941E`, `1946E` | Y/N prompt → `cond` |
| 11 | 3 | `1947E` | show monster picture (id, position) |
| 12 | 3 | `194D4` | teleport (map id, `y<<4|x`); bit 7 of the map id = random cell |
| 13 | 2 | `19560` | play sound effect |
| 14 | 2 | `19716` | enter location: 1 inn, 2 training hall, 3 tavern, 4/5 temple / mage guild, 6 blacksmith, 7/8 tavern variants; 64h quest prompt; 7Eh slide (enter X,Y and teleport), 7Fh ambush, 80h teleport trap that halves stats, 81h-83h found item, C9h/CAh/CBh/CCh/CDh/CEh/CFh special caves events (donate experience/gems, pick character, time travel to an era), E2h town crier (message of the day), FDh blacksmith robbery (guards); anything else = map entrance (`1956E`) |
| 15 | 1 | `198C8` | end script |
| 16, 17 | 2 | `198D2`, `198F2` | skip *n* commands if `cond` / if not `cond` |
| 18 | 13 | `19912` | fight: 10 monster ids + 2 parameters (`start_combat`) |
| 19 | 11 | `19984` | fight (no parameter bytes) |
| 20 | 1 | `19990` | clear this cell's trigger flag (one-shot events) |
| 21, 24 | 4, 5 | `19A02` | per-character check/modify |
| 22 | 3 | `19ABC` | has item *n* → `cond` |
| 23 | 3 | `19B20` | `cond` = event variable *n* (`evt_var_addr` `18E22` maps indices to DGROUP bytes) |
| 25 | 5 | `19B44` | give item to the first character with a free slot (else party pool) |
| 26 | 3 | `19C1A` | set event variable |
| 27 | 2 | `19C40` | `cond = 0` if arg > `cond` |
| 28 | 2 | `19C5E` | `cond` = random(1, arg) |
| 29, 30 | 2 | `19C72`, `19C8A` | wait / delay with key abort |
| 31, 32 | 7 | `19E40`, `19F38` | modify a character attribute (one / whole party) |
| 33 | 4 | `19F44` | set map cell (`y<<4|x`, wall byte, flag byte) |
| 34, 35 | 3 | `19F90`, `19FC6` | time-of-day / date checks → `cond` |
| 36 | 3 | `1A01E` | pay gold (word) → `cond` |
| 37 | 3? | `1A04C` | pay gems (word) → `cond` (table says 2; unused in data) |
| 38, 39 | 1 | `1A082` | choose a party member (1-8) |
| 40 | 3 | `1A126` | remove item from the party |
| 41 | 1 | `1A19A` | stop |
| 42 | 15 | `1A1A0` | **place treasure** on this spot: gold (3 bytes -> `dword_241AC`), gems (word -> `word_241AA`), three items (item id / flags / quality bytes -> `DGROUP:6950/6953/6956`) and sets `byte_1DC84` = FFh; the 'S' Search command (`13814`) then opens `2MISC:party_search` |
| 43 | 2 | `1A1E2` | skip *n* if night |
| 44 | 2 | `1A202` | add value to `word_1DC18` |
| 45 | 3 | `1A21E` | check character class / race / alignment |
| 46 | 3 | `1A386` | teach skill/spell bits |
| 47 | 1 | `1A404` | read a string from the player (10 chars) |
| 48 | 11 | `1A45A` | compare typed string with 10 encoded bytes (password puzzle) → `cond` |
| 49 | 4 | `1A4BC` | award experience |
| 50 | 2 | `1A570` | `cond` = number of party skill slots equal to skill *n* (`party_skill_count`; e.g. Cartographer, Mountaineer) |

Opcode usage across the shipped maps: see `tools/mm2_data.py` (`python -c` with `split_scripts`).

`tools/mm2_events.py MAP` prints the trigger table, the scripts with opcode names and the message texts of a map.

## Character opcodes (decoded in detail)

The character-related commands (`evt_op21/24/31/32`, `ovl/2PLAY.asm` 0x19A02, 0x19B38, 0x19E40, 0x19F38 with the field reader at
0x1AA00 and `sub_19CB8` at 0x19CB8) use **field ids**: id -> byte of the 82h-byte character record (jump table at 0x1AA56, extracted
into `src/mm2_evfields_gen.inc`); ids 20h/28h/35h/38h/3Ah/3Ch address a word, 31h and 3Eh (experience, gold) a dword, the ids that
follow them address the higher bytes (+1, +2, +3).  Ids 0/1 return the roster id.  "who": 0 = whole party, 1-8 = that member, 9 =
the member picked by opcode 38/39 (else the previous `cond`), +80h on the operand = use the previous `cond` as the value.

| Op | Operands | Meaning |
|---|---|---|
| 21 | who, field, mask | `cond` = OR over the addressed members of (field byte & mask) (mask 0 = no masking) |
| 24 | who, field, mask, value | field byte = (field byte & mask) \| value |
| 31 | who, field, nbytes, amount(3) | **add** amount to the field (8-bit saturates at 255, 32-bit saturates, 16-bit wraps to 0) |
| 32 | same | **subtract**; if the field is smaller, `cond = 0` and nothing is written |
| 22 | x, item | `cond` = number of matching slots (equipped + backpack) of the first member that has the item |
| 25 | x, item, charges, bonus | give the item to the first member with a free slot (`cond` = 1), else it goes to the floor |
| 40 | x, item | remove the item from the first member's backpack (`cond` = 1) |
| 36 / 37 | amount (word) | pay gold / gems from the party pool (non-hirelings), then re-share; `cond` = success |
| 34 | lo, hi | `cond` = (lo <= **era** <= hi) -- the disassembly's "check hour" actually tests the era |
| 35 | lo, hi | day of year in range; lo = B5h: odd day, B6h: even day |
| 33 | cell, wall, flag | set the map cell's wall and flag bytes |
| 38 / 39 | - | pick a party member (key 1-8 or Esc, which ends the script); `cond` = slot |
