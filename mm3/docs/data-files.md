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
