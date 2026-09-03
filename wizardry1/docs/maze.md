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

## The 3D view — `DRAWMAZE` (`P010E02`)  (ported: `engine/wiz/maze3d.h`)

A wireframe drawn with `DRAWLINE(x, y, dH, dV, len)` — a run of `len` points
from `(x,y)` stepping `(dH, dV)` (each ∈ {−1,0,1}) — into an **82 × 79**
picture area, origin top-left.  The DOS build precomputes the shape into 4
fixed depth layers (`RUNNER` procs 21/27/33/39; `DRAWLINE` = `CONUNIT`
`UNITWRITE` subfn 5); the port reproduces the Apple halving loop, which is
equivalent and fully specified in the Pascal.

`LIGHTDIS` squares deep (2 unlit / 3–5 with `LIGHT`, or 3 with `QUICKPLT`; a
lit draw decrements `LIGHT`).  Start geometry `UL=8 LR=72 WALWIDTH=32
DOORWIDT=16 DOORFRAM=8 WALHEIGH=64`; each step halves `WALWIDTH` (→16→8…) and
insets `UL += WALWIDTH`, `LR -= WALWIDTH`.  Per depth, at the drawing cursor
`(X4DRAW, Y4DRAW)` (advanced one square forward each iteration by `SHFTPOS`):

* `CLRPICT(XLOWER, 0, XUPPER, 79)` sets the horizontal clip for this depth's
  `DRAWLINE`s (nearer walls have already narrowed it).
* `LEFTVIEW(0) ≠ OPEN` → `DRAWLEFT` (a receding trapezoid; `XLOWER := UL`);
  else if `FRWDVIEW(−1) ≠ OPEN` → `DRAWFRNT(wt, −2·WW)` (the face one square
  to the left).  `RIGHVIEW`/`FRWDVIEW(+1)` are the mirror.
* `FRWDVIEW(0) ≠ OPEN` → `DRAWFRNT(wt, 0)` and **stop**.
* Door cut-outs are drawn for a real `DOOR`, or a `HIDEDOOR` that is lit or
  passes `RANDOM mod 6 = 3`.
* `DARK` stops the draw; a same-level `TRANSFER` jumps the cursor to
  `(AUX2, AUX1)`.

`wiz1 maze <SCENARIO.DATA> [level] [F/L/R/K/Q script]` prints the wireframe as
ASCII after every step (CMake test `maze_3d`).

## The HUD  (`engine/wiz/maze_ui.cpp`)

640×192 text surface (40×24, 16×8 font) with the wireframe pane blitted over
it (via `Ui::setOverlay`) top-left, the command list at col 13, and `PRSTATS`
(party panel: `# NAME  A-CLS  AC  HP  HPMAX/STATUS`, sorted alive-first) on
rows 18-23.  Movement: `F`/`W`/↑ forward, `K` kick, `L`/`A`/← left, `R`/`D`/→
right, `S` re-list party, `Q` quick-plot, `C` camp (below), `Esc` leave.
`SPECSQAR` handles stairs (Y/N → change level; target level 0 → back to
town), chutes, teleporters, spinners, darkness, pits/damage (`ROCKWATR`:
agility check vs `rand%25 + level`, damage `AUX0 + AUX2·(rand%AUX1 + 1)`),
the river (`ROCKWATE` → level 1), buttons, encounters (→ `runCombat`),
`SCNMSG`, and `UPDATEHP` (below).  Not ported: `SETTIME`, `I` inspect
(`SPECIALS.INSPECT` — look for secret doors).

**`UPDATEHP`** (`P010E1C`) runs once per actual move: each conscious member
has a `rand%4 == 2` chance to take `POISNAMT` damage and `HEALPTS` regen
(net) — poison can kill (`… DIED`), regen caps at `HPMAX`.  When `PRSTATS`
then finds nobody `OK` the maze ends at the cemetery
(`MazeExit::PartyWiped`).  `maze-play-test`'s last arg poisons every member
(test `maze_poison`).

Runs from the town: Edge of Town → `M` → `runMaze`.  `wiz1 maze-sdl <CHARSET>
<SCENARIO.DATA> [level]` (SDL) / `wiz1 maze-play-test <CHARSET> <SCENARIO.DATA>
<keyscript> [ASCII.KRN] [level x y dir]` (headless; tests `maze_play`,
`town_to_maze`, `scnmsg_room`).  `wiz1 maze-scan <SCENARIO.DATA> [ASCII.KRN]`
lists every special-square descriptor (with SCNMSG text) across all levels.

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

Subtypes (`AUX2`): `0` none · `1` plain · `2` `TRYGET` — give object `AUX0`
to the first member who can carry it (ported) · `3` `WHOWADE` · `4` `GETYN`
· `5` item-gate · `6` align-gate · `8` bounce-to-shop · `10` riddle · `11`
fee (message shown, side effect **not ported**).  The `AUX0` fire counter
(kinds 1/4/8: `0` dead, `N>0` fires `N`× then the square goes `NORMAL`,
`N<0` persistent) is tracked in `MazeState::scnMsgFired` since the engine
holds `SCENARIO.DATA` read-only.  Tests `scnmsg_text` / `scnmsg_room`
(stepping into L4's reward room shows Trebor's speech and hands the party
the **BLUE RIBBON**, object 100).

## CAMP  (`CAMP` P010C01 — `engine/wiz/camp_ui.{h,cpp}`)

Maze `C` → `runCamp` → `CampExit {ToMaze, Disbanded, WindowClosed}` (in the
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
  (below), `R)EAD` (list known spell names) and `D)ROP` (`DROPITEM` —
  refuses cursed / equipped).  `T`/`S`/`U`/`I` say "not available yet".
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
* **`REORDER`** (`UTILITIE` proc 27) — type the current slot numbers in the
  new order; a selection sort by that permutation swaps the party members
  (`Party::swapMembers` moves the character copy and roster index together).
* **`DISBAND`** — confirm twice → leave the maze for town.

`wiz1 camp-test <CHARSET> <SCENARIO.DATA> <keyscript> [ASCII.KRN] [grants]`
(`grants` = `m:i,…` gives member `m` object `i`) dumps the final screen +
`camp exit: … | order: …` (tests `camp_sheet` / `camp_flow` / `camp_disband`
/ `camp_equip`).  Not ported: `USEITEM`, `DOTRADE`, `IDENTIFY`, `CASTSPEL`
(the non-combat spell set), `CHSPCPOW` (invoke an item's special power),
chevrons/medals.  `StringPool::spellNameKey` was off by one — fixed to
`5000 + idx` (`5001` = `HALITO`).
