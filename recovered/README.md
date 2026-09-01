# `recovered/` — pseudo-BASIC reconstruction of the compiled modules

Legacy of the Ancients was written in **Microsoft BASIC Compiler 6.0** and
linked against the shared `LEGLIB.EXE` runtime. These files reconstruct the
original source, function by function, from the disassembly under
`../*.asm`. They are a **reading and review aid** — they will not compile,
and they deliberately keep the game's structure (one file per `.EXE`
module, calling a shared `leglib` library) rather than pre-flattening it
for the C++/ScummVM port.

## Confidence markers

Every non-trivial line is tagged:

| tag | meaning |
|---|---|
| *(no tag)* | mechanical translation, high confidence |
| `'?ord` | operator *identity* is known, but the operand **order** for a non-commutative op (`- / \ MOD ^`) is not — `A op B` vs `B op A` |
| `'?op` | the operator itself is still inferred (rare now that the dispatch indices are known) |
| `'??` | the whole expression / branch is a best guess |
| `'CHECK` | needs a DOSBox memory-watch to confirm |

Each reconstructed block ends with `' asm: out.asm:NNNN` pointing at the
source lines.

## The `leglib` value-stack model

Arithmetic in compiled BASIC 6.0 goes through a runtime value stack
(`ds:111C` = stack pointer, 12 bytes per slot). The disassembly is a
stream of `int 3Fh` thunks; this table is how to read them back as infix:

| thunk | leglib | meaning |
|---|---|---|
| `rtm_FF20(ax)` | `B$PUSHI` | push `ax` as an INTEGER |
| `rtm_FF4B(addr)` | push | push the **SINGLE** at `[addr]` (also used right after `B$RND` with `addr` = the pointer `B$RND` returned) |
| `rtm_FF50(addr)` | pop | pop top → `[addr]` |
| `rtm_FF22` | pop→AX | pop top into `ax` (end of expression) |
| `rtm_FF2B` | — | INT → SINGLE coercion |
| `rtm_FF27` | — | SINGLE → INT/LONG coercion (truncates) |
| `rtm_B8(seg,off)` | `B$RND` | **`RND(x)`** — `push ds:24E8 : push ds:24E6 : call B$RND`; `ds:24E6` is the SINGLE `1.0`, so this is `RND(1)` = next random in `[0,1)`. Returns a pointer to the result single (→ fed to `rtm_FF4B`). |
| `rtm_FC` | `B$RANDOMIZE`-ish | seed the RNG |

