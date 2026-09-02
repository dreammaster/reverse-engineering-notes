# The Town — CASTLE / SHOPS

The "town" is everything outside the maze: the Castle hub and its four
establishments, plus the Edge of Town. It is driven by the `XGOTO` state
machine in `WIZARDRY` proc 1 (the mainline), which dispatches each state to a
segment; the segment runs its menu loop and sets `XGOTO` again before
returning.

## State machine (`WIZARDRY` proc 1)

DOS `XGOTO` values (from the `XJP 1..29` table in `WIZARDRY` proc 1) — the
enum order matches Apple's `TXGOTO` (`XDONE`=0):

| # | name | segment | meaning |
|--:|---|---|---|
| 0 | `XDONE` | — | exit to "PRESS [RETURN] FOR MORE WIZARDRY" |
| 1 | `XTRAININ` | ROLLER | Training Grounds |
| 2 | `XCASTLE` | CASTLE | Castle hub (Market) |
| 3 | `XGILGAMS` | CASTLE | enter straight into Gilgamesh's Tavern |
| 4 | `XINSPECT` | CAMP | inspect a character |
| 5 | `XBOLTAC` | SHOPS | Boltac's Trading Post |
| 6 | `XCANT` | SHOPS | Temple of Cant |
| 7 | `XRUNNER` | RUNNER | the maze |
| 8 | `XCOMBAT` | COMBAT | |
| 9 | `XNEWMAZE` | UTILITIE | load a maze level |
| 10 | `XCHK4WIN` | SHOPS | check for the amulet / endgame |
| 11 | `XREWARD` | REWARDS | |
| … | | | |
| 17 | `XEDGTOWN` | SHOPS | Edge of Town |

DOS segment numbers: `CASTLE` = 16, `SHOPS` = 14 (Apple has `SHOPS` = 8).
The Apple↔DOS proc-number mapping drifts (the DOS proc dictionary is not in
source order); procs below are cited by Apple name + `P010Ann` id.

## CASTLE (Apple segment 16, `P010A01`)

`CASTLE` proc 1 body: reset combat globals, `TEXTMODE`, `DSPPARTY('')` unless
arriving from Boltac, `XGOTO2 := XGILGAMS`; if `XGOTO = XGILGAMS` run
`GILGAMSH` immediately; then loop: `DSPTITLE('MARKET')`, print the
`P010A26` menu, read one of `A G B C E` (but stay unless `PARTYCNT>0` or key
`E`/`G`), dispatch:

| key | proc | establishment |
|---|---|---|
| `G` | `GILGAMSH` `P010A06` | Gilgamesh's Tavern |
| `A` | `ADVNTINN` `P010A0F` | Adventurer's Inn |
| `C` | `GOTEMPLE` `P010A0E` → `XGOTO:=XCANT` | Temple of Cant (runs in SHOPS) |
| `B` | `GOBOLTAC` `P010A0D` → `XGOTO:=XBOLTAC` | Boltac's (runs in SHOPS) |
| `E` | `EXTCASTL` `P010A2A` → `XGOTO:=XEDGTOWN` | Edge of Town (runs in SHOPS) |

### Shared display

* **`DSPTITLE(s)`** `P010A04` — row 1: `! CASTLE` + `s` right-justified in 30
  + ` !`.
* **`DSPPARTY(title)`** `P010A05` — the party box at the top of the screen:
  ```
  +--------------------------------------+
  ! CASTLE            <title:30>         !
  +----------- CURRENT PARTY: -----------+
   # CHARACTER NAME  CLASS AC HITS STATUS
  <CHARINFO for each of PARTYCNT members, rows 5..>
  +--------------------------------------+
  ```
* **`CHARINFO(x)`** `P010A03` — one party row at `y = 5 + x`:
  `"{x+1:2} {NAME}"`, then at col 19
  `"{ALIGN[0]}-{CLASS[0..2]} {AC:2 or 'LO'}{HPLEFT:5} "` then
  `OK` → `POISON` (if poisoned) / `HPMAX:4`, else the status name.

### Gilgamesh's Tavern — `GILGAMSH` `P010A06`

