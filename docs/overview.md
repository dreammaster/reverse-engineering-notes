# Ultima II (DOS) — Disassembly Overview

Working notes on the `ultima2.idb` / `ultima2.asm` reverse-engineering
effort. Goal: fully document the DOS executable well enough to write a
clean C++ reimplementation and, from that, a ScummVM engine module.

This file is the entry point into `docs/`. See also:
- [file-formats.md](file-formats.md) — on-disk data formats (maps, dungeons,
  talk files, save file, pics), cross-referenced against the disassembly.
- [roadmap.md](roadmap.md) — prioritized list of what's investigated vs.
  still open.

Status as of this pass: 145 `proc` blocks total, 79 still placeholder
`sub_XXXXX` names (both counts jumped from 93/58 after resolving the A-Z
command jump table below — most of the newly-visible code was previously
orphaned/undefined and is still unnamed). `ultima2.asm` is ~28.7k lines.

## Platform

- Original release: Apple II (1982). This IDB is the **1983 DOS port**,
  16-bit real-mode x86, segmented `MZ` executable (`ULTIMAII.EXE`).
- Three segments in the IDB: `sg01a2` (main code, huge — contains gameplay
  logic, movement, text/UI, save/load), `seg002` (second code segment,
  starts asm line 27525), `seg003` (stack segment, line 28713). The
  `sg01a2`/`sg08e3`-style names are IDA's auto-generated segment names
  (hex of the segment's start paragraph) — worth renaming once we know
  what each contains, e.g. `sg08e3` currently holds at least `_picData`.

## The "inline data after CALL" trick

This is the central gotcha of this binary and it isn't confined to one
function. The pattern: a function is called with its argument data
(a string, a filename, ...) placed **immediately after the CALL
instruction** in the code stream, rather than passed via register/pointer.
The callee reads its own return address off the stack to find the data,
processes it, then rewrites the return address to point past the data
before it `RET`s. This defeats IDA's linear disassembly (the inline bytes
get decoded as garbage instructions) and, if fixed by hand with
Undefine/String/Code, breaks the function's flowgraph (IDA still thinks
the CALL falls through into the now-defined data).

Two confirmed instances, both fixed via scripts in `ida_scripts/` (see
that directory's docstrings for the exact mechanism — remove the bogus
CALL→data fallthrough edge, add the real CALL→(data end) edge):

| Function | asm line | Inline payload | Terminator |
|---|---|---|---|
| `write_string` | 16755 | display text | null byte (0x00) |
| `access_file` | 17318 | 8-byte DOS filename | fixed length, no terminator (space-padded) |

`access_file` is the shared low-level file I/O routine — see
[file-formats.md](file-formats.md) for how it's used for every file type.
**If you find another function with this shape (pops the return address,
adjusts it, reads from the old value), it's worth adding to this table
and writing a third fix script** — grep the asm for `pop.*bx` /
`add.*bx` near a `proc` start as a starting heuristic, or look for
functions whose callers are followed by suspicious garbage instructions
in the current `.asm`. Checked as of this pass: every other `pop bx` in
the current `.asm` is a mid/end-of-function register restore, not a
first-instruction return-address grab — no third instance found yet, but
that only covers code IDA has already disassembled.

### Resolved: A-Z command jump table, and the `TLKXFF` loader

