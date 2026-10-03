# MM3 data files

Tools: `tools/mm3_cc.py` (list / extract `.CC` archives incl. LZHUF decoding and filename recovery from the EXE),
`tools/mm3_monsters.py`.  Container format, cipher, hash: see `mm3-re.md`.  The LZHUF decoder in `mm3_cc.py` decodes all
558 members of `MM3.CC` (321 get their real names back from the strings table in the EXE).

## MM3.CUR and `*.mm3` save games

`MM3.CUR` (207,551 bytes) is itself a `.CC` archive of 240 members, **all stored uncompressed**:

| Member | Size | Content |
|---|---|---|
| `MAZE.NAM` | 31 | save-game name (`"Default Characters"`), copied to DGROUP `E8CAh` |
| `MAZE.CHR` | 9,090 | the roster: 30 characters x 303 (`12Fh`) bytes, loaded to a far buffer (`Roster_buffer`) |
| `MAZE.PTY` | 918 (`396h`) | party state block, loaded to DGROUP `E8EAh` (`Party_state`) |
| `MAZEnn.DAT` | 832 | per-maze state, nn = 01-50 (+ a few others) |
| `MAZEnn.BIN` | 138-513 | per-maze data, 64 of them |
| `MAZEnn.EVT` | | per-maze event state, 63 of them |

`loadSavedGame` (`26447`) copies the three headline members into DGROUP; `readSaveHeader` (`264ED`) reads only
name + party + the highest level of the characters in the party (for the load menu).

### `MAZE.PTY` (offsets into the block; each is also a DGROUP variable)

| Offset | Field |
|---|---|
| `000h` | party size (`Party_count`) |
| `001h`.. | roster indexes of the party members (`ff` = empty); `+0Ah` facing, `+0Bh` x, `+0Ch` y, `+0Dh` map id |
| `12h` | levitate (spell counter) |
| `14h` | wizard eye |
| `15h` | walk on water |
| `34Bh` | day (0-99) |
| `34Eh` | light (word), then `350h` fire / `352h` electricity / `354h` cold / `356h` poison resistance (words) |
| `34Ch` | year (word); new games start at 500 |
| `358h` | minutes into the day (word, `1E0h` = 8:00) |
| `35Ah` | food (word) |
| `362h` | bank gold (dword) |
| `366h` | bank gems (dword) |
| `36Ah` | gold (dword) (by use) |
| `36Eh` | gems (dword) (by use) |
| `376h` | 32-byte game-flag bit array (`isBitSet`/`setBit`, event mode 20) |

Party time is the same system as Xeen: 1440 minutes a day, 100 days a year, a condition tick every 480 minutes
(`changeTime`, `addTime`).

### Character record (303 bytes)

See `overview.md`; the party's active copies live at DGROUP `B9D6h`, stride `12Fh`.

## Monster tables (`MON*.DAT` members of `MM3.CC`)

Monsters are stored as one file per field, each a column array indexed by monster id (90 monsters).  `loadMonsterData`
(`26716`) loads all 22 into far pointers (`Mon_hp` ... `Mon_phys`, DGROUP `37722`-`37772`):

| File | Type | | File | Type |
|---|---|---|---|---|
| `MONHP` | u16 | | `MONGOLD` | u32 |
| `MONAC` | u8 | | `MONGEMS` | u16 |
| `MONSPD` | u8 | | `MONMAGI` | u8 magic resistance |
| `MONDMGN` | u8 dice count | | `MONFIRE`/`MONELEC`/`MONCOLD`/`MONACID`/`MONENER`/`MONPHYS` | u8 resistances |
| `MONDMGS` | u8 dice sides | | `MONTREA` | u8 treasure class |
| `MONNUMA` | u8 attacks | | `MONRANG` | u8 ranged attack |
| `MONEXP` | u32 | | `MONHITB` | u8 to-hit bonus |
| `MONDMGT` | u8 damage type | | `MONATTP` | u8 attack type |
| `MONSPEC` | u8 special attack | | | |

(Field meanings other than the obvious ones are inferred from the names and values; `python tools/mm3_monsters.py DIR`
prints the table.)

## Where the maps are

`MM3.CC` does **not** contain the dungeon data.  The world lives in `MM3.CUR` (the same `.CC` container, all members
stored): `MAZEnn.DAT` (nn = 01..99), `MAZEnn.BIN` (64) and `MAZEnn.EVT` (63) plus the save-state members above; a new game
starts from a copy of it, `*.mm3` saves are modified copies.  (Only `MAZE72.DAT`, `MAZE73.DAT` and `MAZE96.EVT` also exist
in `MM3.CC`.)  `TEXTnn.MAZ` (text blocks per map, 64) and the graphics are in `MM3.CC`.

