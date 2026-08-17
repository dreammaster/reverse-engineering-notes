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
field_33        db ?   ; unnamed, xref'd
field_34        db ?   ; unnamed, xref'd
; ... struct continues to offset 0x100, largely unmapped past here
```

`_disableSave` is also referenced (used by `save_game`, asm ~6692) but
not yet located within the struct layout above — worth pinning down.

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