`GETALIGN`; `DSPTITLE('TAVERN')`; loop: `UNITCLEAR(1)`, `GILGMENU`, read key.
`[RETURN]` leaves. Menu (`GILGMENU` `P010A08`):

* `A)DD A MEMBER` — only if `PARTYCNT < 6`
* `R)EMOVE A MEMBER`, `#) SEE A MEMBER` — only if `PARTYCNT > 0`
* `[RETURN] TO LEAVE`

**`ADDPARTY`** `P010A09`: prompt `WHO WILL JOIN ? >`, `GETLINE`; scan the roster
(`ZCHAR` records) for a live character whose `NAME` matches and `STATUS<>LOST`;
reject with a centered message if: not found (`** WHO? **`), already out
(`INMAZE` or a non-zero lost-location) (`** OUT **`), or alignment clash with
the current party alignment (`** BAD ALIGNMENT **`). Then `ENTER PASSWORD >`
(`GETPASS`) — must equal the character's `PASSWORD`. On success: set
`CHARDISK[PARTYCNT]` = roster index, `INMAZE := TRUE`, write the record back to
the roster (`GETRECW`), `PARTYCNT++`, `GETALIGN`, `CHARINFO(PARTYCNT-1)`.

**`REMOVE`** `P010A0B`: `GETCHARX(FALSE,'WHO WILL LEAVE')` → party index;
clear `INMAZE`, write back to roster; shift the party array down to close the
gap; `PARTYCNT--`; `GETALIGN`; redraw.

**`GETALIGN`** `P010A07`: party alignment = the alignment of the last
non-neutral member, else `NEUTRAL`. (Used to gate `ADDPARTY`.)

**`GETCHARX(dspNames, solicit)`** `WIZARDRY` `P01000F`: optionally lists the
party names at rows 20-21 (two columns), then reads a digit `1..PARTYCNT`;
`[RETURN]` → returns −1. Result is 0-based.

### Adventurer's Inn — `ADVNTINN` `P010A0F`  (ported: `engine/wiz/inn.h`)

`GETWHO` (`GETCHARX(FALSE,'WHO WILL STAY')`) → if `STATUS = OK`, the room menu
loops until `[RETURN]` or the character stops being OK.

| key | `TAKENAP(hpAdd, gpWeek)` | room |
|---|---|---|
| A | 0, 0 | The Stables (free) |
| B | 1, 10 | Cots |
| C | 3, 50 | Economy Rooms |
| D | 7, 200 | Merchant Suites |
| E | 10, 500 | Royal Suites |

**`TAKENAP`** (`P010A23`): if `hpAdd > 0`, `HEALHP` loops
**`while GOLD >= gpWeek and HPLEFT < HPMAX and not KEYAVAIL`** — each week:
`HPLEFT += hpAdd` (cap `HPMAX`), `GOLD -= gpWeek`, **`AGE += 1`** (the DOS
`HEALHP`, CASTLE proc 41, ages a week; the Apple `HEALHP` does not — DOS also
tracks `AGE mod 52 = 1` in a birthday flag). The Stables just prints
`<name> IS NAPPING`. Afterwards, unconditionally: `CHNEWLEV` then `SETSPELS`
— so **resting always refills spell slots and checks for a level, even for
free in the Stables and even while broke**.

**`CHNEWLEV`** (`P010A19`): threshold = `EXP2NEXT[CLASS][CHARLEV]` for level
≤ 12, else `EXP2NEXT[CLASS][12] + (CHARLEV-12)*EXP2NEXT[CLASS][0]`. If
`EXP ≥ threshold` → **`MADELEV`** (one level per stay), else print the
shortfall. `MADELEV`: `CHARLEV++`, bump `MAXLEVAC`; `SETSPELS`; `TRYLEARN`;
`GAINLOST`; recompute `HPMAX` = Σ `MOREHP` over 1..CHARLEV (+1 extra for
Samurai), floored at old `HPMAX + 1`.

* **`MOREHP`** (`P010A1B`): `rand % {Fig/Lord 10, Pri/Sam 8, Thi/Bis/Nin 6,
  Mage 4}` + 1, then vitality mod (`3:-2  4-5:-1  16:+1  17:+2  18:+3`), min 1.
