# The Maze — RUNNER

`RUNNER` (`P010E01`, DOS segnum 20, 74 procs) is maze navigation + the
first-person wireframe view.  Entered on `XGOTO = XRUNNER`; `XNEWMAZE`
(handled by `UTILITIE`) loads a level first.

## Coordinates

`MAZEX` (east), `MAZEY` (north), `DIRECTIO` (0 N / 1 E / 2 S / 3 W), `MAZELEV`
— all globals.  X and Y wrap mod 20.  You start each descent at `MAZEX=0
MAZEY=0 MAZELEV=-1 DIRECTIO=0` (`ENTMAZE`); `XNEWMAZE` bumps the level.

* **`SHFTPOS(x, y, right, fwd)`** (`P010E03`) — shift `right` squares to the
  party's right and `fwd` ahead, given `DIRECTIO`; wraps mod 20.
* **`MOVEFRWD`** (`P010E1D`) — save x/y/lev, step one cell along `DIRECTIO`.
* **`FORWRD`** (`P010E1F`) / `KICK` (`P010E20`) — `FORWRD` steps only through an
  `OPEN` edge; `KICK` steps through anything that is not a solid `WALL` (so
  doors + hidden doors).  A blocked non-initial move prints `OUCH!` + bell.
* **`DOTURN(lr)`** (`P010E21`) — `DIRECTIO := (DIRECTIO + lr) mod 4`; `lr = 3`
  left, `lr = 1` right.

Ported: `engine/wiz/runner.h` — `MazePos {x,y,dir}`, `shftPos`, `stepForward`,
`turn`, `canWalk`, `canKick`, and `frwdView`/`leftView`/`righView` (the wall a
given square shows toward the front / left / right, for the renderer).

## RUNMAIN (`P010E0E`) — the turn loop  (ported: `engine/wiz/maze_ui.h`)

`runMaze(Ui&, Party&, Scenario&, Rng&, MazeState&) -> MazeExit`.  On load,
`FIGHTS` (`P010116`, in `engine/wiz/maze.h` `FightMap`) seeds the unfought-room
mask (9 random `FIGHTS`-cell seeds + every `ENCOUNTE` square, each flood-filled
through open edges) and `CLROOMFG` clears the room you spawn in.

`RUNINIT` draws the fixed HUD (`F)ORWARD C)AMP S)TATUS` … , `PRSTATS`), then
`REPEAT`:
1. update `LIGHT` / `PROTECT` indicators
2. if `NEEDDRMZ` → `DRAWMAZE`
3. if the current cell is special and it is the first turn on it → `SPECSQAR`
4. random encounter check: `RANDOM mod 99 = 35`, or `CHSTALRM`, or
   `FIGHTMAP[x][y]`, or a kick into a `FIGHTS` square (`RANDOM mod 8 = 3`)
   → `ENCOUNTR` → `XGOTO := XCOMBAT`
5. `UPDATEHP` (poison / regen, `RANDOM mod 4 = 2`)
6. `GETKEY` → `F/W` `FORWRD` · `A/L` turn left · `D/R` turn right · `K` `KICK` ·
   `S` `PRSTATS` · `T` `SETTIME` (delay) · `Q` `QUIKPLOT` toggle · `C` camp
   (`XGOTO := XINSPCT2`) · `I` inspect (`XGOTO := XINSAREA`)

## Special squares — `SPECSQAR` (`P010E10`)

`SQTYPE := SQREXTRA[x][y]`; `CASE SQRETYPE[SQTYPE] OF`:

| type | proc | effect |
|---|---|---|
| STAIRS | `STAIRSYN` | prompt Y/N, then `QUIETXFR` to `AUX0` level, `(AUX2, AUX1)` |
| PIT | `APIT` → `ROCKWATR` | agility check vs `RANDOM mod 25 + level`; damage `AUX0 + Σ(rand mod AUX1 + 1)` ×`AUX2` |
| CHUTE | `ACHUTE` → `QUIETXFR` | silent transfer |
| SPINNER | `SPINDIR` | `DIRECTIO := RANDOM mod 4` |
| DARK | `VERYDARK` | `LIGHT := 0` |
| TRANSFER | `QUIETXFR` | teleport to `(AUX2, AUX1)` on `AUX0` |
| OUCHY | `OUCH` → `ROCKWATR` | same damage as PIT |
| BUTTONZ | `BUTTONS` | pick a letter A..(A+AUX1-AUX2) → go to level `AUX2 + letter` |
| ROCKWATE | — | `MAZELEV := -99; XGOTO := XNEWMAZE` (river — kicks you out) |
| FIZZLE | — | `FIZZLES := 1` (spells fail here) |
| SCNMSG | `DOSCNMSG` → `SPCMISC` | scripted message (ported, see below): `AUX2` = subtype, `AUX1` = message #, `AUX0` = fire counter / payload |
| ENCOUNTE | `CHENCOUN` | fixed fight from `AUX2` (+ `rand mod AUX1`), `AUX0` = remaining count |

