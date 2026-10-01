# ROSTER.DAT (the saved game)

`ROSTER.DAT` (8292 bytes) is the only save file: characters *and* game state.

| File offset | Size | Content |
|---|---|---|
| `0000` | `1860h` (6240) | 48 character records of `82h` bytes (24 characters + 24 hireling slots), see file-formats.md.  Loaded to `g_characters` by `load_roster` (`1276C`), written by `save_roster` (`12792`) |
| `1860` | `805h` (2053) | game state, assembled from a table of `(DGROUP offset, length)` pairs at `DGROUP:5309` (terminated by length FFFFh): `state_load` `168B6` copies the block out of the file into those variables, `state_save` `16932` gathers them and writes the block |

The table (state offset = position within the 2052-byte payload; the 2053rd byte is a spare):

| State off | Length | DGROUP | Variable |
|---|---|---|---|
| `0000` | 20 | `03A2` | day of year (1-180) for each of 10 eras (10 words) |
| `0014` | 20 | `03B6` | year for each era (10 words, max 999) |
| `0028` | 16 | `0416` | `g_party_ids` (8 words; FFFF = empty, id >= 18h = hireling) |
| `0038` | 2 | `0426` | `g_party_size` |
| `003A` | 2 | `03CA` | `g_era` (current era) |
| `003C` | 2 | `03CC` | `g_day_fraction` |
| `003E` | 2 | `040E` | word (`word_1DC5E`, not traced) |
| `0040` | 2 | `0410` | battle counter (`word_1DC60`, "Nth battle") |
| `0042` | 2 | `0412` | counter incremented when the party flees (`word_1DC62`) |
| `0044` | 1920 | `968C` | per-map bit arrays: 60 maps x 32 bytes = one bit per 16x16 cell (visited / "already looted" state; consulted by the automap and one-shot events) *(check)* |
| `07C4` | 10 | `03EC` | timers (`1DC3C..45`) |
| `07CE` | 24 | `03F6` | **event variables** 0-23 (`evt_var_addr`, event opcodes 23/26) |
| `07E6` | 4 | `03DC` | four state bytes (`1DC2C..2F`) |
| `07EA` | 1 | `03CE` | `g_view_mode` |
| `07EB..07F6` | 1 each | `03D0..03DB` | party status effects, one byte each: `1DC20`, `1DC21`, `1DC22`..`1DC2B` (`1DC24` is the inn town, not an effect). `1DC25` = **Light**, `1DC26` = **Magic** protection, `1DC27` = **Forces** protection, `1DC28` = **Levitate**, `1DC29` = **Walk on Water**, `1DC2A` = **Guard Dog** (the panel in `147D8` prints these labels with the value) |
| `07F7..0801` | 1 each | `03E0..03EA` | `1DC30..1DC3A`: more effect/flag bytes (`1DC33`..`1DC37` are the to-hit/damage/protection bonuses used in combat: `1DC33` accuracy bonus, `1DC36` damage-halving protection, `1DC37` flat damage bonus) *(partly checked)* |
| `0802` | 1 | `0414` | `byte_1DC64` |
| `0803` | 1 | `0415` | `byte_1DC65` (surprise state carried into combat) |

**Current map and position are not saved**; the game can only be saved at an inn.  `byte_1DC24`
(`g_inn_town`, DGROUP `03D4`, the 5th of the status bytes `03D0..03DB`) holds the id (0-4) of the town whose inn
last saved: `inn_common_helper` and `inn_menu` (1RETINN) set it from `g_map_id` before `save_roster`, and
loading (`1MENU2`, "continue") copies it back into `g_map_id` and re-enters that town
(`2PLAY:B5EA`) at the inn's own entrance.  The same byte is the map the party returns to from event
`FDh` in the blacksmith trap and in `inn_leave`.  So the save only ever records which town you were in.

The original game also keeps a copy of `ROSTER.DAT` on the player disk / in `DEFAULT.DAT` (new-game
template, 780 bytes).

**Where the party stands after loading / leaving the inn**: `inn_menu` (on leaving) copies the position from per-town tables
indexed by the town id: x = `DGROUP:21E8` = {7, 9, 7, 7, 3}, y = `21EE` = {3, 13, 11, 0, 10}, facing = `21F4` = "NNENW"
(towns 0-4 in map order), then calls `set_facing_masks`.  Loading goes through the same exit, so a loaded game always begins at
the inn entrance of `g_inn_town`.
