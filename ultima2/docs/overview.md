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
- Four segments in the IDB, renamed from IDA's auto-generated
  `sgXXXX`-style names (hex of the segment's start paragraph) once
  their roles were clear (2026-08-18):
  - **`CODE`** (was `sg01a2`, `0x10000`) — the main segment, holds
    nearly all code (`start_`, all 26 A-Z commands, every helper) plus
    embedded read-only data (`TEXT_STRINGS`, the tile graphics, all
    `write_string` literals).
  - **`DATA`** (was `sg08e3`, `0x17410`) — holds `player`/
    `_mapMonsters` and most runtime globals. Was never properly
    classified (`SegClass` was `UNK`), fixed to `DATA` in the same
    pass.
  - **`seg002`** (`0x18AC0`) — the EXE's *actual* DOS entry point
    (header: "Entry Point: 18AC:0"), investigated in full 2026-08-18.
    See below.
  - **`seg003`** (`0x19FE0`) — the stack segment.

### `seg002` — the real DOS entry point, and `start_` is stage two

Found while renaming the auto-named segments: `seg002` isn't leftover
code or dead space, it's where DOS actually starts executing this EXE.
Three pieces:

- **`start`** (`0x18AC0`-`0x18AE0`, 32 bytes) — a clean, legitimate
  bootstrap routine, the *true* entry point (`start_`, which this whole
  project had been treating as the entry point, is really stage two).
  Computes `SS` relative to `DS` (set by the DOS loader at load time —
  real-mode DOS EXEs get relocated at load, so absolute segment values
  aren't fixed at compile time), zeroes all general registers, then
  far-jumps into `start_`.
- **`seg002_unexplained_data`** (`0x18AE0`-`0x18FE0`, 1280 bytes) — was
  disassembled as nonsense x86 (random FPU instructions, a jump to a
  nonsensical far pointer); confirmed not real code, cleaned up to a
  plain byte array (`ida_scripts/clean_seg002_garbage.py`) rather than
  left as misleading fake instructions. **Genuinely unresolved**: not
  simple zero padding either (only 45% zero bytes; the rest is a
  semi-regular repeating pattern, no readable text, no recognizable
  table structure). Left as an open question rather than guessed —
  see `docs/roadmap.md`.
- **`byte_18FE0`** (`0x18FE0`-`0x1A044`, `0x1000` bytes) — already
  correctly marked "Uninitialized", and exactly matches the EXE
  header's "Loaded length: 8FE0h" (`0x10000 + 0x8FE0 = 0x18FE0`): this
  tail was never part of the file at all, just reserved-but-unfilled
  memory the DOS loader allocates without initializing (extra
  stack/heap headroom, a common technique for old EXEs to reserve
  runtime scratch space without paying file-size cost for it).

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
_inSpace        db ?    ; persisted "away from overworld" flag, checked at game-load
_launchMapX     db ?    ; saved launch position, written by `launch`, read back by `klimb`
_launchMapY     db ?
_ringQuestFlag  db ?    ; ring-quest eligibility gate in `offer` ("EARN THE RING!")
; gap 0x37 is _disableSave, see below (kept out of this block since it's
; documented separately due to its richer dual-purpose role)
_patrolWaypoint db ?    ; update_patrol_marker's 4-waypoint cycling index (0/2/4/6)
; gap 0x39-0x3D
_gameSpeed?     db ?    ; 0x3E
; gap 0x3F
_offerRewardItems db 9 dup(?)  ; 0x40-0x48, index 0 unused (code always `inc di` first);
                        ; random reward from offer's 3rd NPC type, slot contents not
                        ; yet cross-referenced against any item table
_enilnoOwned    db ?    ; 0x49, "ENILNO IS YOURS!" ("ONLINE" backwards) -- offer's 3rd
                        ; one-shot quest reward, alongside the Ring and the 8-item array
; gap 0x4A-0x5F, large, unmapped
_armorOwned     db 7 dup(?)   ; 0x60
_weaponOwned    db 10 dup(?)  ; 0x76
_spellCharges   db 10 dup(?)  ; 0x80
; gap 0x8A-0x9F
; 0xA0-0xAF is ONE 16-element treasure-item array, _ringOwned[0..15] --
; see the TEXT_STRINGS treasure-item block writeup below.
_ringOwned      db ?    ; item 0 = RING
_wands          db ?    ; item 1 = WAND, gates cast
_staves         db ?    ; item 2 = STAFF, gates cast
_bootsOwned     db ?    ; item 3 = BOOTS, resists the leg-paralysis trap
_cloakOwned     db ?    ; item 4 = CLOAK, resists the arm-paralysis trap
_helmsOwned     db ?    ; item 5 = HELM, spent by `view`
_gemOwned       db ?    ; item 6 = GEM, array position only
_ankhOwned      db ?    ; item 7 = ANKH, required to board the Rocket in `board`
_redGemOwned    db ?    ; item 8 = RED GEM, array position only
_planeAllowed   db ?    ; item 9 = SKULL KEY; gates boarding the Airplane in `board`
_greenGemOwned  db ?    ; item 10 = GREEN GEM, array position only
_brassButtonOwned db ?  ; item 11 = BRASS BUTTON; required to launch a Plane in `launch`
_frigateAllowed db ?    ; item 12 = BLUE TASSLE; gates boarding the Frigate in `board`
_strangeCoin    db ?    ; item 13 = STRANGE COIN, spent by `negate_time`
_idolOwned      db ?    ; item 14 = GREEN IDOL, resists the sleep trap
_triLithium     db ?    ; item 15 = TRI-LITHIUM; Rocket + hyperwarp fuel
; gap 0xB0-0xFE, huge, unmapped -- field_FF at the very end has zero
; code references anywhere, likely just an IDA size-boundary artifact
```

**`_disableSave`** (offset `0x37`, already a named struct member —
an earlier roadmap note calling it "not yet placed" was simply stale
documentation, not a real gap) is genuinely **dual-purpose**, found
while pinning down the struct's remaining fields (2026-08-18):
- Its primary, most common role: `save_game` and several other
  subsystems (`update_patrol_marker`, `klimb`) treat it as a simple
  boolean — nonzero disables normal saving/background-event
  processing.
- But in the hyperwarp code (`check_hyperwarp_sun_collision` and
  around it), it's also reused as a **current orbit-target index**:
  set to `0`-`8` when the ship's coordinates match one of 9 known
  planets (Earth/Mercury/Venus/Mars/Jupiter/Saturn/Uranus/Neptune/
  Pluto, in that order), or `0xA` for deep space — driving "YOU ARE
  ORBITING ___." Both roles are consistent (any of these values is
  nonzero, so orbiting/deep-space also naturally disables normal
  ground-based saving) rather than contradictory. A third specific
  value, `9`, gates the Castle NPC interaction that unlocks
  `_ringQuestFlag` — distinct from both the planet indices and the
  deep-space marker.
- Kept the existing name rather than renaming (unlike `_gems` →
  `_helmsOwned`, this name isn't *wrong* anywhere — it's just
  incomplete) — same "reused scratch/multi-purpose field" pattern as
  `_circleDeltaX`/`_mapLeft` elsewhere in this codebase.

**`_readiedWeapon`/`_readiedArmor`/`_readiedSpell`/`_torches`/`_keys`/
`_thievesTools`** (formerly `field_2B`/`2C`/`2D`/`2E`/`2F`/`30`) all
decoded and renamed — see
[file-formats.md](file-formats.md#monx--monsternpc-data) for
`_torches`/`_keys`/`_thievesTools` (consumable resources dropped by
monster kills) and the
[`TEXT_STRINGS` section below](#text_strings--the-cs4c60h-table-fully-decoded)
for `_readiedWeapon`/`_readiedArmor`/`_readiedSpell`.

Note `_mapNum1`/`_mapNum2` are stored as **raw digit values that get
`+ '0'` at use time** (see `load_map`, asm 6620-6628: `clc` /
`adc al, '0'`), not as pre-formatted ASCII — i.e. the struct fields hold
0-9, not '0'-'9'. Confirm this against the wiki's save-game format notes
if/when a page for it is found.

### `_mapNum2` value reference

Confirmed from `enter`'s dispatch (asm 6585-6846, sets this field right
before each `load_map` call) — **corrects an earlier assumption in this
doc that tower was `0`**, actually traced properly while chasing
`cast`'s Down/Up Ladder branches:

| Value | Map type |
|---|---|
| 0 | Overworld (surface) |
| 1 | Village |
| 2 | Town |
| 3 | Castle |
| 4 | Tower |
| 5 | Dungeon |

Matches the wiki's "file number ends in 4-5" note for dungeon/tower
maps (`docs/file-formats.md`) — Tower=4, Dungeon=5, not a shared value.
`canMoveToTile`/`cast`'s gate checks (`_mapNum2==0` = overworld,
`_mapNum2>=4` = tower-or-dungeon) both read correctly against this
table; earlier informal descriptions elsewhere in this doc calling
`_mapNum2==4` generically "a dungeon" were imprecise — see the `cast`
Down/Up Ladder writeup below for why the 4-vs-5 distinction actually
matters (towers and dungeons number their levels in opposite
directions).

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
| `alert_town_guards` (0x1113A) | attack, steal | No-op only on the overworld (`player._mapNum2==0` — see the `_mapNum2` value table below). Everywhere else (village/town/castle/tower/dungeon) it sets `[di+1D7h]=1` for monster slots 0-7 plus the current target slot. Committing a hostile act (attacking or stealing) raises an alarm flag on a block of slots — plausibly the town's reserved "guard" slots (the tower/dungeon case is presumably moot in practice, no NPCs there to alert). Note `[di+1D7h]` is the same byte `transact` reads as a shopkeeper item-index (high bit set); the two uses don't collide since this only ever writes `1`, never `>=0x80`. |
| `try_spend_gold` (0x12052) | offer, transact | Subtracts a 2-byte BCD amount (`byte_17438`/`_sleepFlag2?` as low/high) from `player._gold`; on underflow, undoes the subtraction and prints "YOU DONT HAVE THAT MUCH!"; returns 0 (via AL, checked by caller) on failure. |
| `play_hit_sound` (0x15ED7) | attack | Thin wrapper: calls `play_tone_sweep` with fixed parameters, right after a successful melee hit. |
| `play_tone_sweep` (0x15DA9) | (via `play_hit_sound`, `pause?`, others) | Checks a mute flag (`byte_1795D`, toggled elsewhere via `xor ...,0FFh` — a sound on/off setting), then sweeps the PC speaker from one frequency to another over N steps. Generic tone-sweep primitive reused for multiple sound effects, not attack-specific itself. |

Not chased further this pass (single-caller, lower priority by the
reuse-based ranking above): `sub_15FE0`, `sub_15FA6`,
`sub_172A0` (attack-only), `sub_15EAC` (fire-only), `sub_1361C`
(get-only), `sub_16999`/`sub_15EC7` (launch-only), `sub_129D2`
(view-only). (`sub_14B26` was in this list originally but has since
been traced in full — see below.)

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
| 1-18 | **none — dead data, confirmed** (see below) | Location descriptors: "IN THE WATER.", "IN A MARSH.", "ON GRASS.", "IN WOODS.", "IN THE MTS.", "NEAR A VILLAGE.", "NEAR A TOWNE.", "NEAR A TOWER.", "NEAR A CASTLE.", "NEAR A DUNGEON.", "NEAR A SIGN.", "NEAR A HORSE.", "NEAR A FRIGATE.", "NEAR A PLANE.", "NEAR A ROCKET.", "NEAR ARMOUR.", "NEAR A HOLE.", "ON COBBLE." |
| 19-28 | `ready`: digit(1-9) `+ 0x13`; `zstats`: `_readiedWeapon + 0x13` | Weapons: HANDS(19, unarmed), DAGGER, MACE, AXE, BOW, SWORD, GREAT SWORD, LIGHT SWORD, PHASER, QUICK SWORD(28) |
| 29-35 | `wear_armor`: digit(1-6, wraps 7→0) `+ 0x1D`; `zstats`: `_readiedArmor + 0x1D` | Armor: SKIN(29, none), CLOTH, LEATHER, CHAIN, PLATE, REFLECT, POWER(35) |
| 36-45 | `magic`: digit(0-9) `+ 0x24`; `zstats`: `_readiedSpell + 0x24` | Spells: NONE(36), LIGHT, DOWN LADDER, UP LADDER, PASSWALL, SURFACE, PRAYER, MAGIC MISSILE, BLINK, KILL(45) — matches Ultima II's real spell list |
| 46-61 | `zstats`: `_ringOwned[di] + 0x2E` where `di` is the item's slot 0-15 — see below | Items/treasure: RING(46), WAND, STAFF, BOOTS, CLOAK, HELM, GEM, ANKH, RED GEM, SKULL KEY, GREEN GEM, BRASS BUTTON, BLUE TASSLE, STRANGE COIN, GREEN IDOL, TRI-LITHIUM(61) |
| 62-67 | `zstats`: fixed literals `0x3E`-`0x43` (not player-data-driven — these are constant labels printed before each stat's number) | Attributes: STRENGTH(62), AGILITY, STAMINA, CHARISMA, WISDOM, INTELL.(67) |
| 68-71 | `zstats`: `player._race + 0x44` | Races: HUMAN(68), ELF, DWARF, HOBBIT(71) |
| 72-75 | `zstats`: `player._class + 0x48` | Classes: FIGHTER(72), CLERIC, WIZARD, THIEF(75) — same 4 words reused as monster "class" flavor-text keys in `transact` |

### `TEXT_STRINGS` location-descriptor block, traced — dead data

Indices 1-18 (the "IN THE WATER." / "NEAR A ..." strings) are
unreachable in the current disassembly. Checked exhaustively, not just
spot-checked:

- All **23** real call sites of `print_indexed_menu_string` in the
  binary — every single one lands in the weapon (≥19), armor (≥29),
  or spell (≥36) ranges, or is one of the already-documented callers
  (`ready`, `wear_armor`, `magic`, `cast`, `zstats`). None compute an
  index in 1-18. (Several sites weren't previously catalogued —
  turned out to be wandering NPC merchants selling spell scrolls and
  reusing the weapon/armor shop scripts, e.g. a Turkish-flavored
  "MUSERREF OLDUM!... BIR IKI UC...?" scroll-seller — all still just
  reusing the ≥19 ranges, nothing new for 1-18.)
- The location strings (`aInTheWater` etc.) have **zero xrefs**
  anywhere in the `.asm` beyond their own declarations.
- The raw `TEXT_STRINGS[di]` table access only appears twice in the
  whole binary — both inside `print_indexed_menu_string`'s own
  string-walking loop. No other code touches the table directly,
  bypassing the shared helper.
- Ruled out the standing hypothesis directly: `view`'s `sub_129D2`
  (asm 5247-5470) is a pure map-drawing/flood-fill routine — repeated
  calls to a screen-drawing helper (`sub_12B65`) and `sub_14B4E`
  (`flash_screen`'s sibling), walking outward from the player's
  position. **No `write_string` or `print_indexed_menu_string` call
  anywhere in its body.** `view` shows the map graphically; it prints
  no status text at all.

Most likely explanation: a cut or unfinished "status line" feature
(telling the player what terrain they're standing on/near — the
strings read exactly like that) that never got wired up to a caller,
or was wired up in a small piece of code that hasn't been
disassembled/named yet (a candidate for one of the 79 remaining
`sub_XXXXX` functions, though none of the ones already surveyed in
this project show any sign of it). Not pursued further — this is a
settled negative result, not an open thread to keep chasing blindly.

### `TEXT_STRINGS` treasure-item block, traced — a 16-element inventory array, unifying 8 prior findings

Opposite outcome from the location block: **fully wired up**, and
tracing it revealed the entire `Savegame` `0xA0`-`0xAF` cluster — which
this project has been naming field-by-field over several sessions,
somewhat awkwardly — is actually **one 16-byte array**, `_ringOwned[0..15]`,
displayed by `zstats`' `"ITEMS:"` screen (asm ~8935-8973, the loop
bound is confirmed `di < 0x10` = 16, matching the 16 treasure strings
exactly). For each nonzero slot it prints `TEXT_STRINGS[di + 0x2E]`
(0x2E = 46, the block's start) followed by `write_number` of the count
— a real inventory listing, name + quantity, not just a flag.

Cross-checking this array-position formula against every `0xA0`-`0xAF`
field already named in earlier sessions is where this gets
interesting — **7 line up perfectly**, confirming the formula is
right, and along the way this also explains *why* two fields already
named for what they gate (`_planeAllowed`, `_frigateAllowed`) gate
what they do, plus turns up an explicit 8th confirmation that wasn't
in this cluster's neighborhood before:

| `di` | Offset | Item | Current name | Status |
|---|---|---|---|---|
| 0 | `0xA0` | RING | `_ringOwned` | ✓ resolved — was `_hasRing`, renamed for consistency with the array's other boolean-flag slots once confirmed it's a flag (set to 1 as a quest reward in `offer`, never incremented) not a count |
| 1 | `0xA1` | WAND | `_wands` | ✓ consistent |
| 2 | `0xA2` | STAFF | `_staves` | ✓ consistent |
| 3 | `0xA3` | BOOTS | `_bootsOwned` | ✓ consistent |
| 4 | `0xA4` | CLOAK | `_cloakOwned` | ✓ consistent |
| 5 | `0xA5` | HELM | `_helmsOwned` | ✓ resolved — was `_gems`, corrected once `view`'s IDB gap was fixed and the real "VIEW WITH MAGICAL HELM!" text came back |
| 6 | `0xA6` | GEM | `_gemOwned` | added — no independent reference anywhere, named from array position only |
| 7 | `0xA7` | ANKH | `_ankhOwned` | ✓ consistent (gates Rocket boarding) |
| 8 | `0xA8` | RED GEM | `_redGemOwned` | added — same as `_gemOwned`, array position only |
| 9 | `0xA9` | SKULL KEY | `_planeAllowed` | kept functional (Paul's call) — structurally = Skull Key owned |
| 10 | `0xAA` | GREEN GEM | `_greenGemOwned` | added — same as `_gemOwned`, array position only |
| 11 | `0xAB` | BRASS BUTTON | `_brassButtonOwned` | traced and added — wasn't even a recognized struct member before (raw `player+0ABh`); gates launching the Plane, `"FUNNY THIS PLANE IS MISSING A BRASS BUTTON!"` if zero (asm ~7038-7043) |
| 12 | `0xAC` | BLUE TASSLE | `_frigateAllowed` | kept functional (Paul's call) — structurally = Blue Tassle owned |
| 13 | `0xAD` | STRANGE COIN | `_strangeCoin` | traced — spent by the `negate_time` command, `"YOU RUB A COIN..."` (asm ~7245-7268) |
| 14 | `0xAE` | GREEN IDOL | `_idolOwned` | ✓ consistent (resists the sleep trap) |
| 15 | `0xAF` | TRI-LITHIUM | `_triLithium` | traced — Rocket *and* hyperwarp fuel (asm ~6717-6939, ~12790-13575) |

**All 16 slots now named.** A bonus find while tracing the last 3: a
barkeep "buy a rumor" hint table (`unk_116BC`, asm ~2899-2909, `TIP HOW
MUCH?`/`THE BARKEEP SAYS:`) independently confirms nearly every
item-requirement mapping in this whole arc — `"SOME FIGHTERS WEAR
MAGIC HELMS!"`, `"AVIATORS USE SKULL KEYS!"`, `"SAYLORS WEAR BLUE
TASSLES!"`, `"MAGES CARRY WANDS OR STAFFS!"`, `"GUARDS CARRY KEYS!"`,
`"ANKHS OPEN SPACE!"`, `"PLANES NEED BRASS BUTTONS!"` — exact matches
to the code-derived findings above.

Two nice thematic unifications this explains: boarding vs. launching a
Plane turn out to need **two different items** (Skull Key to board,
Brass Button to actually launch it) — a real two-step requirement, not
a naming coincidence. And `_ankhOwned`/`_idolOwned` (already confirmed
independently, positions 7/14) match their array slots exactly, which
is strong corroborating evidence the whole array-position theory is
sound, not just true for the 2 new confirmations.

**The one real conflict: `_gems` (`0xA5`, array position 5 = HELM, not
GEM).** This directly contradicts how `_gems` was named — `view`
requiring it nonzero and (per the original trace) decrementing it,
attributed to "spending a gem to see the map." Re-reading `view`'s code
to resolve this surfaced why the conflict likely exists:
**`view` has the same unfixed IDB gap** the Ship/Frigate and Airplane
paths in `board` had (`aView db 'VIEW'` with no null terminator,
falling straight into undissasembled garbage bytes) — a **third
instance** of the pattern documented at the top of this file. The
original "`_gems` decremented by `view`" claim was extracted from that
same garbled region, so it should be treated as unverified until the
gap is fixed, the same way the Ship/Airplane "requirements" turned out
to be wrong once *that* gap was fixed. Recommend: fix `view`'s inline-data
gap the same way, re-read what's actually there, and reconcile — real
GEM is very plausibly `0xA6` (currently unclaimed) rather than `0xA5`.
Not renaming `_gems` in this pass — that would be undoing an
already-applied rename on unverified grounds, worth Paul's explicit
call once the gap is fixed and the real behavior is visible.

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
2. Requires `player._wands + player._staves != 0` — i.e. owning at
   least one of two items — else `"NEED WAND OR STAFF!"` and abort
   (matching `TEXT_STRINGS` indices 47/48, WAND/STAFF).
3. Requires `_readiedSpell != 0` (a spell is actually selected) —
   silent abort otherwise, no message.
4. Requires `player.field_80[_readiedSpell] != 0` — a **per-spell
   charge count**, one byte per spell 1-9 (another struct-of-arrays
   member inside `Savegame`, same shape as the armor/weapon-owned
   arrays noted below) — else `"NO SPELL!"`.
5. **Consumes a charge**: `_spellCharges[_readiedSpell]` decremented by
   1 (BCD), unconditionally, before checking whether the cast can even
   succeed — so a failed cast still costs a charge.
6. Requires `player._mapNum2 >= 4` — i.e. Tower (4) or Dungeon (5)
   only, see the `_mapNum2` value table above — else `"-FAILED!"`.
7. Dispatches on `_readiedSpell`'s value to 9 distinct effects, all
   confirmed by reading each branch's actual implementation:

   | `_readiedSpell` | Spell | Effect |
   |---|---|---|
   | 1 | Light | Sets `byte_17436 = 150` — identical mechanism to `ignite_torch` (asm ~9648), same light-duration timer. |
   | 2 / 3 | Down Ladder / Up Ladder | **Tower vs. Dungeon, not "in vs. out of a dungeon"** — see the full trace below. Given the gate above, `_mapNum2` is always 4 or 5 by the time these run; the branch tests `_mapNum2==4` specifically (Tower) against the implicit "else" (Dungeon), not a dungeon/non-dungeon split as earlier notes here assumed. |
   | 4 | Passwall | Computes a position local to a 16-cell dungeon block; requires `loc_11451` to find a wall there (high bit set — `0x80`+ per the dungeon tile encoding in `docs/file-formats.md`) — you can only Passwall an actual wall; on success, writes `0` (floor) into the map there. |
   | 5 | Surface | Restores the player's overworld `_mapX/_mapY`, clears loaded monster data, sets `_mapNum2 = 0`, reloads the map — returns from a dungeon to the surface. **Shares its entire implementation with the `klimb` command** (asm 9720, xref'd from both `cast` and `klimb+37`) — "cast Surface" and "climb out" are the same code. |
   | 6 | Prayer | Random chance of failure (`rand_byte`, high bit = fail); on success, finds a targeted monster (`sub_1330C`) and — **if one's found — instantly kills it**, jumping straight into `attack`'s own monster-death cleanup (`loc_12ED1`, see below). Effectively "smite," not a defensive/healing prayer. |
   | 7 | Magic Missile | Finds a targeted monster via `sub_1330C` (position + type match, not `find_target_monster` — a different monster-lookup helper), then damages its magic-HP pool (`_mapMonsters+0x40`, see `docs/file-formats.md`) by a formula based on `player._experience`. |
   | 8 | Blink | Random short-range teleport within a 16-cell block; requires `loc_11451` to confirm the random destination is floor (`0x00`) before teleporting there. |
   | 9 | Kill | Same monster-finding step as Prayer (`sub_1330C`), then **unconditionally** takes the same instant-kill path — Kill is Prayer's smite effect without the random-failure roll. Resolves last session's open question about why the two share a tail: they're not *sharing behavior* with each other so much as both routing into the same generic "a monster just died" cleanup that `attack` also uses when melee HP hits zero — see `loc_12ED1` below. |

   ### Down Ladder / Up Ladder — Tower vs. Dungeon, fully traced

   Superseded an earlier, wrong assumption in this doc (that these two
   branches distinguished "in a dungeon" from "outside one," searching
   for a nearby entrance in the latter case). That can't happen: the
   gate above (`_mapNum2>=4`) already guarantees `_mapNum2` is 4
   (Tower) or 5 (Dungeon) by the time either branch runs — there's no
   "not in a dungeon-like map" case left to handle. The real
   distinction the `_mapNum2==4` check makes is **Tower vs. Dungeon**,
   and it matters because the two map types number their levels in
   opposite directions (`docs/file-formats.md`: "level 0 = top for
   dungeons, bottom for towers, depth increases with level number").

   Both spells route into one of two shared primitives depending on
   map type and which direction that spell needs to move:
   - **Increment** `byte_17435` (go deeper: level+1, capped at 15) —
     Down Ladder in a Dungeon, Up Ladder in a Tower.
   - **Decrement** `byte_17435` (go toward the entrance: level-1) — Up
     Ladder in a Dungeon, Down Ladder in a Tower. If already at level
     0, this branch instead **returns to the surface** (jumps to the
     same code `cast` Surface and `klimb` use) rather than failing —
     level 0 is the exit for both map types under this direction.

   Either way, `loc_11451` gates the move: reads the destination
   level's tile at the player's current (Y,X); the move only commits
   (and prints `"TO LEVEL <N>"`) if that cell is floor (`0x00`),
   otherwise `"-FAILED!"` and the level doesn't change (the spell
   charge is still consumed either way, per step 5 above).

   ### `loc_11451` — the shared dungeon-cell accessor

   Fully traced. It's not a standalone function — it's a **second entry
   point into the already-named `map_get_monster_at?` proc** (asm
   3283-3309), landing one instruction past that proc's own header and
   skipping only `mov al, (_mapMonsters+80h)[di]`. Given `al = level`
   (a dungeon depth 0-15) and `_playerX`/`_playerY` (a cell position,
   set by the caller beforehand), it computes a pointer
   (`word_17418`) into the loaded dungeon buffer at `(level, Y, X)` —
   high byte = high-byte of `level*256 + map_ptr` (selects the
   256-byte page for that level, matching the dungeon format's 256
   bytes/level from `docs/file-formats.md`), low byte = `Y*16 + X`
   (standard 2D-to-1D index for a 16×16 grid). Returns the raw tile
   byte in AL (flags set via `or al,al`) and leaves `word_17418`
   pointing at that exact byte, so callers can write through it too.

   Confirmed via all 5 real call sites, not just the 3 `cast` branches
   above:
   - Down/Up Ladder, Passwall, Blink (above) — straightforward terrain
     reads, all consistent with the dungeon tile encoding table in
     `docs/file-formats.md` (`0x00` floor, `0x80`+ wall/door types).
   - **`attack`'s dungeon-combat variant** (asm ~6214-6370, taken when
     `player._mapNum2 >= 4`, i.e. actually in a dungeon/tower — a
     separate code path from the overworld `attack` traced two
     sessions ago) calls it *twice*:
     1. To read the current-level cell in front of the player and
        check `al & 7` — nonzero triggers a monster search
        (`[di+137h]/[di+157h]/[di+1B7h]` matched against position +
        level, same fields documented in `docs/file-formats.md`'s
        runtime monster fields), zero means "MISS" immediately. Since
        every documented dungeon tile value is a multiple of `0x10`
        (so `& 7` is always 0 for plain terrain), this reveals **dungeon
        monster presence is also flagged via the low 3 bits of the
        dungeon map's own tile bytes** — a second, independent tracking
        mechanism alongside the `_mapMonsters`-style per-slot fields,
        specific to dungeons.
     2. After a kill, called again at the monster's position to `AND
        al, 0F0h` and write the result back — clearing those
        monster-presence bits while preserving the terrain type in the
        high nibble.

   **`loc_12ED1`** (asm 6307, `attack`'s generic "a monster in slot
   `di` just died" cleanup — zeroes its HP/type/saved-tile fields,
   restores the terrain it was standing on) turns out to be the shared
   destination for Prayer's and Kill's success paths too, resolving the
   open question from last session.

   **`map_get_monster_at?`'s own entry point** (before `loc_11451`,
   used from the monster-AI wander loop in `sub_10A30`) passes a
   monster's display-tile value (`TileId × 4`) as "level" instead of an
   actual dungeon depth — not yet reconciled with the level/page
   interpretation above, since that only makes sense for values 0-15.
   Separate open question, not resolved by this trace.

   ### `sub_15FC3` and `sub_15E83` — the two sound effects, traced

   `sub_15E83` (asm 11610-11636) is `cast`'s **fail buzzer**: checks
   the same mute flag `play_tone_sweep` does (`byte_1795D`), then
   sweeps a *descending* tone directly via the lower-level primitives
   (`sub_15CB2`/`sub_15CD2`/`sub_15CE4`) rather than going through
   `play_tone_sweep` itself. Called from all 4 of `cast`'s failure
   points ("NEED WAND OR STAFF!", "NO SPELL!", both `"-FAILED!"`
   cases) — cast-only, no other callers.

   `sub_15FC3` (asm 11907-11926) is a thin `play_tone_sweep` wrapper,
   structurally identical to `play_hit_sound` but with different fixed
   parameters — a **generic "something magical just happened" chime**,
   not cast-specific. Besides `cast` (played as soon as a cast attempt
   begins, before the map-type gate — so it plays even for attempts
   that go on to fail that check), it's also called from three
   unrelated trap/curse handlers elsewhere in the code: "LEGS
   PARALIZED!", "ARMS PARALIZED!", and "SLEEP SPELL!" (plus a fourth,
   "MAGIC MISSILE!" — a *trap* hitting the player, distinct from the
   player's own Magic Missile spell in `cast`). Same magic chime either
   way, whether it's good news (your own successful cast) or bad
   (something magical just happened to you). All four of those trap
   handlers share a bracketing pattern — `flash_screen` called
   immediately before and after `play_magic_sound` — mirroring how
   `attack` brackets `play_hit_sound` with `xorDrawCircle`. Now fully
   traced, see below.

   ### `flash_screen` — the screen-flash effect, traced and renamed

   Confirmed as a screen-invert flash. `flash_screen` (formerly
   `sub_14B26`, asm 9039-9048) calls a private helper
   `xor_invert_cga_bank` (formerly `sub_14B35`, asm 9054-9070, no
   other callers) twice, once with `ax=0xB800` and once `ax=0xBA00` —
   the two interleaved CGA framebuffer segments (even/odd scanlines).
   `xor_invert_cga_bank` sets `ES` to that segment and XORs every word
   from offset 0 through `0x1900` with `0xFFFF`, inverting that whole
   region of video memory. Being XOR-based, calling it once inverts
   the screen and calling it again restores it — exactly the
   before/after bracketing pattern seen everywhere: flash to inverted
   colors, play/run the paired effect, flash back to normal.

   Reused well beyond the four trap handlers above — also brackets an
   armor-damage-style random encounter in `end_of_turn` (asm
   ~2750-2754, paired with `pause?`) and the "killed Minax" branch in
   `fire` (asm ~5581-5584, paired with `sub_15FA6`, not itself traced)
   — confirming it's a general-purpose "flash the screen for this
   significant event" utility, not tied to any one event type.

   Both renames applied and confirmed in the `.asm`.

   ### `_bootsOwned`/`_cloakOwned`/`_idolOwned` — magical-item protection flags, traced and applied

   All three are owned-item flags for the trap/curse handlers above,
   confirmed via the exact "SAVED BY ..." message each one gates, and
   all three follow the **identical mechanic**: if the flag is zero,
   the trap always hits, no roll. If nonzero, `rand_byte() >= 0x40`
   (192/256 ≈ 75% chance) resists it and prints the save message;
   otherwise the trap still hits despite owning the item. Added as new
   `Savegame` struct members (they weren't recognized at all before —
   raw `player+0xNNh` displacement) via `apply_structs.py`'s
   `"add_member"` op, confirmed in the `.asm`.

   | Field | Item | Traps it resists |
   |---|---|---|
   | `_bootsOwned` (`0xA3`) | Boots (`TEXT_STRINGS` 49) | Leg paralysis (`"LEGS PARALIZED!"` → `"SAVED BY MAGICAL BOOTS!"`) |
   | `_cloakOwned` (`0xA4`) | Cloak (`TEXT_STRINGS` 50) | Arm paralysis (`"ARMS PARALIZED!"` → `"SAVED BY MAGICAL CLOAK"`) |
   | `_idolOwned` (`0xAE`) | Green Idol (`TEXT_STRINGS` 60) | The sleep trap (`"SLEEP SPELL!"` → `"SAVED BY IDOL!"`) |

   The fourth trap in this same family, `"MAGIC MISSILE!"`, has no
   item-protection check at all — it always affects the player.
   All four traps are triggered from a proc the IDB already had named
   `end_of_turn` (asm ~2050 area) — a per-turn random encounter system,
   consistent with the "SAVED BY..." rolls being independent per-trap
   luck checks rather than anything player-controlled.

   ### `board`'s vehicle-boarding requirements, fully traced

   `board` (asm 5961-6161) gates on two things before it even looks at
   what you're standing on:
   1. `player._mapNum2 < 4` — no boarding in a Tower or Dungeon (no
      vehicles underground). Else `"THINK AGAIN <name>"`.
   2. `_playerTileId >= 0x78` — confirms **exactly** the four class
      sprite tiles (`TileId` Fighter/Cleric/Mage/Thief = 60-63, ×2
      encoding = `0x78`/`0x7A`/`0x7C`/`0x7E`, same internal ×2 scheme
      `_mapTileIds`/`draw_map` use, see `docs/file-formats.md`). This
      is the same threshold `enter`'s "-ONLY ON FOOT!" check uses,
      confirming `_playerTileId` doubles as "which sprite currently
      represents the player" — normally one of their own class's 4
      tiles (on foot), temporarily overwritten with a vehicle's tile
      while riding one. Below the threshold = already riding
      something = `"THINK AGAIN <name>"`.

   Then it reads the tile the player is standing on
   (`_tilePlayerCenter`) and matches against exactly the four vehicle
   tiles (again the ×2 encoding — confirmed precisely: `0x22`=Horse,
   `0x24`=Ship, `0x26`=Airplane, `0x28`=Rocket, `TileId` 17-20 ×2).
   Anything else → `" WHAT?"`.

   | Tile | Vehicle | Requirement |
   |---|---|---|
   | `0x22` | Horse | None — always boards. |
   | `0x24` | Ship / Frigate | `player._frigateAllowed`: nonzero boards the upgraded **Frigate**. Zero: `"THE CREW OF THIS SHIP WILL NOT LET YOU BOARD!"` — rejected, no boarding. |
   | `0x26` | Airplane | `player._planeAllowed`: nonzero boards successfully. Zero: `"STRANGE YOU CAN'T GET IN!"` — rejected. |
   | `0x28` | Rocket | `player._ankhOwned` (Ankh, `TEXT_STRINGS` 53) — zero prints `"A METALIC VOICE COMMANDS: YOU MUST HAVE AN ANKH!"` and the turn ends without boarding. |

   **Correction from the initial pass**: the Ship/Airplane failure
   messages were hidden behind an IDB formatting gap (below) when this
   was first traced, and I'd guessed the zero case for Ship was a
   harmless default — "anyone can board a plain ship." Once the gap
   was fixed and the real text came back, that's wrong: **all three
   non-Horse vehicles are gated the same way** — a required condition
   or outright rejection, no free/default path. There's no "board a
   plain Ship" outcome at all in this handler; stepping onto that tile
   either upgrades you to a Frigate or turns you away.

   All four *successes* jump to `end_of_turn2`; every failure jumps to
   plain `end_of_turn`. Traced that distinction too: `end_of_turn2` is
   not a separate function, it's a **label inside `end_of_turn` itself**
   (asm 1346-1373), one instruction past the proc's own start — the
   only thing a successful board skips is a single call to
   `play_tick_sound` (formerly `sub_15C95`), now fully traced (see
   below): it's **just a sound effect**, not turn/food bookkeeping as
   originally guessed here. Vehicle boarding skips the generic "turn
   tick" beep, nothing state-related.

   ### `play_tick_sound` — the "turn tick" sound, traced and renamed

   Called from exactly one place: `end_of_turn`'s own first
   instruction (asm 11163-11181). Same shape as the other sound
   effects already named: checks the mute flag `byte_1795D`, then
   plays a single fixed tone via the shared low-level primitives —
   `sub_15CB2` (programs the PC speaker's timer frequency, `bx=0x500`,
   and switches the speaker on), `sub_15CD2` (holds it for a duration,
   `cx=0x4000`), `sub_15CE4` (silences it, not traced in detail but
   clearly the companion "stop" call to `sub_15CB2`'s "start"). Unlike
   `play_tone_sweep`/`play_hit_sound`/`play_magic_sound`/`play_fail_sound`,
   this one is a flat single-frequency beep, no sweep — a generic
   "turn advances" tick, played once at the start of every full
   `end_of_turn` and skipped whenever a caller jumps straight to
   `end_of_turn2` instead (as `board`'s four vehicle-boarding
   successes do).

   **IDB hygiene gap — fixed.** The zero/failure paths for Ship and
   Airplane fell through `write_string`'s inline-text trick into bytes
   IDA hadn't converted from data back to code (the same pattern
   documented at the top of this file for `write_string`/`access_file`'s
   inline-argument convention) — Paul fixed both directly in IDA,
   confirmed in the refreshed `.asm`.

   `_ankhOwned`, `_frigateAllowed` (`field_AC`), and `_planeAllowed`
   (`field_A9`) are all renamed and applied — no natural item name for
   the latter two the way Ankh/Boots/Cloak/Idol had one (the failure
   messages read as an authorization/access check, "the crew won't let
   you" / "you can't get in," not "you're missing item X"), hence the
   more functional names.

**Also resolves an open item from the `_mapMonsters` decoding pass**:
`_gems` (previously "Fighter-kill drop, +1 BCD, no consumption site
found") is the **gem count**. `view` (asm 10927-10935) requires it
nonzero ("VIEW WHAT?" otherwise) and decrements it on use (asm
10962-10968) — spending a gem to see the world map, matching table
entry 52 (GEM) and the classic Ultima "gem shows the map" item.
Fighters dropping gems is a sensible loot pairing.

**Three per-item "owned"/"charges" flag arrays**, now applied inside
`Savegame` as proper array-typed members (structured like
`_mapMonsters` — one byte per possible item, indexed by a small
0-9-ish choice number — but living inline in the `player` struct
rather than a separate loaded file):
- **`_armorOwned`** (7 bytes, index 0-6) — armor-owned flags, checked
  by `wear_armor` before letting you wear something you don't have
  (asm ~11019). Was `field_60`, a struct member IDA had auto-created
  undersized (1 byte); resized via `ida_scripts/apply_structs.py`'s
  new `"resize_member"` op.
- **`_weaponOwned`** (10 bytes, index 0-9) — weapon-owned counts (a
  real count, not just a flag — `get` increments it by 1 BCD on
  finding a random weapon, asm ~9448-9451; `transact` also increments
  it on buying a weapon from a shopkeeper, "SOLD!" then the same
  increment). Referenced from `ready`, `get`, `steal`, `zstats`, and
  `transact`. Didn't exist as a struct member at all before — added
  via the new `"add_member"` op's `element_size`/`count` support.
- **`_spellCharges`** (10 bytes, index 0-9) — per-spell charge count,
  gates and is decremented by `cast` (see above). Was `field_80`, same
  undersized-then-resized situation as `_armorOwned`.

All four `Savegame` field renames plus these three arrays are applied
and confirmed in the `.asm`. `_weaponOwned` needed a follow-up:
because `add_struc_member` creating a *new* member doesn't
retroactively re-type existing instruction operands the way resizing
an *existing* member's already-typed operands does, its reference
sites still displayed as raw `[di+76h]`. Fixed via
`ida_scripts/fixup_weaponowned_stroff.py` (`op_stroff`), now showing
as `[di+Savegame._weaponOwned]` — a different render style than
`player._armorOwned[di]`/`player._spellCharges[di]` since `op_stroff`
types the offset generically rather than tying it to the specific
`player` global the way those two's carried-over typing does, but
numerically identical and no longer raw hex. The script initially
missed `transact`'s purchase site (its proc list was assembled from
what had been read at the time, not an exhaustive search) — caught
and fixed on a second pass, now covers all 5 procs.

### `sub_XXXXX` sweep — unnamed helpers ranked by call-site reuse

Systematic sweep of the (at the time) 73 unnamed `sub_XXXXX` functions,
ranked by how many static `call`/`jmp` sites reference each (this
codebase tail-jumps into shared code a lot, so `jmp` matters as much as
`call` for reuse counting). Working down the ranked list rather than
picking functions arbitrarily. **Complete: 73 of 73 named, zero
unnamed functions left in the binary.** The single biggest find —
identifying the entire dungeon first-person view rendering pipeline,
including the 10 individual wall-segment helpers — gets its own
writeup below rather than being folded into this list.

**CGA point/line-drawing primitives** (the most-reused unnamed code in
the binary — `draw_line` alone has 44 static call/jmp sites):
- `draw_line` (was `sub_167B3`) — Bresenham line stepper: steps a
  current point (`byte_17889`/`byte_178A`) toward a target
  (`byte_1788B`/`byte_1788C`), calling `plot_point` once per pixel.
- `plot_point` (was `sub_14B76`) — converts a point + viewport origin
  (`word_178CF`/`word_178D1`) to a CGA framebuffer address/2bpp pixel
  mask (same interleaved-bank addressing as `xor_invert_cga_bank`),
  `OR`s the mask in.
- `erase_point` (was `sub_14BAA`) — identical address math, `AND`s an
  *inverted* mask in instead — the erase counterpart.
- `clear_screen`/`clear_cga_bank` (were `sub_14B4E`/`sub_14B5D`) —
  clears both CGA banks; `clear_cga_bank` zeroes one bank's 6400 bytes
  (160 of 200 rows — leaves room for a status/text area).

**PC-speaker primitives**, already understood from the earlier
`play_hit_sound`/`play_tone_sweep`/`play_fail_sound`/`play_tick_sound`
trace (which call these) but never renamed themselves:
`speaker_on`/`hold_tone`/`speaker_off` (were `sub_15CB2`/`sub_15CD2`/
`sub_15CE4`) — start the PIT square wave + enable speaker, reload
frequency + busy-wait hold (called per-step during a sweep), and
restore the saved port state, respectively.

**Hyperwarp starfield animation** — the per-frame rendering for flying
between planets, all sharing the `byte_1788x` point-drawing variables
with the CGA primitives above:
- `animate_starfield` (was `sub_16B3D`) — draws the ship marker, then
  loops all 64 star slots, computing each star's new position relative
  to ship motion (boundary-checked via lookup tables), erasing the old
  point and plotting the new one, respawning any star that scrolls
  off-screen via `next_star_coord`.
- `draw_ship_marker`/`erase_ship_marker` (were `sub_16D83`/`sub_16CC4`)
  — draw/erase a "+"-shaped crosshair at the ship's position (6+6
  `plot_point`/`erase_point` calls, vertical then horizontal stroke).
- `next_star_coord` (was `sub_16D68`) — a lightweight PRNG step
  distinct from the game's main `rand_byte`, only used to generate
  fresh coordinates when respawning an off-screen star.

**`view`'s actual map-overview mechanism** — resolves what `sub_129D2`
does, previously only described as "a pure map-drawing routine" during
the `TEXT_STRINGS` location-descriptor dead-data investigation:
- `draw_world_map_overview` (was `sub_129D2`) — `_circleDeltaX`
  increments 0-63 as the inner loop, `_circleDeltaY` 0-63 as the outer
  loop (both masked `&3Fh`) — a full 64×64 scan of the overworld map,
  **not** a circle despite the variable names (reused scratch globals
  here, unrelated to their name elsewhere). For each cell, reads the
  tile via `get_player_tile` and, for several tile-type categories,
  draws a small 2-4-point glyph icon via `plot_map_icon_point` instead
  of the real tile graphic — a simplified overview, since a real
  64×64-tile render wouldn't fit legibly on a 320×200 screen. This is
  literally `view`'s `"VIEW WITH MAGICAL HELM!"` effect.
- `plot_map_icon_point` (was `sub_12B65`) — converts a small sub-cell
  offset (0-3) plus the current scan position into screen coordinates
  and plots one point; different offset combinations per tile-type
  case form each glyph's distinct point pattern.

**Small math utilities**: `get_dungeon_tile_at_player` (was
`sub_168C0`) — dungeon/tower tile reader, 16-row stride banked by the
depth level, distinct from `get_player_tile`'s 64-row overworld
stride; `sign_byte`/`abs_byte` (were `sub_150D8`/`sub_150EF`) —
classic `sign()`/`abs()` for a signed byte.

**`transact` helpers**: `read_direction_keypress` (was `sub_12362`) —
"which direction?" prompt (busy-waits for a directional key, sets
`_circleDeltaX`/`_circleDeltaY` to the delta — reused as generic
scratch vars here, same as in `draw_world_map_overview`, unrelated to
their circle-sounding name), parallels the already-named
`read_digit_keypress`. `compute_item_price` (was `sub_120AB`) — shop
item-price calculator, discounted by `bit-length(_intelligence +
_charisma)`, built via repeated BCD addition; all 6 call sites are
inside `transact`'s shop-purchase branches — a "smooth talker gets a
better price" haggling mechanic. `longPauseScreen` (was `sub_1068F`)
— identical to `pauseScreen` but a longer delay constant.

**More hyperwarp/sound pieces**: `seed_star_prng` (was `sub_16D5B`) —
seeds `next_star_coord`'s PRNG state with a fixed constant, so every
hyperwarp launch gets the same starfield sequence. `play_melody` (was
`sub_15D09`) — generic table-driven melody player (duration/frequency
note list, `0xFFFF`=rest). `play_step_tick` (was `sub_15EC7`) —
thin `play_melody` wrapper always playing tune 0, called across
`play_game`'s idle-wait loop, `canMoveToTile`, `normal_movement`'s
walking loop, and `launch`'s cleanup — a generic step/tick sound tied
to animation frames. `play_bump_sound` (was `sub_15FA6`) — one-shot
sweep with fixed params called from `normal_movement`, best guess is
a "can't move there" bump sound but not independently confirmed
(lower confidence than the rest of this sweep). `play_cannon_sound`
(was `sub_15EAC`) — clearest context is `fire`'s "FIRE DIRECT-"
sequence (`_playerTileId==0x24`, Ship) right after the direction is
confirmed. `read_animated_digit_keypress` (was `sub_16F7F`) — same
shape as `read_digit_keypress` but calls `animate_starfield` every
loop iteration, used for hyperwarp's XENO=/YAKO=/ZABO= coordinate
entry. `speaker_off_once` (was `sub_17289`) — guards `speaker_off`
behind an "already silenced" flag.

**Text/UI helpers**: `clear_text_row`/`clear_caption_rows` (were
`sub_153A0`/`sub_1538F`) — clear a 40-column text row (`text_y=23`
and `24` specifically for the caption pair), matching the demo
sequence's caption `text_y` values exactly.

**More small pieces**: `draw_hyperwarp_hud` (was `sub_16FA3`) —
displays fuel (`_triLithium`) plus the XENO=/YAKO=/ZABO= destination
coordinates. `check_hyperwarp_sun_collision` (was `sub_17030`) — "YOU
HIT THE SUN!" if all 3 destination coordinates equal 4.
`patch_map_filename` (was `sub_1720B`) — patches the 'X' placeholder
byte of the `MAPXFF`/`MONXFF`/`TLKXFF` filename templates for dynamic
map/monster/talk file selection. `speaker_on_once` (was `sub_17276`)
— mirror of `speaker_off_once`. `play_attack_sound` (was `sub_15FE0`)
— weapon-swing sound, called twice from `attack`, distinct from
`play_hit_sound`'s on-hit-only sound. `check_monster_collision` (was
`sub_12199`) — `canMoveToTile`'s monster-blocks-movement check, and
the direct evidence for the next finding below: entirely bypassed
while Negate Time is active.

**`_flag1` → `_negateTimeDuration`** (plain global, not a `Savegame`
member): confirmed via 3 sites — initialized to 0 in `play_game`
alongside `_flag3`/`_sleepFlag` (paralysis/sleep duration counters),
decremented once per turn in the exact same `end_of_turn` spot as
those two, and set to `0x14` by `negate_time` ("YOU RUB A COIN...",
spends `_strangeCoin`, see the treasure-item block writeup above).
While nonzero, `check_monster_collision` skips its monster-blocking
check entirely — you can walk through monsters while time is negated.

### Ultima II's dungeon first-person view, fully identified

Found while continuing the `sub_XXXXX` sweep — this is what the
original roadmap's "`sub_16xxx` cluster, dense self-contained call
graph, likely one subsystem — combat? dungeon movement?" note was
pointing at. It's neither combat nor movement: it's the classic
first-person wireframe dungeon-corridor renderer (the "maze view"
familiar from early Ultima/Wizardry-style dungeon crawlers), called
from `end_of_turn`. Four pieces, each confirmed by reading the full
body:

- **`render_dungeon_view`** (was `sub_16000`) — top-level step. Always
  computes `_tilePlayerCenter`/`_tilePlayerUp`/`_tilePlayerDown` via
  `get_dungeon_tile_at_player` (needed for movement/combat logic
  regardless of rendering); if `byte_17436` is set, also triggers the
  full corridor render. Reuses `_mapLeft`/`_mapTop` as scratch
  facing-direction deltas here — a different meaning than their more
  common "viewport scroll origin" role elsewhere, the same
  reused-global pattern already seen with `_circleDeltaX`/`Y` in
  `draw_world_map_overview`.
- **`precompute_dungeon_corridor`** (was `sub_16078`) — for each of 8
  steps ahead along the facing direction, reads 3 dungeon tiles via
  `get_dungeon_tile_at_player` and builds 3 parallel 8-entry arrays:
  left-wall type, right-wall type + monster-presence flag (same
  low-3-bits monster flag as `loc_11451`'s dungeon-cell accessor), and
  straight-ahead wall type — each the tile's high nibble.
- **`draw_dungeon_corridor`** (was `sub_16144`) — the actual renderer.
  `clear_screen`, then iterates depth 0-7, checking bit patterns of
  the 3 precomputed wall-type bytes at each depth to dispatch to one
  of the 10 wall-segment-drawing helpers below, stopping early once a
  wall blocks further view (sign bit set on the right-wall byte).
- **`draw_dungeon_monster`** (was `sub_168ED`) — runs after the
  corridor renders: scans the precomputed monster-presence flags up to
  whatever depth the wall-render loop stopped at, and draws a monster
  sprite at that depth if one's present (`draw_dungeon_monster_sprite`/
  `draw_sprite_row`, were `sub_168A3`/`sub_14BE0`).

**The 10 wall-segment helpers, individually pinned** (follow-up
finished, not just scoped): precisely decoded — not guessed — by
cross-referencing `draw_dungeon_corridor`'s bit-check dispatch logic
against the already-documented dungeon tile format
([file-formats.md](file-formats.md#dungeontower-format-mapx-number-ends-4-5):
`0x00` Floor, `0x10` Ladder up, `0x20` Ladder down, `0x30` both,
`0x40` Chest/Tri-Lithium, `0x80` Wall, `0xC0` Door, `0xE0` Secret
door — all multiples of `0x10` in the high nibble, sign bit set =
Wall/Door/SecretDoor, clear = Floor/Ladder/Chest). Every mapping
cross-checked against each function's actual `CODE XREF` caller site,
not just the bit logic alone:

| Position | Case | Function (was) |
|---|---|---|
| Right | Wall/Door/SecretDoor (sign set) + initial depth-0 frame | `draw_corridor_wall_segment` (`sub_16425`) |
| Right | Door specifically (`0xC0`) | `draw_dungeon_door` (`sub_1648D`) |
| Right | Ladder down (`0x20` bit) | `draw_ladder_down` (`sub_16594`) |
| Right | Ladder up (`0x10` bit) | `draw_ladder_up` (`sub_1660C`) |
| Right | Chest (`0x40` bit) | `draw_chest_icon` (`sub_166CB`) |
| (shared) | Called by both ladder functions | `draw_ladder_rail` (`sub_16684`) |
| Left | Wall/Door/SecretDoor | `draw_left_wall_segment` (`sub_16377`) |
| Left | Door specifically | `draw_left_door` (`sub_164E2`) |
| Left | Floor/Ladder/Chest (open) | `draw_left_open` (`sub_16291`) |
| Ahead | Wall/Door/SecretDoor | `draw_ahead_wall_segment` (`sub_163CE`) |
| Ahead | Door specifically | `draw_ahead_door` (`sub_1653F`) |
| Ahead | Floor/Ladder/Chest (open) | `draw_ahead_open` (`sub_16304`) |

Note the left/ahead positions only distinguish "blocked" vs. "open" —
ladders and chests are only rendered as distinct icons on the right
probe position, not on the side/ahead ones.

**With this, the `sub_XXXXX` sweep is fully complete: 73 of 73 named,
zero unnamed functions remaining in the binary.**

Two more pieces of the dungeon view, found right after: `draw_sprite_row`/
`draw_dungeon_monster_sprite` (were `sub_14BE0`/`sub_168A3`) — the
actual monster-sprite draw `draw_dungeon_monster` calls, same
table-driven CGA addressing style as `plot_point` but writing a full
byte mask instead of a 2-bit pixel. `play_trap_sound` (was `sub_15E3B`)
— the "ARGH! A TRAP!" dungeon pit encounter, chance scaling with depth.

### `end_of_turn`'s trap handlers and monster-AI helpers, named

The item-protection flags gating `end_of_turn`'s random trap
encounters (`_bootsOwned`/`_cloakOwned`/`_idolOwned`) were named in an
earlier session, but the trap *handler functions themselves* were
never named — found while continuing the sweep:

- `leg_paralysis_trap`/`arm_paralysis_trap` (were `sub_10E70`/
  `sub_10EC2`) — "LEGS PARALIZED!"/"ARMS PARALIZED!", same
  flash+sound+resist-check shape. A real mechanical distinction: legs
  sets `player_paralyzedFlag`, arms sets `_flag3` (the duration
  counter `attack`'s "PARALIZED!" gate checks) — different
  consequences for movement vs. combat.
- `magic_missile_trap` (was `sub_10F12`) — no protection item, always
  wastes 2 turns.
- `sleep_trap` (was `sub_10F48`) — `_idolOwned`-gated, sets
  `_sleepFlag`. Declared `proc far`, not `proc near`, so it was missed
  by the sweep's initial grep.
- `minax_curse_trap` (was `sub_10F8E`) — "MINAX CRIES: DIE FOOL!", a
  cursed-*tile* encounter (called from `canMoveToTile`, not
  `end_of_turn`), 1 HP damage.
- `random_item_loss_trap` (was `sub_10FCD`) — ~25% chance, picks a
  random treasure-array slot and decrements it if owned. No resist
  mechanic, unlike the paralysis/sleep traps.
- `consume_food` (was `sub_112DB`) — per-turn food consumption;
  starving to 0 kills the player.

And the monster-AI side, also called from `end_of_turn`:
`check_monster_on_level` (was `sub_1143C`) gates the dungeon
monster-tile accessor by depth; `spawn_dungeon_monster` (was
`sub_1147F`) is the wandering-monster generator (finds an inactive
slot, assigns position/type, writes both the monster array and the
map tile); `compute_monster_direction_to_player`/
`compute_monster_direction_scaled`/`compute_monster_delta` (were
`sub_1152E`/`sub_14F11`/`sub_14F36`) are three related but distinct
delta helpers — direction sign at 1x and 4x scale, and the raw signed
delta with no sign taken (likely a distance/range check) — named for
their structural differences since the exact semantic split between
the 1x/4x direction variants wasn't fully traced.

Two more, found nearby: `update_patrol_marker` (was `sub_126F9`) — a
map marker cycling through 4 fixed waypoints every 8 turns,
`_disableSave`-gated, preserving/restoring terrain as it moves.
Functional name only — possibly Minax's overworld movement per Ultima
II lore, not independently confirmed which entity this represents.
`find_cursor_target_monster` (was `sub_1330C`) — `cast`'s
cursor-based monster finder, already referred to this way in earlier
notes but never actually renamed; distinct from the facing-direction-
based `find_target_monster` used by `attack`/`fire`/`offer`/
`transact`. `clear_picked_up_tile` (was `sub_1361C`) — clears a
picked-up item's overworld tile back to ground, called from `get`.

### Space-flight loop and Minax's death, named

Closing out the hyperwarp/space-travel cluster: `space_travel_command_loop`
(was `sub_16A79`) is the main per-turn loop while flying in space,
parallel to `play_game`'s overworld loop — draws the HUD, prompts
"CMD: ", keeps `animate_starfield` running while waiting for a
keypress, dispatches on the key. `init_starfield` (was `sub_16D3E`)
seeds the PRNG and populates all 64 star positions once, entering the
view. `play_star_twinkle_sound` (was `sub_17242`) is the periodic
ambient chime scheduled from inside `animate_starfield`'s countdown.
`setup_rocket_launch_display` (was `sub_16999`) is `launch`'s
post-launch setup (ship marker, destination coords, fuel decrement,
HUD labels).

Unrelated but found in the same pass: `minax_death_sequence` (was
`sub_172A0`) — "MINAX IS DEAD!! ALL HER WORKS SHALL DIE!", called
from `attack`, a victory/destruction animation (repeated
`play_hit_sound` bracketing randomized screen positions).

With these, the `sub_XXXXX` sweep is essentially complete: 63 of the
original 73 named, and the remaining 12 are exactly the dungeon
wall-segment helper cluster already flagged above as its own
follow-up — nothing else is left unaddressed.

### DOS/BIOS interrupt dependencies — the porting boundary, fully cataloged

Every `int` and direct hardware-port instruction in the binary
(2026-08-18) — this is the complete list of what a ScummVM engine shim
will need to replace:

| Interrupt/port | Function | Site(s) | Purpose |
|---|---|---|---|
| `int 10h` AH=0 | `set_cga_mode` | 1 | Set video mode 1 (40×25 16-color text) |
| `int 10h` AH=0 | `setPalette` | 1 | Set video mode 4 (320×200 4-color CGA graphics) |
| `int 10h` AH=0Bh | `setPalette` | 2 | Set border color 0, select palette 1 |
| `int 10h` AH=6 | (inside `write_character`/scroll helper) | 1 | Scroll page up |
| `int 10h` AH=9 | (inside `write_character`) | 1 | Write attributes/character at cursor |
| `int 10h` AH=2 | `set_cursor_position` | 1 | Set text-mode cursor position |
| `int 16h` AH=1/0 | `keypress_check` | 4 (2 check+read pairs) | Poll/read a keypress from the BIOS keyboard buffer |
| `int 21h` AH=0Fh | `access_file` | 1 | Open (FCB) |
| `int 21h` AH=1Ah | `access_file` | 1 | Set Disk Transfer Area address |
| `int 21h` AH=`_picture_int21_function` | `access_file` | 1 | Read/write record (function selected by a variable, not a fixed literal) |
| `int 21h` AH=10h | `access_file` | 1 | Close (FCB) |
| `out 43h`/`out 42h` | `speaker_on`/`hold_tone` | — | PIT (8253) timer channel 2 — tone frequency |
| `out 61h`/`in 61h` | `speaker_on`/`speaker_off` | — | PC/XT PPI port B — speaker gate enable/disable |

CGA framebuffer access itself (`plot_point`/`erase_point`/`draw_line`/
`draw_tile`/etc.) is **not** interrupt-based at all — it's direct
writes to the `0xB800`/`0xBA00` interleaved video memory segments,
already extensively documented throughout this file. The `int 10h`
calls above only cover mode-setting, palette, cursor, and text-mode
scroll/write — the actual pixel/tile drawing bypasses BIOS entirely.

### The tile-rendering/animation subsystem, found and documented

Found while chasing the interrupt inventory above: `set_cga_mode` and
`setPalette` turned out to be **collapsed functions** in the IDB (IDA's
folding feature) — the `.asm` export was showing a
`[NN BYTES: COLLAPSED FUNCTION ...]` placeholder instead of their real
instructions, which is why they hadn't been readable via the exported
`.asm` before. Expanding them (`ida_scripts/uncollapse_functions.py`,
using the per-function `ida_funcs.FUNC_HIDDEN` flag — a different
mechanism than IDA's generic "hidden range" feature, which reported 0
ranges despite 7 visibly-collapsed functions) revealed 5 more
collapsed functions right alongside them, none previously documented
in this project despite already having clear names from an earlier
session:

- **`draw_tile`** — blits one 16-row tile bitmap onto the screen,
  copying 4 bytes/row from the tile's graphics data (`si`) into the
  CGA framebuffer via a `screen_rows[]` lookup table (segment per
  scanline, matching the same interleaved-bank addressing as
  `plot_point`).
- **`draw_map_content`** — the actual redraw loop `draw_map` calls:
  iterates a 20×10 grid of 16×16 cells (320×160 px), reading each
  cell's cached value from `_mapTileIds[]`. **This resolves the
  `draw_map` bit-shift question left open earlier as unreconciled**:
  `_mapTileIds[]`'s low 7 bits hold the tile ID **× 2** (not the raw
  0-63 ID) — which is exactly the correct byte-offset scaling to index
  directly into `TILE_OFFSETS[]`, a *word*-sized table, with no further
  multiply needed at draw time. Bit 7 is a separate flag: if set, the
  cell already matches what's on screen (set by `draw_map`'s
  `_priorMapTileIds[]` comparison) and drawing is skipped entirely — a
  dirty-rectangle redraw optimization, confirmed rather than guessed.
  `setPalette` initializes both `_mapTileIds[]` and
  `_priorMapTileIds[]` to `0xFFFF`, forcing a full redraw the first
  time `draw_map` runs after a mode/palette change.
- **`animate_water`/`animate_forcefield`/`animate_tile`** — a simple
  animated-tile mechanism: `animate_tile` does a circular rotation of
  15 animation-frame pointers (shifts frames 1-14 back by one slot,
  wraps frame 0 to the end) within a tile's `TILE_OFFSETS` record —
  the classic "cycle through N pre-drawn bitmaps" technique for water/
  forcefield animation. `animate_water` operates on `TILE_OFFSETS`
  itself (tile 0); `animate_forcefield` on `TILE_OFFSETS+0x2Eh`. Called
  from `play_game`'s main loop.

### The game's actual ending, found while building the string catalog

Building a full `write_string` text catalog
([game-text-catalog.txt](game-text-catalog.txt), 333 entries — see
`docs/roadmap.md` for the full story of how a mechanical "dump every
comment" script turned into several real findings) turned up one
genuinely broken site among 332: `0x1734F`, never fixed before,
because it's reached by *ordinary fallthrough* rather than any
recognized function's `CALL` — the general `fix_inline_strings.py`
sweep only ever finds sites via `XrefsTo`, so a call reached this way
was invisible to it no matter how many times it re-ran.

It's a direct continuation of `minax_death_sequence`'s function chunk
— the actual game-ending sequence, entirely undocumented until now:

1. `minax_death_sequence` plays out (the "MINAX IS DEAD!!" animation).
2. `"YOU FEEL A STRANGE FORCE!"`
3. Loads a special final map via `access_file` — `MAPX30` (**level
   30**, not a normal numbered map/dungeon file).
4. `"YOU HAVE SAVED THE UNIVERSE,\` `AND COMPLETED ULTIMA ][! SEEK\`
   `NOW TO CONQUER WICKED EXODUS,"` — the true victory message.
5. Falls straight into a second orphaned `write_string` call
   (`0x173AE`, same fallthrough pattern) printing `"FOUND IN ULTIMA
   ]I[-D ]II[-P!"` — an Easter-egg teaser for future games in the
   series (`]I[` = Ultima III, `]II[` = Ultima IV), styled with the
   same bracket convention as this game's own "ULTIMA ][" branding.

Fixed via `ida_scripts/fix_ending_strings.py` (identical technique to
`fix_inline_strings.py`, scoped to these 2 specific sites). Both
`play_bump_sound` calls bracketing this sequence are worth revisiting
— see the open item in `docs/roadmap.md`, since their use here weakens
that name's original "movement blocked" justification.