`Map_load` (`43698`) builds the names `maze%02u.bin`, `maze%02u.evt`, `text%02u.maz`, `%s.pic` and `%s.mon`;
`loadMazeDats` (`263C3`) loads up to four `maze%02u.dat` cell blocks (the current map and its neighbours).

### `MAZEnn.DAT` (832 bytes) -- same idea as Xeen's `MazeData`

| Offset | Size | Content |
|---|---|---|
| `000h` | 512 | 16x16 cells, one `u16` each: four wall nibbles (N/E/S/W as in Xeen's `_wallData`) |
| `200h` | 256 | 16x16 cell flags / surface byte |
| `300h` | 64 | header (below) |

### `MAZEnn.BIN`

Monster list: records of 4 bytes (`x`, `y`, `type<<2 | direction`, ...) ended by `FFh`; the monster's hit points are taken
from `MONHP`; then 5 bytes (picture slots, `2Ah` = no picture) and finally object records of 4 bytes.
(Only read from `Map_load`; the exact field meanings still need confirming.)

### How the engine addresses the maze (from `mazeGetWordRel`, `mazeGetFlagsRel`, `mazeSetBits`)

The map is a 32x32 world made of up to four 16x16 pages (`.DAT` blocks) held in four slots at DGROUP `C554h`, stride
`340h`; `Maze_curSlot` (`34C2E`) is the slot the party is in and `MAZE_SLOT_X/Y` (DGROUP `2600h`/`2604h`, `00 10 00 10` /
`00 00 10 10`) give each slot's origin.  A cell at world (x, y), 0-31:

* x > 15 -> the page east of the current one, y > 15 -> the page to the south; their maze ids are bytes `308h`/`309h`
  of the current page's header, and `mazeNeighbourSlot` maps an id to a loaded slot (`1111h` = not loaded),
* wall word at `slot*340h + (y&15)*32 + (x&15)*2`, flag byte at `slot*340h + 200h + (y&15)*16 + (x&15)`.

## `MAZEnn.BIN` (decoded from `Map_load`, `43698`)

* Monster records, 3 bytes each, until a record starting with `FFh`: `x`, `y`, `b` where `b & 3` picks one of the map's 3
  monster picture ids (`MAP_MONSTER_PICS`, 3 bytes per map) and, if `b >> 2` is non-zero, the monster id is `(b >> 2) + 28h`
  instead.  A monster gets random hit points..., `MONHP[id]` as its maximum.
* Then 5 bytes: the picture slots (`2Ah` = unused) of the objects used on the map (`%s.pic` via a name table at DGROUP `58D4h`).
* Then object records, 3 bytes each (`x`, `y`, picture slot), at most 80 (`50h`).

## `MAZEnn.EVT` -- the event scripts

Same engine as Xeen's `Scripts` class.  The file is a sequence of records:

| Byte | Meaning |
|---|---|
| 0 | length `n` of the rest of the record (the next record starts `n + 1` bytes later) |
| 1, 2 | x, y of the square |
| 3 | facing it triggers on (0-3, 4 = any) |
| 4 | line number: lines of the same (x, y, facing) run in order |
| 5 | opcode |
| 6.. | `n - 5` operand bytes |

`indexEvents` (`3BCF6`) turns this into a 10-byte table (x, y, facing, line, offset) and `runMazeEvent` (`19608`) walks it
whenever the party moves.  The opcode numbering is exactly the one of ScummVM's `Scripts::_cmdList` (opcode 0 does nothing),
operand counts in the shipped data:

| Op | Xeen name | Operand bytes | Op | Xeen name | Operand bytes |
|---|---|---|---|---|---|
| 1 | Display1 (message) | 1 (text index) | 17 | DoTownEvent | 1 |
| 2 | DoorTextSml | 1 | 18 | Exit | 0 |
| 3 | DoorTextLrg | 1 | 19 | AlterMap | 4 |
| 4 | SignText | 1 | 20 | GiveMulti | 6-12 |
| 5 | NPC | 5 | 21 | ConfirmWord | 4 |
| 6 | PlayFX | 1 | 22 | Damage | 3 |
| 7 | Teleport (map, x, y) | 3 | 23 | JumpRnd | 3 |
| 8, 9, 10 | If (width-coded operands) | 3-4 | 24 | AlterEvent | 2 |
| 11 | MoveObj | 3 | 25 | CallEvent / goto | 3 |
| 12 | TakeOrGive | 4-10 | 26 | Return | 0 |
| 14 | Remove | 0 | 27 | SetVar | 2 |
| 15 | SetChar | 1 | 28, 29 | TakeOrGive variants | 4-7 / 6 |
| 16 | Spawn | 4 | 30 | cutscene end | 0 |
| | | | 31 | Teleport (variant) | 3 |
| | | | 32 | WhoWill | 1 |
| | | | 33 | TakeOrGive variant (shares the handler of 12, 28, 29; Xeen's 33 is RndDamage) | 4 |

The names are inferred from the handler shapes (same jump table order and operand counts); the operand encodings of the `If`
family (value widths 1-4 bytes selected by a type byte) are not yet written down.

### `.DAT` header (offsets from `300h`; found from the code that reads it)

| Off | Meaning |
|---|---|
| `00`-`06` | 7 bytes compared with the previous page's when changing slots (environment type) |
| `07` | chance (%) that `run` succeeds in combat (`run`, `4B3D1`) -- e.g. `32h` = 50% |
| `08` | map id of the neighbour page when y > 15 |
| `09` | neighbour when x > 15 |
| `0A` | neighbour when y < 0 |
| `0B` | neighbour when x < 0 |
| `0E` | resting allowed (`rest`: "Too dangerous to rest here!" when 0) |
| `0F` | dismissing a character allowed ("Too dangerous to dismiss here!" when 0) |
| `14` | Teleport allowed, `15` Lloyd's Beacon, `16` Time Distortion, `17` Super Shelter, `18` Town Portal, `19` Nature's Gate, `1A` Etherealize (each tested by that spell's routine) |
| `0C`, `0D`, `10`-`13`, `1B`-`1E` | used by tavern / death / other code (not yet identified); in the shipped files `11h`-`13h` and `1Bh`-`1Eh` are typically `64h` |

### Event operand encodings (verified by decoding every shipped `MAZEnn.EVT` with `tools/mm3_events.py`)

* **Pairs**: `If` (8 `>=`, 9 `==`, 10 `<=`), `TakeOrGive` (12, 28, 33: 2 pairs), `TakeOrGive3` (29) and `GiveMulti` (20: 3 pairs),
  `SetVar` (27: 1 pair) use `(mode, value)` pairs; the value is 4 bytes for modes 16 (experience) and 34 (gold), 2 bytes for 25
  (minutes) and 35 (gems), else 1 byte.  The mode numbers are those of Xeen's `ifProc` (21 item, 44 yes/no prompt, 20 game flag,
  ...), see `ifProc` (`3D32E`).  `If` is followed by one byte: the line to jump to when the test is true.
* `Display1`/`DoorText*`/`SignText`: 1 byte, the index of a NUL-terminated string in `TEXTnn.MAZ` (string 0 first).
  Strings start with control bytes (`03` centre, `0B nnn` ...).
* `Teleport` (7, 31): map, x, y.  `MoveObj`: object, x, y.  `CallEvent` (25): x, y, line.  `SetChar` (15): 0 = all, 1-6 member,
  7 = random, 8 = ask (as Xeen).  `WhoWill` (32): message.  `DoTownEvent` (17): building 0-6.
* `Damage` (22): damage (u16), damage type -> `giveCharDamage`.  `Spawn` (16): monster slot, x, y, (unused) -> resets that monster's
  position/hit points.  `AlterMap` (19): x, y, side (4 = all four), value -> `mazeSetBits`.  `ConfirmWord` (21): word id, line when
  right, text index (message), text index (prompt; 0 = "What's the Password?").  `NPC` (5): a0, name text index, face resource
  args, line.  `AlterEvent` (24): line, new opcode -- rewrites the opcode byte of that line of the event on the party's square.
  `CallEvent` (25): x, y, line (pushes the current position on a call stack, `Return` pops it).  `JumpRnd` (23): max, value, line
  (jumps when `rnd(1,max) == value`, `3 5 .. ` shape; the third operand is the target line).
* One `If` record in MAZE60 carries a stray extra byte; scripts end with a 00 padding byte in some files.

Example (`MAZE01`, pit that needs a rope; text from `TEXT01.MAZ`): `SetChar 0; If item(21)=73 goto 5; Display1 "The drop is
too great. You need a rope to climb down."; If yesno=1 goto 4; Exit; Display1 "A pit descends ... Enter the caverns?"; If yesno=0 goto 8;
Exit; TakeOrGive item 73; SetVar; Teleport 6, 1, 13`.

### Directions and wall nibbles (verified)

* `Party_facing` / the event facing byte: **0 = north (+y), 1 = south (-y), 2 = east (+x), 3 = west (-x)**; 4 = any (events).  The view tables
  `VIEW_DX`/`VIEW_DY` are indexed in that order.  Turning left from 0 goes to 3.
* Wall word of a cell: four nibbles, from the top: sides N, E, S, W (N faces the higher row index of the 16 rows stored in the `.DAT`).  Adjacent
  cells agree (a wall is stored on both sides; 473 of 480 shared edges of `MAZE05` are consistent).  The low 3 bits of a nibble are the wall style,
  bit 3 is a flag.  `tools/mm3_map.py MAZEnn.DAT` prints an ASCII map (flag bytes shown inside the cells).