`QUIETXFR` only changes level (via `EXITRUN` → `XNEWMAZE`) when
`MAZELEV <> AUX0[SQTYPE]`.

## The 3D view — `DRAWMAZE` (`RUNNER` proc 3)  (ported: `engine/wiz/maze3d.h`)

**The DOS wireframe is character-cell line-art, not pixel lines.**  It is
composed into `WINDOW1` — a **36 × 20 cell grid** (≈ 576 × 160 px, near the
full screen width) — by `DRAWLINE` (`CONUNIT` `UNITWRITE` blk 5, native
`INTERP:0x18C1`): `DRAWLINE(glyph, count, dRow, dCol, row, col)` writes a run
of `count` (0 = to the window edge) identical `CHARSET` glyphs stepping
`(dRow, dCol)`, clipped to `col ∈ [X4DRAW_lo, X4DRAW_hi]`.  The line-art
glyphs are `200.CHARSET` codes 0–24 (see `../../docs/ui.md`).

`RUNNER` proc 3 draws **4 depth layers** (procs 21 / 27 / 33 / 39; 2 shown
unlit, 4 with `LIGHT`).  `proc 12` steps the cursor one square forward
(`SHFTPOS`) and `proc 17` reads `leftView` / `frwdView` / `righView` (procs
13/14/15) + door reveal before each; then per layer:

* left wall solid → proc 22/28/34/40 · left open → proc 23/29/35 ·
  right → 24/30/36/41 · right open → 25/31/37 · front → 26/32/38 (checks
  `frwdView` internally); a `DOOR` adds a door-frame overlay
  (`if wallVal == 2`).
* `FRWDVIEW(0) ≠ OPEN` → draw the front wall and **stop** (`CIP 4`).
* `DARKNESS` stops the draw; a same-level `TRANSFER` jumps the cursor.

**Port status:** `engine/wiz/maze3d.cpp` renders into the 36 × 20 cell grid
with the line-art glyphs and the verified `leftView`/`frwdView`/`righView` +
door logic, but with **approximated receding-trapezoid geometry** (the DOS
per-depth `DRAWLINE` coordinate tables — procs 22–44 — are transcribed in
the source but the exact `DRAWLINE` arg encoding needs a live DOSBox trace
to pin; the current geometry is close, not glyph-exact).  `maze_ui.cpp`
frames it as the DOS RUNNER screen: menu bar `(0,0,40,3)`, the wireframe
window `(1,2,38,22)`, the party strip `(0,10,40,6)` toggled by `S`, a
message strip along the bottom.

`wiz1 maze <SCENARIO.DATA> [level] [F/L/R/K/Q script]` prints the grid as
ASCII after every step (CMake test `maze_3d`); `WIZ1_MAZE_DUMP=<dir>
wiz1 maze-play-test …` dumps the HUD per frame.

## The HUD  (`engine/wiz/maze_ui.cpp`)

640×192 text surface (40×24, 16×8 font).  DOS RUNNER screen: the menu bar
window `(0,0,40,3)`, the framed wireframe window `(1,2,38,22)` (its 36×20
interior is the cell grid drawn by `renderMazeCells`), level/facing set into
its top border and `LIGHT`/`PROTECT`/`QUICK` into its bottom border, and a
message strip along rows 20-23.  `PRSTATS` (party panel: `# NAME  A-CLS  AC
HP STATUS`, sorted alive-first) is toggled by `S` into a `(0,10,40,6)`
window over the lower view.  Movement: `F`/`W`/↑ forward, `K` kick,
`L`/`A`/← left, `R`/`D`/→ right, `S` toggle party, `Q` quick-plot, `C` camp
(below), `I` inspect (below), `Esc` leave.  `SPECSQAR` handles stairs (Y/N → change level; target
level 0 → back to town), chutes, teleporters, spinners, darkness,
pits/damage (`ROCKWATR`: agility check vs `rand%25 + level`, damage
`AUX0 + AUX2·(rand%AUX1 + 1)`), the river (`ROCKWATE` → level 1), buttons,
encounters (→ `runCombat`), `SCNMSG`, and `UPDATEHP` (below).  `T` =
`SETTIME` (`P010E22`): prompts `NEW DELAY (1-5000)` and stores it in
`MazeState::timeDelay` (DOS `TIMEDLAY`, default 2000 — a busy-loop count;
the engine scales it to `≈timeDelay/5` ms for `MazeCtx::pause1`, the
message pause after a death / chute / river).  It rides along in `maze.dat`.

