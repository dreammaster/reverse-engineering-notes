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
| `000h` | party size |
| `001h`.. | roster indexes of the party members (`ff` = empty) |
| `34Bh` | day (0-99) |
| `34Ch` | year (word); new games start at 500 |
| `358h` | minutes into the day (word, `1E0h` = 8:00) |
| `35Ah` | food (word) |
| `362h` | bank gold (dword) |
| `366h` | bank gems (dword) |
| `36Ah` | gold (dword) (by use) |
| `36Eh` | gems (dword) (by use) |

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
| `300h` | 64 | header: map number, neighbour maps, flags, 16 wall types, 16 surface types, ... (not yet decoded) |

### `MAZEnn.BIN`

Monster list: records of 4 bytes (`x`, `y`, `type<<2 | direction`, ...) ended by `FFh`; the monster's hit points are taken
from `MONHP`; then 5 bytes (picture slots, `2Ah` = no picture) and finally object records of 4 bytes.
(Only read from `Map_load`; the exact field meanings still need confirming.)