* **`SETSPELS`** (`P010A12`): `MINMAG`/`MINPRI` set each of 7 groups to the
  count of spells known in it; then `SPLPERLV` raises them toward
  `CHARLEV - levelMod` (per-class `levelMod`/`step`: Mage 0/2, Priest 0/2,
  Bishop priest 3/4 + mage 0/4, Lord priest 3/2, Samurai mage 3/3), cap 9.
  Mage spell groups: 1-4, 5-6, 7-8, 9-11, 12-14, 15-18, 19-21.  Priest:
  22-26, 27-30, 31-34, 35-38, 39-44, 45-48, 49-50 (the `SPELLSKN` array is
  effectively 1-indexed 1..50).
* **`TRYLEARN`/`TRY2LRN`** (`P010A1C/D`): for each accessible spell group, for
  each unknown spell, learn it if `rand % 30 < IQ` (mage) / `PIETY` (priest),
  or if no spell in the group is known yet.
* **`GAINLOST`/`OLDAGE`** (`P010A20/22`): per attribute, with prob 3/4, roll
  `rand % 130 < AGE/52` → lose 1 (18 resists 5/6 of the time); else gain 1
  (unless already 18). Vitality dropping to 2 = death of old age (`STATUS :=
  LOST`, `HPLEFT := 0`).

## SHOPS (Apple segment 8 / DOS 14, `P010201`)

`SHOPS` proc 1 dispatches on `XGOTO`: `XCANT`→`CANT`, `XBOLTAC`→`BOLTAC`,
`XEDGTOWN`→`EDGETOWN`, `XCHK4WIN`→`CHK4WIN`, `XCEMETRY`→…

### Edge of Town — `EDGETOWN` `P01021A`

Menu depends on `PARTYCNT`:
* party empty: `T)RAINING GROUNDS`, `C)ASTLE`, `L)EAVE THE GAME`
* party present: also `M)AZE`

* `M` → `ENTMAZE` `P01021B`: "ENTERING {GAMENAME}", set `XGOTO:=XNEWMAZE`,
  `MAZEX=MAZEY=0`, `MAZELEV=-1`, `DIRECTIO=0`, exit.
* `T` → `XGOTO:=XTRAININ` + `UPDCHARS` (write every party member back to the
  roster with `INMAZE:=FALSE`, `PARTYCNT:=0`).
* `L` → `XGOTO:=XDONE` + `UPDCHARS`.
* `C` → `XGOTO:=XCASTLE`.

### Boltac's Trading Post — `BOLTAC` `P01020A`  *(not yet ported)*

Buy / sell / uncurse / identify / pool gold. Item stock and prices from the
`ZOBJECT` records (`BOLTACXX` = shop stock count / −1 = unlimited).

### Temple of Cant — `CANT` `P010202`  *(not yet ported)*

Tithe to cure status: heal paralysis / stone / dead / ashes for a fee scaled
by the patient's level; failed resurrection worsens DEAD→ASHES→LOST.

## Engine port

* `engine/wiz/party.h` — `Party`: up to 6 `Character` copies + `charDisk[6]`
  roster indices + `partyCount`; `add`/`remove` move records between the party
  and the `Roster`; `align()` = `GETALIGN`.
* `engine/wiz/inn.h` — the Adventurer's Inn rules (pure): `setSpells`,
  `moreHp`, `tryLearn`, `gainLost`, `checkNewLevel`/`madeLevel`, `kRooms`.
* `engine/wiz/town_ui.{h,cpp}` — `runTown(...)`: the CASTLE hub, Gilgamesh's
  Tavern (add/remove/see), the Adventurer's Inn (`advntInn`/`takeNap`), and
  the Edge of Town. Boltac / Temple are stubs that return to the hub. Returns
  a `TownExit` telling `main` where to go next (`Roller`, `Maze`, `LeaveGame`,
  `WindowClosed`).
* `wiz1 town <CHARSET> <SCENARIO.DATA> [TITLE] [roster.dat] [party.dat]` —
  SDL; `wiz1 town-test <CHARSET> <SCENARIO.DATA> <keyscript> [dumpdir]` —
  headless, drives one trip through the town; `wiz1 inn-test <SCENARIO.DATA>`
  — deterministic level-up check.
