# Global-variable map

DOS `SYSTEM.PASCAL` globals live in one record based at `ds:[600h]`; the
p-code reaches them by word index — `LDO`/`SRO`/`LAO n`, and `SLDO 1..16`.
`tools/globals.py` runs a usage census; `tools/pcode_dis.py` annotates
`LDO`/`SRO`/`LAO`/`SLDO` from `docs/globals.tsv`.

## The +289 rule

The DOS global record is a **reorganised superset** of the Apple one. The
1987 build inserted ~289 words of new state (the `ASCII.KRN` string tree,
the two node caches, kanji state, extra window pointers, more game-state
scalars) but kept the layout **from `CHARACTR` onward byte-identical**:

| structure | Apple word | DOS word | check |
|---|--:|--:|---|
| `CHARACTR` (party, `ARRAY[0..5] OF TCHAR`) | 74 | 363 | +289 |
| `SCNTOC` (`TSCNTOC`) | 698 | 987 | +289; internal arrays `RECPER2B`/`RECPERDK`/`RECSIZE`/`BLOFF` at +21/+29/+37/+45 all match |
| `IOCACHE` (1024-byte block buffer) | 950 | 1239 | +289 |
| `CHARSET` (`64 × TCHRIMAG`) | 1462 | 1751 | +289 |

So **`DOS_word = Apple_word + 289` for `word ≥ 363`**, and `TCHAR` = 104
words / `TSCNTOC` = 252 words / `IOCACHE` = 512 words (same as Apple). This
also fixes the record field layouts for the C++ port for free.

The Apple scalars (`Wiz1WizardryPascal.txt` VAR block, words 3–27:
`PARTYCNT CACHEBL SCNTOCBL LLBASE04 TIMEDLAY CACHEWRI INCHAR XGOTO XGOTO2
ATTK012 FIZZLES CHSTALRM LIGHT ACMOD2 ENSTRENG BASE12 ENEMYINX SAVELEV
SAVEY SAVEX DIRECTIO MAZELEV MAZEY MAZEX ENCB4RUN`) were **renumbered** and
are hand-mapped in `docs/globals.seed.tsv`. Full Apple list:
[`docs/globals.apple.tsv`](globals.apple.tsv).

## Confirmed DOS scalars (`docs/globals.seed.tsv`)

| word | name | how identified |
|--:|---|---|
| 3 | `INCHAR` | most-read; `GETKEY` writes it; consts CR/'0'/'A' |
| 4 | `PARTYCNT` | most-read global (86×); `0..PARTYCNT-1` loop bounds |
| 14 | `CACHE_LRU` | string-cache MRU list head; `STRINIT:=NIL`, `GETSTR` prepends |
| 16 / 17 | `BACKGND` / `WINDOW1` | window records allocated in `STRINIT`; `BACKGND` by-ref 54× |
| 18 / 19 | `XGOTO` / `XGOTO2` | `TXGOTO` state target; written 63× with enum consts 2/7/9/10 |
| 20 | `CACHEWRI` | boolean; `GETREC:=FALSE` / `GETRECW:=TRUE` (Apple word 8) |
| 24 | `CACHEBL` | scenario block in `IOCACHE`; `STRINIT:=-1` (Apple word 4) |
| 26 | `KRNBLOCK` | first volume block of `ASCII.KRN`; set by `LOADSTRTREE` |
| 27 | `SCNTOCBL` | first volume block of `SCENARIO.DATA`; `GETREC` adds it (Apple word 5) |
| 44–47 | `STR_NODE` `STR_ROOT` `STR_OFFSETS` `STR_TREE` | the `GETSTR` range tree |
| 48–51 | `S_CACHE_HEAD/TAIL` `D_CACHE_HEAD/TAIL` | the 86- / 518-byte node caches |
| 57 | `HIRES400` | 400- vs 200-line mode (picks the `200`/`400` file prefix) |
| 59 | `HAS_EXTRAM` | extended-RAM flag |
| 60 | `CONUNIT` | console output unit |
| 195 | `GBUF` | shared scratch string buffer; by-ref from 45 procs, every segment |

## Extending

`python tools/globals.py <SYSTEM.PASCAL> --g <word>` shows every proc that
reads / writes / addresses a global (named via `docs/procmap.tsv`). Match
against the Apple proc's variable references, add rows to
`docs/globals.seed.tsv`, re-run `tools/globals.py gen …`.