**`UPDATEHP`** (`P010E1C`) runs once per actual move: each conscious member
has a `rand%4 == 2` chance to take `POISNAMT` damage and `HEALPTS` regen
(net) — poison can kill (`… DIED`), regen caps at `HPMAX`.  When `PRSTATS`
then finds nobody `OK` the maze ends at the cemetery
(`MazeExit::PartyWiped`).  `maze-play-test`'s last arg poisons every member
(test `maze_poison`).

Runs from the town: Edge of Town → `M` → `runMaze`.  `wiz1 maze-sdl <CHARSET>
<SCENARIO.DATA> [level]` (SDL) / `wiz1 maze-play-test <CHARSET> <SCENARIO.DATA>
<keyscript> [ASCII.KRN] [level x y dir] [poison]` (headless; tests
`maze_play`, `town_to_maze`, `scnmsg_room`, `maze_poison`).  `wiz1 maze-scan
<SCENARIO.DATA> [ASCII.KRN]` lists every special-square descriptor.

**Save / resume.**  DOS Wizardry has no `PLAYER.DATA` — the save is
`SCENARIO.DATA` in place (see `../../docs/file-formats.md`).  The engine keeps
`MazeState::save/load` (`maze.dat`, magic `WZM2`): closing the window
mid-delve (`MazeExit::WindowClosed`) writes level / `MAZEX,Y` / `DIRECTIO` /
`LIGHT` / `ACMOD2` / `QUICKPLT` / `FIGHTMAP`-less `scnMsgFired`, and the
party resumes at the same square on the next launch (`st.active`).  Leaving
via the stairs / camp / Esc / a wipe ends the delve and deletes `maze.dat`.
`wiz1 game-test` / test `game_resume`.

## SCNMSG — scripted messages  (`SPECIALS` `SPCMISC` / `DOMSG`)

`engine/wiz/specials.h` + `maze_ui.cpp runScnMsg`.  `DOSCNMSG` just stashes
the square's descriptor index and exits to `SPECIALS`; the DOS `SPCMISC`
reads `AUX2` = subtype, `AUX1` = message number, `AUX0` = a counter/payload.

The text lives in **`ASCII.KRN`** (not the Apple build's `SCENARIO.MESGS`
disk file) at **`key = 15000 + 50·msgNo + line`** — recovered from DOS
`SPECIALS` proc 14 (`LDCI 15000; SLDC 50; …; MPI; ADI`).  `DOMSG` (proc 16)
reads lines from that base until `GetStr` answers `**ERR**` (an absent key).
A leading `@` / `^` centres the line, `$` forces it left; all are stripped.
`showScrollText` pages 12 lines at a time (`[RET] FOR MORE`).

Subtypes (`AUX2`), every one that appears in WIZ1 is ported:
* `0` none · `1` plain
* `2` `TRYGET` — give object `AUX0` to the first member who can carry it
* `4` `GETYN` — show the message, then `SEARCH (Y/N)?`; `Y` → if `AUX0 > 0`
  a fight against monster `AUX0` (`attk012 = 0`), else `TRYGET(|AUX0|)`
* `5` `ITM2PASS` — a quest-item gate: silently pass if any member holds
  object `AUX0`, else `BOUNCEBK` (shoved back one square) + the message
* `8` `BCK2SHOP` — show the message, then `MAZELEV := 0` (straight to town)
* `9` `LOOKOUT` — no message; seed `FIGHTMAP` over a `(2·AUX0+1)²` block
  around the party (wrapping), then clear the party's own cell
* `3` `WHOWADE` · `6` align-gate · `10` riddle · `11` fee — **not ported**
  (none occur in WIZ1)

