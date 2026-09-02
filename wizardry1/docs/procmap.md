# DOS ↔ Apple procedure map

`docs/procmap.tsv` maps DOS `SYSTEM.PASCAL` procedures to names from the
released Apple Pascal source. `tools/pcode_dis.py` reads it and annotates the
disassembly (`; ---- SEG:proc N = NAME ----` headers, and `CXP/CLP/CGP` call
targets get `; NAME`).

## Segment correspondence

The 1987 DOS rewrite renumbered the p-System segments and split combat into
several new ones; `KANJIREA` (Japanese input) and `GAMEUTIL` are DOS-only.

| Apple segnum | Apple name | DOS segnum | DOS name | Apple procs | DOS procs |
|--:|---|--:|---|--:|--:|
| 1 | WIZARDRY | 1 | WIZARDRY | 60 | 82 |
| 7 | UTILITIE | 13 | UTILITIE | 39 | 32 |
| 8 | SHOPS | 14 | SHOPS | 30 | 42 |
| 9 | SPECIALS | 15 | SPECIALS | 42 | 39 |
| 10 | COMBAT | 7 | COMBAT | — | 8 |
| 11 | CINIT | 10 | CINIT | 9 | 19 |
| 12 | CUTIL | 11 | CUTIL | 40 | 50 |
| 13 | MELEE | — | *(merged into CUTIL / SWINGASW)* | — | — |
| 14 | CASTASPE | 8 | CASTASPE | 29 | 32 |
| 15 | SWINGASW | 9 | SWINGASW | 19 | 22 |
| 16 | CASTLE | 16 | CASTLE | 38 | 42 |
| 17 | ROLLER | 17 | ROLLER | 28 | 36 |
| 18 | CAMP | 18 | CAMP | 34 | 43 |
| 19 | REWARDS | 19 | REWARDS | 39 | 50 |
| 20 | RUNNER | 20 | RUNNER | 38 | 74 |
| — | — | 12 | KANJIREA | — | 50 |
| — | — | 21 | GAMEUTIL | — | 38 |

## How the map is built

`tools/procmatch.py`:

1. **Seeds** (`docs/procmap.seed.tsv`, `conf = manual`) — 35 procedures
   verified by hand from structure: the `WIZARDRY` shared-library core
   (`GETREC`/`GETRECW`/`ADDLONGS`/`SUBLONGS`/`BCD2LONG`/`LONG2BCD`/`MULTLONG`
   = Apple procs 2–9, plus `GETSTR`/`CHECKCACHE`/`FINDFILE`/the retry
   helpers), the `ASCII.KRN` string machinery in `KANJIREA`, and every
   segment's `proc 1` (the `SEGMENT PROCEDURE` body).
2. **Auto-match** — for each remaining proc in a name-matched segment, score
   Apple candidates by `3·(shared string literals) + 2·(matched resolved
   calls)`, propagated from the seeds over ~6 iterations. Only unambiguous
   hits ≥ 3 are written (`conf = auto:N`).

Coverage today: **47 / 659** procedures (35 hand-verified + 12 high-confidence auto). The auto-matcher is limited by
(a) most DOS text coming through `GetStr(key)` rather than inline literals,
and (b) the Apple listing's nested-procedure scoping. Strong auto hits so
far: `PRTRAPTY` (chest-trap names), `EQUIPCHR`/`CURSBELL` (UTILITIE),
`TRADGOLD`/`OLDAGE` (long-math call signature), `CONGRATS`, `DROPLEVL`.

## Extending it

- DOS proc *N* ≈ Apple listing proc *N* holds for the low range of each
  name-matched segment (before the DOS build inserts procs) — a useful
  starting guess, verify case by case.
- Add confirmed rows to `docs/procmap.seed.tsv` (they are never overwritten),
  then re-run `procmatch.py` to re-propagate.
- A global-variable map (Apple `VAR` block → DOS global numbers) would make
  proc bodies directly readable and matching almost mechanical — next step.
