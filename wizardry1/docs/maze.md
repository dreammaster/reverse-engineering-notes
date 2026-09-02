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

## RUNMAIN (`P010E0E`) — the turn loop

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
| SCNMSG | `DOSCNMSG` | `XGOTO := XSCNMSG` (`SPECIALS` shows a scripted message) |
| ENCOUNTE | `CHENCOUN` | fixed fight from `AUX2` (+ `rand mod AUX1`), `AUX0` = remaining count |

`QUIETXFR` only changes level (via `EXITRUN` → `XNEWMAZE`) when
`MAZELEV <> AUX0[SQTYPE]`.

## The 3D view — `DRAWMAZE` (`P010E02`)  *(not yet ported)*

A wireframe drawn with the interp's `DRAWLINE(x, y, dH, dV, len)` primitive
into an 82×79 picture area.  `LIGHTDIS` squares deep (2 dark / 3–5 with
`LIGHT`, halved for `QUICKPLT`); each step halves `WALWIDTH` (32→16→8…) and
insets `UL/LR` toward the vanishing point.  Per depth it draws
`LEFTVIEW`/`RIGHVIEW` side panels (`DRAWLEFT`/`DRAWRIGH`, with door cut-outs
that reveal `HIDEDOOR` only on `RANDOM mod 6 = 3` without light) and, when a
side is open, the perpendicular face one square over (`FRWDVIEW(±1)` →
`DRAWFRNT`).  Stops at the first non-open `FRWDVIEW(0)` or a `DARK` square.

Data + movement + a top-down debug view are done (`wiz1 maze
<SCENARIO.DATA> [level] [F/L/R/K/Q script]`).  Porting `DRAWMAZE` +
`DRAWLINE` to a `Surface` is the next step.
