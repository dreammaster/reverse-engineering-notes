# RNG validation — live capture vs `engine/wiz/rng.h`

Goal: confirm the reproduced PRNG (`engine/wiz/rng.h`, decoded in
`docs/pmachine.md` §RANDOM) matches the shipped interpreter bit-for-bit —
seed, recurrence, and output — and find out whether anything **other than**
a `RANDOM` call advances the state during real play.

## The target

`SYSTEM.INTERP` is a raw ≤16 KB blob (`interp_start` at file offset 0).
The native RNG lives at **file offset `0x221E`**; its 4-word state and
4-word increment table live at **`0x1437`** (`byte_13F5 + 0x42`):

```
0x221E  mov ax, cs:[1437]      ; s0
        mov bx, 6A2Dh
        mul bx
        add ax, cs:[143F]      ; += c0
        mov cs:[1437], ax
        ... same for s1  (mult FFF1h, state cs:[1439], += cs:[1441])
        ... same for s2  (mult FFAFh, state cs:[143B], += cs:[1443])
        ... same for s3  (mult FFD9h, state cs:[143D], += cs:[1445])
        mov cx, 4
        mov ax, cs:[1437] ; shl ax,cl ; and ax,FF00h ; mov bx,ax
        mov ax, cs:[143B] ; shr ax,cl ; and ax,00FFh ; xor bx,ax
        mov ax, cs:[1439] ; and ax,0FF0h ; xor bx,ax
        mov ax, cs:[143D] ; mov cl,8 ; rol ax,cl ; and ax,F00Fh ; xor bx,ax
        and ax, 7FFFh
0x2298  mov [di], ax          ; <-- result handed to p-code, in AX
0x229A  jmp interp_fetch
0x229D  <keyboard "stir": push ax..dx; adds the four c-constants
         cross-wise into the four s-states; retn>
```

Only `s3` reaches the output (the mix goes into `BX`, which the code
discards — a shipped bug): `RANDOM = rol(s3,8) & 0x700F`, 128 values,
period 65536.

Memory at `INTERP:1437` — 16 bytes, little-endian words:

| off  | word | ships as       |
|-----:|------|----------------|
| 1437 | s0   | `AB 5B` 0x5BAB  |
| 1439 | s1   | `2B D0` 0xD02B  |
| 143B | s2   | `15 7E` 0x7E15  |
| 143D | s3   | `51 73` 0x7351  |
| 143F | c0   | `19 36` 0x3619  |
| 1441 | c1   | `8B FF` 0xFF8B  |
| 1443 | c2   | `83 01` 0x0183  |
| 1445 | c3   | `C9 7F` 0x7FC9  |

## Finding the interpreter in memory (DOSBox)

The p-machine runs interpreted, so **`CS` is the INTERP segment the whole
time** the game is up. Break anywhere in-game and read `CS`.

Sanity-check the offset with a memory search for the routine's opening
bytes (distinctive — `mov ax,cs:[1437]; mov bx,6A2D; mul bx`):

```
2E A1 37 14 BB 2D 6A F7 E3
```

The hit is `INTERP_base + 0x221E`.

## Capture

```
BP <CS>:221E
```

On **each** hit, dump the state *before* it advances:

```
D <CS>:1437 L10      ; s0 s1 s2 s3 c0 c1 c2 c3
```

Collect **~64 consecutive hits**, spanning the title/menu, a few dungeon
steps, and at least one full combat (combat burns `RANDOM` fast).  The
first hit's state, if you broke before the game rolled anything, is the
seed.

Then, separately, to check the keyboard "stir": `BP <CS>:229D`, play
through menus / movement / a fight, and note whether it ever fires and
after which action.  (If the state trace above already shows
`state[i+1] == lcg_advance(state[i])` for every step, the stir never ran
between rolls and we're done — the trace catches it either way.)

## Compare

```
wiz1 rng-trace <s0hex> <s1hex> <s2hex> <s3hex> [n]
```

seeds the full 4-state generator and prints, per call:

```
   i   s0   s1   s2   s3    out=<result>  bx=<store-BX variant>
```

The `s0..s3` columns are the state **after** advancing on call `i`, so
they line up with a `D <CS>:1437` taken at `<CS>:221E` on call `i+1`.
Feed the first captured state in; every subsequent line must match the
capture exactly.  `out=` is what `engine/wiz/rng.h` (hence the whole
engine) produces; `bx=` is the "intended" mix the ROM throws away.

## Pass criteria

1. `c0..c3 == {3619, FF8B, 0183, 7FC9}` and constant across the run.
2. Seed (first pre-roll state) `== {5BAB, D02B, 7E15, 7351}`.
3. Every `state[i+1]` equals `rng-trace`'s advance of `state[i]` — i.e.
   nothing but `RANDOM` moves the state during play (or: the exceptions
   are exactly the `<CS>:229D` stir hits, and we then model them).
4. Each call's `AX` at `<CS>:2298` equals `rng-trace`'s `out=` column.
