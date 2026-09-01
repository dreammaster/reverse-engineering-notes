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
| `'?op` | the value-stack operator is inferred (see the op table below) — the *operands* are certain, the `+ - * \ MOD` between them may be wrong |
| `'??` | the whole expression / branch is a best guess |
| `'CHECK` | needs a DOSBox memory-watch or the `leglib` `[ds:0F7C]` op-dispatch table to confirm |

Each reconstructed block ends with `' asm: out.asm:NNNN` pointing at the
source lines.

## The `leglib` value-stack model

Arithmetic in compiled BASIC 6.0 goes through a runtime value stack
(`ds:111C` = stack pointer, 12 bytes per slot). The disassembly is a
stream of `int 3Fh` thunks; this table is how to read them back as infix:

| thunk | leglib | meaning |
|---|---|---|
| `rtm_FF20(ax)` | `B$PUSHI` | push integer register/literal `ax` |
| `rtm_FF4B(addr)` | push | push the INTEGER at `[addr]` |
| `rtm_FF50(addr)` | pop | pop top → `[addr]` |
| `rtm_FF22` | pop→AX | pop top into `ax` (end of expression) |
| `rtm_FF44(addr)` / `rtm_FF42` | op `0x00` | **`+`** (with immediate `[addr]` / stack-stack) |
| `rtm_FF53(addr)` | op `0x08` | **`*`** `'?op` |
| `rtm_FF4E(addr)` / `rtm_FF4C` | op `0x0C` | **`/`** (real divide) `'?op` |
| `rtm_FF47` | op `0x10` | **`\`** (integer divide) `'?op` |
| `rtm_FF49(addr)` | op `0x14` | **`MOD`** `'?op` |
| `rtm_FF1F` | op `0x1C` | **compare** (`<` — sets carry for `jb`/`jnb`) |
| `rtm_FF27` / `rtm_FF2B` | — | long↔int coercion (no value change) |
| `rtm_B8(seg,off)` | `B$RND` | **`RND`** — next pseudo-random; the arg is the game-data far pointer used as a nonzero "advance" seed |
| `rtm_FC` | `B$RANDOMIZE`-ish | seed the RNG |
| `basStrAssign(dst,src)` | — | `dst$ = src$` |
| `basStrConcat(a,b)` | — | `a$ + b$` |
| `basStrBuild` | — | `a$ = STR pattern` (PRINT USING-ish) |
| `rtm_D2(fmt,val)` | — | `fmt$ + STR$(val)` |
| `drawString` / `drawStringInner` | — | render a position-coded string to the CGA screen |
| `rtm_FE27(addr)` | — | pause `[addr]` ticks / wait for key |

**`'?op` caveat:** the `0x00 / 0x08 / 0x0C / 0x10 / 0x14` selectors map to
`+ * / \ MOD` on the standard QuickBASIC 4.x integer op vector (4-byte
stride), which is a strong guess but unverified for this exact runtime.
Extracting `leglib`'s `[ds:0F7C]` dispatch table, or watching one fight in
DOSBox, nails it.

## Status

- [x] `out_combat.bas` — the overworld player-attack path (pilot)
- [ ] `out_combat.bas` — the monster-attack path (`creatureAttack`, still
      collapsed in `out.asm`)
- [ ] the rest of `out`, then `dun`, `casdr`, `mus`, `twndr`
- [ ] `leglib.bas` — the shared engine primitives