**Correction**: the jump table lives in `sub_10A30` (asm ~1710-2018,
messy — IDA's own `; sp-analysis failed` comment on its `endp`, and code
clearly owned by this function continues well past that `endp`, e.g. its
`map_get_monster_at?` calls around asm 2960-2988 are still commented
`CODE XREF: sub_10A30+...`), **not** `canMoveToTile` — earlier notes
conflated the two. `canMoveToTile` (asm 6821) is a distinct, separate
function (movement/collision, several disjoint function chunks, called
from the main loop) that `sub_10A30`'s command handlers call into, not
the other way around. `sub_10A30` is the actual single-key command
*dispatcher*: it has the 26-entry A-Z jump table
(`mov bx, cs:[si+730h]` / `jmp bx`, anchored at label `loc_10A1F`, asm
~1746-1760) at `off_10730` → `command_jump_table` (segment start +
0x730), which IDA's switch analysis never recognized on its own. This has now been resolved by hand
(worked out with `ida_scripts/resolve_command_jump_table.py` as a
starting point) — all 26 targets are named procs, in order:
`attack, board, cast, descend, enter, fire, get, hyper, ignite_torch,
jump, klimb, launch, magic, negate_time, offer, pass, quit, ready,
steal, transact, unlock, view, wear_armor, x_it, yell, zstats` (asm
1294-1319, xrefs at each proc's `DATA XREF: sg01a2:off_10730`).

**Correction to the earlier hypothesis**: the orphaned `TLKXFF`-loading
block (asm ~6631-6649, now `load_talk_file`, formerly `sub_122D5`) is
*not* one of these 26 targets — there is no dedicated Talk command in
Ultima II. `T` is `transact` (shop/trade), matching the classic Ultima
key set. Instead, `load_talk_file` is called directly from inside
`enter` (`E`) at three of its five "what did you walk onto" branches —
asm 9136 (`-VILLAGE`, sets `player._mapNum2 = 1`), asm 9159 (`-TOWN`,
`_mapNum2 = 2`), and asm 9206 (`-CASTLE`, `_mapNum2 = 3`), each right
after the `load_map` call for that branch. The `-TOWER` branch (asm
9165-9186, `_mapNum2 = 4`) does **not** call it — towers/dungeons have
no NPCs. So talk data loads automatically the moment you enter a
settlement, not via a standalone keypress.

`load_talk_file` itself (asm 6631-6649) confirms the earlier hand-decode:
patches `player._mapNum1`/`_mapNum2` into the `TLKXFF  ` template (same
digit-patching pattern as `load_map`), then `mov ah, 27h` (FCB random
block read), `mov cx, 100h` (256 bytes), `mov dx, word_17886` (dest
buffer offset `2800h` within the `sg08e3` data segment), `call
access_file`.

**Settled**: file-formats.md's wiki-sourced spec says `tlkx???` is 384
bytes; this DOS port's disassembly reads only 256, and that's confirmed
deliberate, not a bug — full trace in
[file-formats.md](file-formats.md#256-vs-384-byte-discrepancy-traced-and-settled).

**Also traced end-to-end**: the loaded buffer's only consumer is
`print_indexed_shop_string` (asm 3383), called solely from `transact` — so `TLKXFF`
data is shop/response text read during a shopkeeper transaction, not
walk-up dialogue (consistent with there being no Talk key at all). There
is no separate ROT-128 decrypt loop anywhere in the binary; the decode
is an incidental side effect of `write_character`'s universal `and
al,7Fh` (asm 12645), which strips the high bit off every character it
ever prints. Full trace in
[file-formats.md](file-formats.md#consumer-traced-read-out-by-transact-not-a-walk-up-talk).

`off_10730` (the table) and `sub_122D5` (the loader) have been renamed
to `command_jump_table` and `load_talk_file`. That rename (and all
renames since) went through `ida_scripts/apply_renames.py`, a single
accumulating script — see its docstring for the convention, and
`ida_scripts/apply_structs.py` for the struct-member equivalent.

## Known data structures

### `FCB` (asm line 3, sizeof 0x25 = 37 bytes)

Standard (unextended) DOS File Control Block, used for all file I/O in
this game (see `access_file`):

```
drive           db ?          ; 0 = default, 1 = A, ...
filename        db 8 dup(?)   ; space-padded, no dot
extension       db 3 dup(?)
current_block   dw ?
record_size     dw ?
file_size       dd ?
date            dw ?
time            dw ?
reserved        db 8 dup(?)
current_record  db ?
random_record   dd ?
```

The only named FCB instance so far is `_picData` (in `sg08e3`), reused
for every file the game opens via `access_file`, not just pics — the
name is a holdover from earlier tentative work and is worth renaming to
something generic (e.g. `_fileFcb`) once you're touching that area.

### `Savegame` (asm line 22, sizeof 0x100 = 256 bytes)

This is the in-memory player-state struct (backed by the `player` save
file — see file-formats.md). Fields identified so far:

```
_name           db 13 dup(?)
_sex            db ?
_class          db ?
_race           db ?
_mapNum1        db ?   ; ASCII digit ('0'-'9') of current map number, tens? ones?
_mapNum2        db ?   ; the other digit — see load_map, asm 6618
_strength       db ?
_agility        db ?
_stamina        db ?
_charisma       db ?
_wisdom         db ?
_intelligence   db ?
_hp             db 2 dup(?)
_food           db 2 dup(?)
_foodTurnCtr    db ?
_experience     db 2 dup(?)
_gold           db 2 dup(?)
_mapX           db ?
_mapY           db ?
_lastKeypress   db ?
; gap 0x28-0x2A, no individually-referenced members here yet
_readiedWeapon  db ?    ; 0-9, see TEXT_STRINGS section below
_readiedArmor   db ?    ; 0-6, see TEXT_STRINGS section below
_readiedSpell   db ?    ; 0-9, see TEXT_STRINGS section below
_torches        db ?    ; checked/spent by ignite_torch, see file-formats.md's monx?? section
_keys           db ?    ; checked/spent by unlock, +2 from killing a Guard
_thievesTools   db ?    ; saves from a trap death, +1 from killing a Thief
; gap 0x31-0x32
field_33        db ?   ; unnamed, xref'd
field_34        db ?   ; unnamed, xref'd
; ... struct continues to offset 0x100, largely unmapped past here
_gems           db ?    ; dropped +1 by killing a Fighter, spent by view to show the world map
```

`_disableSave` is also referenced (used by `save_game`, asm ~6692) but
not yet located within the struct layout above — worth pinning down.

**`_readiedWeapon`/`_readiedArmor`/`_readiedSpell`/`_torches`/`_keys`/
`_thievesTools`/`_gems`** (formerly `field_2B`/`2C`/`2D`/`2E`/`2F`/`30`/
`A5`) all decoded and renamed — see
[file-formats.md](file-formats.md#monx--monsternpc-data) for
`_torches`/`_keys`/`_thievesTools` (consumable resources dropped by
monster kills) and the
[`TEXT_STRINGS` section below](#text_strings--the-cs4c60h-table-fully-decoded)
for `_readiedWeapon`/`_readiedArmor`/`_readiedSpell`/`_gems`.

Note `_mapNum1`/`_mapNum2` are stored as **raw digit values that get
`+ '0'` at use time** (see `load_map`, asm 6620-6628: `clc` /
`adc al, '0'`), not as pre-formatted ASCII — i.e. the struct fields hold
0-9, not '0'-'9'. Confirm this against the wiki's save-game format notes
if/when a page for it is found.

## Named functions of interest (partial)

| Name | asm line | Notes |
|---|---|---|
| `start_` | 325 | main entry (near) |
| `start` | 27533 | far entry stub in `seg002`, presumably the actual MZ entrypoint that jumps to `start_` |
| `access_file` | 17318 | shared FCB-based file I/O; see above |
| `load_map` | 6618 | loads `mapx??`/`monx??` for current map, patches filename digits from `player._mapNum1/2` |
| `load_talk_file` | 6631 | loads `tlkx??` for the current map; called from `enter` on VILLAGE/TOWN/CASTLE only — see inline-data-trick section above |
| `print_indexed_shop_string` | 3383 | indexed null-string lookup + print into the `load_talk_file` buffer; called only from `transact` (shopkeeper NPC response) |
| `write_character` | 12639 | universal char-output primitive; `and al,7Fh` here is the de facto ROT-128 decode for `tlkx` text, no dedicated decrypt step exists |
| `attack, board, cast, descend, enter, fire, get, hyper, ignite_torch, jump, klimb, launch, magic, negate_time, offer, pass, quit, ready, steal, transact, unlock, view, wear_armor, x_it, yell, zstats` | 1294 table, procs 7930-11082 | the 26 A-Z single-key command handlers, resolved from `off_10730` — see inline-data-trick section above |
| `save_game` / `save_game1` | 6692 | writes `mapx??`/`monx??`/`player` back out |
| `canMoveToTile` | 6884 | large — movement/collision logic, many other functions are subroutines of this |
| `draw_map` | 18152 | renders the map view |
| `write_string` | 16755 | see inline-data trick above |
| `write_character`, `write_stats`, `write_two_numbers`, `write_number`, `write_player_name` | 16788, 17021, 17097, 17461, 16887 | text/UI output helpers |
| `set_cga_mode`, `set_text_pos`, `set_cursor_position`, `set_normal_text_color`, `set_reverse_text_color` | — | CGA display helpers |
| `keypress_check`, `get_character` | 16628, 17483 | input helpers |
| `xorDrawCircle`, `xorSpriteDraw*`, `xorDrawCircleAt` | ~15852-17318 | sprite/cursor XOR-drawing routines (light source circle + player/NPC sprites over the map) |
| `get_player_tile`, `update_points_remaining` | 16948, 18093 | |

79 functions remain `sub_XXXXX` placeholders — biggest untapped source of
understanding is probably `canMoveToTile` and its many `sub_` callees
(movement/combat logic), the newly-exposed bodies of the 26 A-Z command
handlers themselves (each likely calls several still-unnamed helpers),
and the `sub_16xxx` cluster (asm ~19113-20276) which looks like a
self-contained subsystem (dense internal call graph, not yet named).

### Command-handler shared helpers (decoded and applied)

Tallied every `call sub_XXXXX` inside the 26 A-Z command handler bodies
and ranked by how many *different* commands share each helper — reused
helpers are higher-value to decode first since multiple call contexts
cross-check each other. All 8 below applied via `ida_scripts/apply_renames.py`,
confirmed in the `.asm`:

| Name | Called from | What it does |
|---|---|---|
| `rand_byte` (0x15217) | attack, fire, get, offer, steal | PRNG: combines three running BIOS-timer-driven values (`_timer2/5/6`) into `_timer1`, updates a small shift-register history buffer, returns the new byte in AL. The dice-roll used everywhere — hit chance, item drops, spell luck. |
| `find_target_monster` (0x123E6) | attack, fire, offer, transact | Computes the cursor+facing-direction target cell, scans all 32 monster slots for one active (`[di+197h]!=0`) and positioned there, returns its slot index in `di` (0 if none found). The core "who does this command act on" lookup. |
| `read_digit_keypress` (0x12028) | magic, offer, ready, wear_armor | Blocks on keypress until a `'0'`-`'9'` digit, echoes it + a cursor-control byte, returns the numeric value 0-9 in AX. Used for "which spell/item number?" prompts. |
| `print_indexed_menu_string` (0x1508C) | cast, magic, ready, wear_armor, zstats | Same *mechanism* as `print_indexed_shop_string` (find the Nth null-terminated string, print it) but against a **fixed** compile-time table at `cs:4C60h` (`TEXT_STRINGS`), not the runtime talk buffer — see full table below. |
| `alert_town_guards` (0x1113A) | attack, steal | No-op outside a settlement (`player._mapNum2==0`, i.e. tower/dungeon). Inside one: sets `[di+1D7h]=1` for monster slots 0-7 plus the current target slot. Committing a hostile act (attacking or stealing) in town raises an alarm flag on a block of slots — plausibly the town's reserved "guard" slots. Note `[di+1D7h]` is the same byte `transact` reads as a shopkeeper item-index (high bit set); the two uses don't collide since this only ever writes `1`, never `>=0x80`. |
| `try_spend_gold` (0x12052) | offer, transact | Subtracts a 2-byte BCD amount (`byte_17438`/`_sleepFlag2?` as low/high) from `player._gold`; on underflow, undoes the subtraction and prints "YOU DONT HAVE THAT MUCH!"; returns 0 (via AL, checked by caller) on failure. |
| `play_hit_sound` (0x15ED7) | attack | Thin wrapper: calls `play_tone_sweep` with fixed parameters, right after a successful melee hit. |
| `play_tone_sweep` (0x15DA9) | (via `play_hit_sound`, `pause?`, others) | Checks a mute flag (`byte_1795D`, toggled elsewhere via `xor ...,0FFh` — a sound on/off setting), then sweeps the PC speaker from one frequency to another over N steps. Generic tone-sweep primitive reused for multiple sound effects, not attack-specific itself. |

Not chased further this pass (single-caller, lower priority by the
reuse-based ranking above): `sub_15FE0`, `sub_14B26`, `sub_15FA6`,
`sub_172A0` (attack-only), `sub_15EAC` (fire-only), `sub_1361C`
(get-only), `sub_16999`/`sub_15EC7` (launch-only), `sub_129D2`
(view-only).

### `TEXT_STRINGS` — the `cs:4C60h` table, fully decoded

Paul directly formatted this in IDA and pasted the result — a flat list
of 75 null-terminated strings (indices below are 1-based, matching the
addressing convention `print_indexed_menu_string` actually uses: index
0 hits the leading `db 0` and behaves like a huge/invalid index, no
caller passes 0). Confirmed against every caller's index-computation
formula, cross-checking the additive constant each command uses against
where each block of strings starts — all matched exactly:

| Index range | Formula seen in callers | Content |
|---|---|---|
| 1-18 | (not yet tied to a specific caller — likely `view`'s `sub_129D2`, not chased) | Location descriptors: "IN THE WATER.", "IN A MARSH.", "ON GRASS.", "IN WOODS.", "IN THE MTS.", "NEAR A VILLAGE.", "NEAR A TOWNE.", "NEAR A TOWER.", "NEAR A CASTLE.", "NEAR A DUNGEON.", "NEAR A SIGN.", "NEAR A HORSE.", "NEAR A FRIGATE.", "NEAR A PLANE.", "NEAR A ROCKET.", "NEAR ARMOUR.", "NEAR A HOLE.", "ON COBBLE." |
| 19-28 | `ready`: digit(1-9) `+ 0x13`; `zstats`: `_readiedWeapon + 0x13` | Weapons: HANDS(19, unarmed), DAGGER, MACE, AXE, BOW, SWORD, GREAT SWORD, LIGHT SWORD, PHASER, QUICK SWORD(28) |
| 29-35 | `wear_armor`: digit(1-6, wraps 7→0) `+ 0x1D`; `zstats`: `_readiedArmor + 0x1D` | Armor: SKIN(29, none), CLOTH, LEATHER, CHAIN, PLATE, REFLECT, POWER(35) |
| 36-45 | `magic`: digit(0-9) `+ 0x24`; `zstats`: `_readiedSpell + 0x24` | Spells: NONE(36), LIGHT, DOWN LADDER, UP LADDER, PASSWALL, SURFACE, PRAYER, MAGIC MISSILE, BLINK, KILL(45) — matches Ultima II's real spell list |
| 46-61 | not yet tied to a caller | Items/treasure: RING, WAND, STAFF, BOOTS, CLOAK, HELM, GEM, ANKH, RED GEM, SKULL KEY, GREEN GEM, BRASS BUTTON, BLUE TASSLE, STRANGE COIN, GREEN IDOL, TRI-LITHIUM |
| 62-67 | `zstats`: fixed literals `0x3E`-`0x43` (not player-data-driven — these are constant labels printed before each stat's number) | Attributes: STRENGTH(62), AGILITY, STAMINA, CHARISMA, WISDOM, INTELL.(67) |
| 68-71 | `zstats`: `player._race + 0x44` | Races: HUMAN(68), ELF, DWARF, HOBBIT(71) |
| 72-75 | `zstats`: `player._class + 0x48` | Classes: FIGHTER(72), CLERIC, WIZARD, THIEF(75) — same 4 words reused as monster "class" flavor-text keys in `transact` |

This closes out three more `Savegame` fields, confirmed by matching
each command's *write* site against `zstats`' *read* formula for the
same field:

- **`_readiedWeapon`** (0 = unarmed .. 9 = Quick Sword). Set by `ready`
  (asm ~10454) right after "` READY.`"; read by `attack`'s damage
  formula (`_readiedWeapon*8 + _strength`, asm ~8050) — so the readied
  weapon directly feeds melee damage.
- **`_readiedArmor`** (0 = none/Skin .. 6 = Power). Set by `wear_armor`
  (asm ~11053).
- **`_readiedSpell`** (0 = none .. 9 = Kill). Set directly by `magic`
  from the keypress (asm 10089, no intermediate scratch var); fully
  traced into `cast` — see below.

### `cast` — how `_readiedSpell` drives spell effects

`cast` (asm 8667-8680 header, chunks through ~9002 plus a tail at
9707-9741) is a straight-line gate-then-dispatch on `_readiedSpell`:

1. Prints `"CAST-<spell name>"` via `print_indexed_menu_string(_readiedSpell + 0x24)`.
2. Requires `player.field_A1 + player.field_A2 != 0` — i.e. owning at
   least one of two items — else `"NEED WAND OR STAFF!"` and abort.
   Given `TEXT_STRINGS` indices 47/48 are WAND/STAFF, these are almost
   certainly **wand-owned/staff-owned counts** (not yet renamed).
3. Requires `_readiedSpell != 0` (a spell is actually selected) —
   silent abort otherwise, no message.
4. Requires `player.field_80[_readiedSpell] != 0` — a **per-spell
   charge count**, one byte per spell 1-9 (another struct-of-arrays
   member inside `Savegame`, same shape as the armor/weapon-owned
   arrays noted below) — else `"NO SPELL!"`.
5. **Consumes a charge**: `field_80[_readiedSpell]` decremented by 1
   (BCD), unconditionally, before checking whether the cast can even
   succeed — so a failed cast still costs a charge.
6. Requires `player._mapNum2 >= 4` (not inside a settlement — village/
   town/castle are 1-3) else `"-FAILED!"`.
7. Dispatches on `_readiedSpell`'s value to 9 distinct effects, all
   confirmed by reading each branch's actual implementation:

   | `_readiedSpell` | Spell | Effect |
   |---|---|---|
   | 1 | Light | Sets `byte_17436 = 150` — identical mechanism to `ignite_torch` (asm ~9648), same light-duration timer. |
   | 2 / 3 | Down Ladder / Up Ladder | **Context-sensitive**: inside a dungeon (`_mapNum2==4`) they descend/ascend one level — `byte_17435` (confirmed = current dungeon depth, printed as `"TO LEVEL <N>"` at the shared tail, asm 9707-9717) is incremented/decremented, bounded via a validity check (`loc_11451`, not traced). Outside a dungeon the same two branches swap roles and do a different, not-fully-traced search (still via `loc_11451`) — plausibly locating a nearby dungeon entrance to descend into, or the reverse. |
   | 4 | Passwall | Computes a position local to a 16-cell dungeon block; on success, writes `0` (floor) into the map there — carves through a wall. |
   | 5 | Surface | Restores the player's overworld `_mapX/_mapY`, clears loaded monster data, sets `_mapNum2 = 0`, reloads the map — returns from a dungeon to the surface. **Shares its entire implementation with the `klimb` command** (asm 9720, xref'd from both `cast` and `klimb+37`) — "cast Surface" and "climb out" are the same code. |
   | 6 | Prayer | Random chance of failure (`rand_byte`, high bit = fail) via the same tail as Kill (see below) — effect beyond the fail-check not fully traced. |
   | 7 | Magic Missile | Finds a targeted monster via `sub_1330C` (position + type match, not `find_target_monster` — a different monster-lookup helper), then damages its magic-HP pool (`_mapMonsters+0x40`, see `docs/file-formats.md`) by a formula based on `player._experience`. |
   | 8 | Blink | Random short-range teleport within a 16-cell block, position validated via `loc_11451`. |
   | 9 | Kill | Falls into the **same tail as Prayer's success path** (`sub_1330C` → `loc_12ED1`, not traced) — the sharing between Prayer and Kill isn't explained yet, worth a closer look. |

   Not traced further this pass: `loc_11451` (the shared "is this a
   valid destination/target" validator used by 5 of the 9 branches),
   `loc_12ED1` (Prayer/Kill's shared continuation), `sub_15FC3`
   (called once per successful cast before the charge-decrement step),
   `sub_15E83` (called on every failure path — likely a fail sound or
   flash effect, same slot as `play_hit_sound` structurally).

**Also resolves an open item from the `_mapMonsters` decoding pass**:
`_gems` (previously "Fighter-kill drop, +1 BCD, no consumption site
found") is the **gem count**. `view` (asm 10927-10935) requires it
nonzero ("VIEW WHAT?" otherwise) and decrements it on use (asm
10962-10968) — spending a gem to see the world map, matching table
entry 52 (GEM) and the classic Ultima "gem shows the map" item.
Fighters dropping gems is a sensible loot pairing.

**Three per-item "owned"/"charges" flag arrays** now identified inside
`Savegame`, structured like `_mapMonsters` (one byte per possible item,
indexed by a small 1-9-ish choice number) but living inline in the
`player` struct rather than a separate loaded file:
- `player.field_60[di]` — armor-owned flags (`di` = 1-6), checked by
  `wear_armor` before letting you wear something you don't have (asm
  ~11019). Already a recognized (if unnamed) struct member.
- `[di+76h]` (offset 0x76 from `player`, **not yet even a recognized
  struct member** — raw displacement, no `player.` prefix in the
  disassembly) — weapon-owned flags (`di` = 1-9), same role for
  `ready` (asm ~10406).
- `player.field_80[di]` — per-spell charge count (`di` = 1-9), gates
  and is decremented by `cast` (see above). Same shape as the other
  two.

All three are lower-hanging fruit than `_mapMonsters`'s fields since
they're proper array-typed struct members (or nearly so), not a
struct-of-arrays split across a separately-loaded file — `apply_structs.py`
would need a size/array extension to handle them, though, since it
currently only does single-byte/word/dword scalar members.

The four `Savegame` field renames above are applied. The two "owned"
flag arrays are not — see `docs/roadmap.md`.
