# SYSTEM.PASCAL — p-code codefile & disassembler

The codefile *format* is engine-wide; the segment/proc counts and examples
below are Wizardry I's (`wizardry1/`).  `tools/pcode_dis.py`, `sources/`,
`extracted/` paths are under `wizardry1/`.

`tools/pcode_dis.py` disassembles the game's linked p-code. Dialect and
operand formats: [`pmachine.md`](pmachine.md). Runs clean over all 16
segments / 587 procedures of Wiz I — no decode desyncs, no unknown opcodes.

```
python tools/pcode_dis.py segments extracted/wiz1/SYSTEM.PASCAL
python tools/pcode_dis.py procs    extracted/wiz1/SYSTEM.PASCAL WIZARDRY
python tools/pcode_dis.py dis      extracted/wiz1/SYSTEM.PASCAL WIZARDRY 3
python tools/pcode_dis.py dis-all  extracted/wiz1/SYSTEM.PASCAL extracted/pcode
```

## Codefile structure (little-endian, no byte-swap)

**Block 0 — directory**

| off | contents |
|---|---|
| `0x00` | 16 × (`u16 firstBlock`, `u16 byteLength`) — segment table |
| `0x40` | 16 × 8-byte segment name |

**Segment** — raw p-code from offset 0. At the very end:

| position | contents |
|---|---|
| `segEnd-2` | last word: **lo byte = p-System segment number**, **hi byte = procedure count** |
| below it | `nproc` × `u16` self-relative pointers; proc *p* at `segEnd-2-2p`, `PATanchor = ptrPos - *ptrPos` |

| dict idx | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| name | WIZARDRY | COMBAT | CASTASPE | SWINGASW | CINIT | CUTIL | KANJIREA | UTILITIE | SHOPS | SPECIALS | CASTLE | ROLLER | CAMP | REWARDS | RUNNER | GAMEUTIL |
| **segnum** | **1** | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 |
| procs | 82 | 8 | 32 | 22 | 19 | 50 | 50 | 32 | 42 | 39 | 42 | 36 | 43 | 50 | 74 | 38 |

Segnums 2–6 are absent (RSP / interpreter-resident). `WIZARDRY` (seg 1) is the
resident shared library — every other segment reaches it by `CXP WIZARDRY,n`
and only ever calls itself with `CLP`/`CGP`/`CIP`.

**PAT** (procedure attribute table; `anchor` = the dictionary pointer target)

| offset | `u16` | meaning |
|---|---|---|
| `anchor-8` | data size | local-variable bytes |
| `anchor-6` | param size | parameter bytes (copied from eval stack on entry) |
| `anchor-4` | exit-IC | self-relative, points near the proc's exit code |
| `anchor-2` | entry-IC | self-relative → first bytecode: `entry = (anchor-2) - *(anchor-2)` |
| `anchor` | proc number | sanity check (= *p*) |

Procedure *p*'s bytecode is `[entry, jumpTable)`; a word **jump table** sits
just below `anchor-8`, referenced by backward `UJP`/`FJP` (a negative byte
operand `d` → target word at `anchor + d`, self-relative). Forward jumps are
self-relative from the byte after the operand. `XJP` carries its case table
inline (word-aligned: `i16 min`, `i16 max`, `max-min+1` self-relative words).

## Activation record (`BP`) — from `pm_proc_entry`

```
[BP+0]   caller global base      [BP+8]   caller IPC (return)
[BP+2]   caller BP (dynamic link)[BP+0A]  caller SP
[BP+4]   caller PAT anchor       [BP+0C]  parameters, then locals
[BP+6]   caller proc-dict base
```
Locals/params are addressed `[BP + 0xC + offset]`; `SLDL n` = `[BP+0xC+2(n-1)]`.
Function results are returned by writing local word 1 (`STL 1`) before `RNP`.

## Validation against the Apple source

The Apple listing (`sources/…/Wiz1WizardryPascal.txt`) tags procedures
`P0100NN`. Disassembled DOS `WIZARDRY` procs match by number:

- **proc 2** = `PRINTBEL` — `SLDC 0 / CGP 58 / RNP 0`.
- **proc 3** = `GETREC` — computes
  `BLOFF[type] + 2*(idx DIV RECPER2B[type])` and
  `size*(idx MOD RECPER2B[type])`, compares to `CACHEBL` (global 24), and on a
  miss calls the write-back / read helpers (`CGP 66` / `CGP 65`) with
  `&IOCACHE` (global 1239). Byte-for-byte the Apple algorithm; the two
  `REPEAT … UNTIL IORESULT=0` loops are refactored into the helper procs.
- **proc 4** = `GETRECW` — `CGP 3` (call `GETREC`) then `SLDC 1 / SRO 20`
  (`CACHEWRI := TRUE`, global 20).

This confirms the opcode table, the codefile layout, and the global-variable
addressing all at once. Note the DOS proc numbering drifts from Apple's above
the low teens (procedures inserted/removed in the 1987 rewrite), so the
`P0100NN` map is a **hint above ~proc 15**, not authoritative.

## Procedures identified so far

| proc | name | notes |
|---|---|---|
| WIZARDRY 2 | `PRINTBEL` | `CGP 58` |
| WIZARDRY 3 | `GETREC` | scenario block cache; `CGP 65/66` = read/write-retry helpers |
| WIZARDRY 4 | `GETRECW` | `GETREC` + `CACHEWRI := TRUE` |
| WIZARDRY 38 | `GetStr(KN, VAR s)` | binary search over `strTree` (globals 44 node / 45 root / 46 strOffsets / 47 strTree) |
| WIZARDRY 82 | *(GetStr inner)* | `ASCII.KRN` block fetch + decipher (see `file-formats.md`) |
| WIZARDRY 64 | `CheckCache(head, tail, key)` | LRU-ish node cache; 518-B `d` nodes, 86-B `s` nodes |
| WIZARDRY 17 | dispose / free RAM block | |
| WIZARDRY 59 | open file on unit, return first block | data=2090 |
| KANJIREA 6 | `AllocCaches` | builds the `d`/`s` cache free lists |
| KANJIREA 7 | `AddCache(prev, size)` | |
| KANJIREA 8 | `LoadStrTree(fname)` | opens `ASCII.KRN`, reads header, `LoadRam`s `strOffsets`/`strTree`, sets `SoffSet` (global 26) |
| KANJIREA 9 | `LoadRam(startBlk, byteLen)` | `Allocate` + `UNITREAD` |
| KANJIREA 10 | string-system init | picks `200`/`400` charset+monster prefix, then `LoadStrTree('ASCII.KRN')` |

## Next

- Cross-reference remaining procs to Apple names (call graph + structure).
- Map the tokenised quest-item names (`0x77` join byte).
- Constant-pool / global-variable map per segment; feed into the Phase 3 C++ port.