`AUX0` fixups from `SPCMISC`: the counter (kinds 1/4/8: `0` dead, `N>0`
fires `N`× then `NORMAL`, `N<0` persistent) is tracked in
`MazeState::scnMsgFired`; a persistently-encoded `AUX0 ≤ -1000` carries its
real payload at `AUX0 + 1000`.  Tests `scnmsg_text` / `scnmsg_room` (L4's
reward room → Trebor's speech + the **BLUE RIBBON**) / `scnmsg_getyn`
(searching the chicken-cat statue grants object 97) / `scnmsg_gate` (no
quest item → bounced back) / `scnmsg_backshop` (L1's `(9,19)` → straight
to town).

## CAMP  (`CAMP` P010C01 — `engine/wiz/camp_ui.{h,cpp}`)

Maze `C` → `runCamp` → `CampExit {ToMaze, Disbanded, WindowClosed, ToTown}` (in the
DOS RUNNER, `C` sets `XGOTO := XINSPCT2`).

* **`CAMPMEN2`** — the party list (`# NAME  A-CLS  AC  HITS  ±  MAX/STATUS`,
  the `±` from `HEALPTS − POISNAMT`) and `R)EORDER  E)QUIP  D)ISBAND
  #) INSPECT  L)EAVE`.
* **`INSPECT` / `DSPSTATS`** — the full sheet: the six attributes, gold /
  exp / level / age (`AGE div 52`), HP / AC / status (`& POISONED`),
  `DSPSPELS` (the 7 mage + 7 priest slot counts) and `DSPITEMS` (the pack in
  two columns with the `* - ? #` flag from equipped / cursed / identified /
  `CLASSUSE`).  Sub-menu by status: OK gets `E)QUIP D)ROP T)RADE R)EAD
  S)PELL U)SE I)DENT L)EAVE`, otherwise the short form.  Ported: `E)QUIP`
  (below), `R)EAD` (list known spells), `D)ROP` (`DROPITEM` — refuses cursed
  / equipped), `T)RADE` (`DOTRADE` — hand gold and non-cursed/unequipped
  items to another member), `S)PELL` / `U)SE` (`CASTSPEL` / `USEITEM`,
  below).  `I)DENT` (`IDITEM`, `P010108`, dispatched via `IDENTIFY`
  `P010C15`): Bishop-only (`** NOT BISHOP **` otherwise); pick a packed
  item; already-identified is refused; `IDENTIF := rand%100 < 10 + 5·CHARLEV`
  (`SUCCESS!` / `FAILURE`); then `rand%100 < 35 − 3·CHARLEV` backfires —
  the item's true `CURSED` flag bites and a now-cursed item sticks to the
  hand (DOS falls through to the equipment display).
* **`CASTSPEL`** (`P010C06`) — the in-camp (non-combat) spell set.
  `S)PELL` lists the caster's known spells whose pool (mage or priest,
  by spell level) is > 0; `U)SE` an item invokes its `SPELLPWR` spell for
  free (skipping the pool check) and rolls `CHGCHANC` to transform the item
  to `CHANGETO` (a spent scroll → object 0).  Effects: `DIOS`/`DIAL`/`DIALMA`
  heal `Nd8`, `MADI` full heal + un-KO, `MILWA`/`LOMILWA` set `LIGHT`,
  `LATUMOFI` unpoison, `DIALKO` cure sleep/paralysis, `MAPORFIC`
  `PROTECT := 2`, `DI`/`KADORTO` resurrect (`DIKADORT`: `rand%100 <= 4·VIT`
  → OK at 1 / full HP, `VIT−1` or `LOST` at VIT 3; botch worsens the
  status).  `DUMAPIC` (`P01010D`) prints the party's facing + `(MAZEX east,
  MAZEY north, MAZELEV down)` (level 10 → "ENCHANTMENTS PREVENT…").
  `KANDI` (`KANDIFND` `P01010A`) reads a name and reports a fallen roster
  character's `LOSTXYL.LOCATION` ("STILL WITH US!" / "IN THE MORGUE" /
  "UNREACHABLE!" / "IN THE ⟨N/S⟩ ⟨E/W⟩ OF LEVEL n" / "LOST FOREVER!").
  `MALOR` (`P01010E`) reads an `N/S/E/W/U/D` displacement then `TELEPORT`:
  a new level equal to the level count bounces; out of bounds with
  `level > 0` → `ROCK` (whole party `LOST` → cemetery); `level < 0` →
  `VOLCANO` (all `DEAD`); `level = 0` → the castle at the origin, else
  the `MOAT` (`rand%25 > AGI` drowns) — both return to town.  `camp_ui`
  signals these with `CampExit::ToTown` / a level-crossing `ToMaze` (the
  maze reloads the level + fight map).
* **`EQUIP`** (`EQUIPCHR` / `ARMORPOW`, `UTILITIE` procs P010119 / P01011E —
  `engine/wiz/equip.h`).  `DOEQUIP` walks the slot types
  (weapon / armor / shield / helmet / gauntlet / misc) — for each it lists
  the class-usable items of that type and prompts (`[RET]` = none); a
  cursed item of a type is force-equipped.  Then `equipRecalc` (the
  `ARM4CHAR` / whole-party path, also run on entering the maze) rebuilds
  the combat tail: `LUCKSKIL` from level/luck + class + race, the base
  unarmed stats (`deriveStats`), the best `HEALPTS` of anything carried,
  then each equipped item via `ARMORPOW` — `ARMORCL -= ARMORMOD`,
  `HPCALCMD += WEPHITMD`, `SWINGCNT := max(SWINGCNT, XTRASWNG)`, a weapon
  sets `HPDAMRC` (keeping the STR damage add) + `CRITHITM` + `WEPVSTYP`
  slay bits; an item whose `ALIGN` clashes with the wearer is cursed
  (`HPCALCMD −1`, `ARMORCL +1`).  A still-unarmed ninja gets
  `ARMORCL −= level/3 + 2`.  `DOS TOBJREC` equip tail decoded at words
  13–22 (verified: LONG SWORD 1d8 / SHORT SWORD 1d6 / LEATHER `ARMORMOD` 2).
  A single-character `E)QUIP` ends with **`CHSPCPOW`** (`P01011A`): for each
  carried item with `SPECIAL > 0`, "WILL YOU INVOKE THE SPECIAL POWER OF
  YOUR ⟨item⟩ (Y/N)?"; on Y the `CHGCHANC` roll may consume it, then the
  one-shot effect fires — `SPECIAL` 1‑6 / 7‑12 raise / lower an attribute
  (clamped 3‑18), 13/14 age ∓52 wks, 15‑17 become Samurai/Lord/Ninja, 18
  +50000 gold, 19 +50000 exp, 20 `LOST`, 21 full-restore, 22 `HPMAX +1`,
  23 heal the whole party.  The party-wide `E)QUIP` (`ARM4CHAR`) skips it.
* **`REORDER`** (`UTILITIE` proc 27) — type the current slot numbers in the
  new order; a selection sort by that permutation swaps the party members
  (`Party::swapMembers` moves the character copy and roster index together).
* **`DISBAND`** — confirm twice; every member is left as a body in the
  current room (`INMAZE := FALSE`, `LOSTXYL.LOCATION := (x,y,level)`,
  `AGE += 25`), written back to the roster, and the party empties.

`wiz1 camp-test <CHARSET> <SCENARIO.DATA> <keyscript> [ASCII.KRN] [grants]
[wound=idx:hp[:status] | cls=idx:class | learn=idx:spell]` (`grants` =
`m:i[:u],…` gives member `m` object `i`, `u=0` = unidentified; `learn`
teaches one spell and tops up the pools) dumps the final screen +
`camp exit: … | order: …` and a per-member line (tests `camp_sheet` /
`camp_flow` / `camp_disband` / `camp_equip` / `camp_cast` / `camp_dumapic`
/ `camp_kandi` / `camp_malor` / `camp_malor_moat` / `camp_chspcpow` /
`camp_trade` / `camp_use` / `camp_identify`).  Not ported: chevrons/medals.
`StringPool::spellNameKey` was off by one — fixed to `5000 + idx`
(`5001` = `HALITO`).

## `I`nspect — body recovery  (`SPECIALS` `INSPECT` / `EXPLROOM` / `LOOKLOST` / `PICKUP`, P010302-05)

`maze_ui.cpp runInspect`.  `EXPLROOM` flood-fills from `(MAZEX, MAZEY)`
through **OPEN** cell edges only (walls *and* doors stop it) to find the
room's cells (`INMYROOM`).  `LOOKLOST` then scans the roster for characters
with `INMAZE = false`, `LOSTXYL.LOCATION[3] == MAZELEV`, and
`(LOCATION[1], LOCATION[2])` inside the room — up to 5 — and lists them.
`P)ICK UP` adds one to the party (`Party::add` sets `INMAZE := TRUE`),
clears its `LOCATION`, and it can then be carried out and resurrected at the
Temple; `L)EAVE` returns to the maze.  A body whose `LOCATION` is
`(-1,-1,-1)` (the cemetery's `rand%50 < level` roll) is unrecoverable.
`wiz1 pickup-test` / test `pickup`.
