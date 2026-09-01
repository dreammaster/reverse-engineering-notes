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
`[ds:0F7C]` table is runtime-filled (zero in the image), but the indices
are hard-coded in the thunks and map to the canonical BASIC operator order:

| index | op | immediate-operand thunk (`mov bx,addr` first) | stack-stack thunk |
|---|---|---|---|
| `0x00` | **`+`** | `rtm_FF44` | `rtm_FF42` |
| `0x04` | **`-`** | `sub_21A02` | `sub_21A4A` |
| `0x08` | **`*`** | `rtm_FF53` | — |
| `0x0C` | **`/`** | `rtm_FF4E` | `rtm_FF4C` |
| `0x10` | **`\`** | `sub_21A4A`* | `rtm_FF47` |
| `0x14` | **`MOD`** | `rtm_FF49` | (`seg004:2186`) |
| `0x18` | **`^`** | `sub_21B63` | `sub_21B63` |
| `0x1C` | **compare** | — | `rtm_FF1F` (sets carry for `jb`/`jnb`) |

\* the `0x04`/`0x10` immediate thunks share code paths in the listing; the
index in `ax` is what matters. Operator identity is now solid; **operand
order** for `- / \ MOD ^` (i.e. is the immediate/second push the divisor or
the dividend) is the remaining unknown — tagged `'?ord`. One DOSBox watch
on the destination of an expression settles it for the whole codebase.

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

- [x] `out_combat.bas` — the overworld player-attack path (pilot v2)
- [x] `decoders/dgroup_consts.py` — pull the SINGLE/INT/STRING constant
      pool out of any module EXE
- [ ] one DOSBox watch on `ds:2192` / `ds:208E` during a fight → pins
      operand order + op `0x10` for the whole codebase
- [ ] `out_combat.bas` — the monster-attack path (`creatureAttack`, still
      collapsed in `out.asm`)
- [ ] the rest of `out`, then `dun`, `casdr`, `mus`, `twndr`
- [ ] `leglib.bas` — the shared engine primitives