**Arithmetic ops** — each thunk loads an index into `bx`, then
`call word ptr [bx+0F7Ch]` (leglib `rtm_FF44` body, `seg004:21A69`). The
`[ds:0F7C]` table is runtime-filled and shows as `db 0` in `leglib.asm`,
**but it is present in `LEGLIB.EXE`** at file offset `5632 + 0x0F7C`
(leglib DGROUP:0 == file 5632). Reading it back and disassembling the 8
handlers (`seg004:0x2CA2 / 0x2C9A / 0x2C98 / 0x2E04 / 0x2E8A / 0x2E88 /
0x2BF3 / —`) gives the **actual** table, which is *not* the canonical
BASIC order — there is no `\`, `MOD`, or `^` in it:

| index | op | how the handler works | immediate thunk | stack-stack thunk |
|---|---|---|---|---|
| `0x00` | **`+`** | add | `rtm_FF44` | `rtm_FF42` |
| `0x04` | **`-`** (a−b) | negates b, then add | `sub_21A02` | — |
| `0x08` | **`-` reversed** (b−a) | swap, negate, add | `rtm_FF53` | — |
| `0x0C` | **`*`** | XOR signs, **add exponents** | `rtm_FF4E` | `rtm_FF4C` |
| `0x10` | **`/`** (TOS1÷TOS) | XOR signs, **sub exponents** | `sub_21A4A` | `rtm_FF47` |
| `0x14` | **`/` reversed** (TOS÷TOS1) | swap, then `0x10` | `rtm_FF49` | — |
| `0x18` | **compare** | ordered float compare | — | — |
| `0x1C` | **compare** | via `[ds:0F78]`, munges flags for `jb`/`jnb` | — | `rtm_FF1F` |

So: `FF4C`/`FF4E` = **`*`** (not `/`), `FF47` = **`/`**, `FF49` = **`/`
with operands reversed** (as an immediate op: `TOS / imm`), `FF53` = **`-`
reversed**. Verified against two independent expressions: `rollCreatureStats`
= `INT(RND*18 + 12.6)` (a 12–30 roll — `/` there would be a constant), and
OUT combat damage = `INT(Str*(wp/6 + 0.5)/(2*RND + 1))` (matches Paul's
DOSBox "BLOW OF 6").

**`\`, `MOD`, `^`, and the relational ops** are separate thunks with their
own leglib handlers, *not* entries in this table. `rtm_FF2B`
(-> `seg004:0x3954`) = **`^` with operands reversed** (`TOS ^ TOS1`): it
**pops two operands** (binary, not the coercion the earlier notes assumed).
Confirmed by an exact float match — OUT to-hit `Dex ^ (Dex/(wp+18)) * (wp+18)
/ (creatureHP*11)` reproduces the observed `ds:208E` = `0x3EAB17EA` bit for
bit (`16^0.8 * 20 / 550 = 0.33416682`).

## Value-stack node layout

Each 12-byte node is `[8 bytes unused][2-byte pointer][2-byte type tag]`.
**The node's value is stored at `[pointer]`**, not inline — and `pointer`
is `nodeStart + 12`, i.e. it points into the next node's space. So:

- the **top-of-stack value is the 4-byte single at `[ds:111C]`** (the stack
  pointer itself), *not* at `[ds:111C]-12`.
- `rtm_FF4B` (push) writes the value to `[oldPtr+12]`, sets the new node's
  pointer field to `oldPtr+12`, tag to 3, advances `ds:111C` by 12.
- `rtm_FF50 addr` (pop→var) does `si = topNode.pointer ; movsw movsw` from
  `ds:si` to `ds:addr`, then `ds:111C -= 12`.

When dumping the value stack in DOSBox, read `[ds:111C]` for the top value
and `[ds:111C - 12*k]` for the k-th-from-top **node header** (its value is
at that header's pointer field).

**Still `'?ord`:** the compare direction (`FF1F` + `jb`/`jnb`), and whether
a stack-stack op sees the deeper or the top operand as its left side (only
matters for `-` / `/`, and `FF49` vs `FF47` / `FF53` already encode the two
directions).

**String / misc thunks:** `basStrAssign(dst,src)` = `dst$ = src$`;
`basStrConcat(a,b)` = `a$ + b$`; `rtm_D2(fmt,val)` = `fmt$ + STR$(val)`;
`drawString` / `drawStringInner` render a position-coded string to CGA;
`rtm_FE27(addr)` = pause `[addr]` ticks / wait for key.

## The DGROUP constant pool

Every module shares the LEGLIB DGROUP layout: LEGLIB scratch in the low
`~0x1AC0` bytes (value stack `ds:111C`, op table `ds:0F7C`), then the
module's own SINGLE / INTEGER / STRING constants and variables. IDA's
`.asm` export writes the whole DGROUP as `db 0`, so the constant *values*
are missing from `../*.asm` — but they are in the unpacked EXE image.
`decoders/dgroup_consts.py <MODULE.EXE>` reads them back (DGROUP:0 sits at
file offset `0x8C80` in `OUT.EXE`, found via the `"ENEMY HIT BY BLOW OF "`
anchor). The combat pool it prints is quoted at the top of
`out_combat.bas`.

## Status

- [x] `out_combat.bas` — the overworld player-attack path (pilot v4)
- [x] `decoders/dgroup_consts.py` — pull the SINGLE/INT/STRING constant
      pool out of any module EXE
- [x] leglib op-dispatch table read from `LEGLIB.EXE` (`+ - -rev * / /rev
      cmp`); `FF2B` = `^` reversed
- [x] value-stack node layout (value at `[node.ptr]` == `[ds:111C]` for top)
- [x] verified formulas: to-hit (exact float), base/chip/weakness-match
      damage, RollEncounterMod
- [ ] confirm the to-hit `L` (= `Dex/(wp+18)`?) with a 2nd trace
- [ ] `out_combat.bas` — the monster-attack path (`creatureAttack`, still
      collapsed in `out.asm`)
- [ ] the rest of `out`, then `dun`, `casdr`, `mus`, `twndr`
- [ ] `leglib.bas` — the shared engine primitives
