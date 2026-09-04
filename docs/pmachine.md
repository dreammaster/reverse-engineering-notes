# The DOS Wizardry p-machine (`SYSTEM.INTERP`)

16 KB of 16-bit real-mode x86. Build string `9/10/87 00:22:00`, © 1987 RWI Inc.
It is a **standard UCSD p-System interpreter** (II.1 / IV lineage) — the opcode
numbering below is confirmed against the handlers, not assumed.

Analysis DB: `wiz1_interp.idb` / `wiz1_interp.asm` (rebuild: see README).

## Runtime model

| p-machine register | x86 | notes |
|---|---|---|
| IPC (instruction ptr) | `SI` | opcodes fetched with `lodsb`; `CS`-relative into the current code segment |
| eval stack | `SP` (CPU stack) | grows down; a "push"/"pop" *is* an x86 `push`/`pop` |
| MP / current activation record | `BP` | locals at `[BP+0Ah+off]`; `[BP+0]`/`[BP+2]` = static/dynamic links; `[BP+0Ah]` = saved SP; `[BP+0Ch..]` = parameters/locals block |
| global (base) data ptr | `word ptr ds:[600h]` | globals at `[base+0Ah+off]` |
| heap ptr / heap top | `ds:[604h]` / `ds:[602h]` | `NEW` bumps `604h`, traps to "Stack overflow!!" if it passes `602h` |
| current seg# / proc# | `ds:[60Ch]` / `ds:[60Eh]` | |
| KP-ish / seg tables | `ds:[606h]`, `ds:[608h]`, `ds:[60Ah]` | proc-dictionary and code-pointer scratch |

Interpreter core (`interp_reloc` → `loc_E7`):

```
loc_E7:  xor ah,ah
         lodsb                 ; AL = opcode, SI++
         or al,al
         jns  loc_FA           ; AL < 0x80  -> SLDC
         mov  di,si            ; save IPC
         shl  al,1             ; (AL-0x80)*2
         mov  bx,ax
         jmp  cs:off_2A1[bx]   ; 128-entry dispatch, opcodes 0x80..0xFF
loc_FA:  push ax               ; SLDC: push constant 0..127
         jmp  loc_E7
```

- **Opcodes `0x00..0x7F` = SLDC** (short load constant): push the opcode byte.
- **Opcodes `0x80..0xFF`** dispatch through **`off_2A1`** (file offset `0x2A1`,
  128 words). This is *the* table.
- "Big" operands are read by `sub_FF`: 1 byte if `<0x80`, else `(b&0x7F)<<8 |
  next`; the routine returns the value **pre-doubled** (word offset → byte).
- Static-link walk for intermediate scopes: `sub_10F` (follow `[BP]` `n` times).

## Opcode table (`off_2A1`, opcodes 0x80–0xFF)

Confirmed from handler semantics. "big" = `sub_FF` operand, "ub"/"sb" = 1
unsigned/signed byte, "w" = inline word.

| op | mnemonic | handler | operands | action |
|---|---|---|---|---|
| 80 | ABI | loc_2777 | – | abs(int) |
| 81 | ABR | loc_268C | – | real → **trap** "FP not supported" |
| 82 | ADI | loc_2782 | – | add int |
| 83 | ADR | loc_268C | – | real → trap |
| 84 | LAND | loc_2760 | – | bitwise/logical and |
| 85 | DIF | loc_33E8 | – | set difference |
| 86 | DVI | loc_27A1 | – | `idiv` quotient |
| 87 | DVR | loc_268C | – | real → trap |
| 88 | CHK | loc_267A | – | range check `lo ≤ x ≤ hi`, traps "Range Error" |
| 89 | FLO | loc_268C | – | real → trap |
| 8A | FLT | loc_268C | – | real → trap |
| 8B | INN | loc_3237 | – | set membership |
| 8C | INT | loc_3359 | – | set intersection |
| 8D | LOR | loc_2768 | – | or |
| 8E | MODI | loc_27AE | – | `idiv` remainder |
| 8F | MPI | loc_2799 | – | `imul` |
| 90 | MPR | loc_268C | – | real → trap |
| 91 | NGI | loc_278A | – | negate int |
| 92 | NGR | loc_268C | – | real → trap |
| 93 | LNOT | loc_2770 | – | `not` |
| 94 | SRS | loc_32C2 | – | build subrange set `[a..b]` |
| 95 | SBI | loc_2791 | – | subtract int |
| 96 | SBR | loc_268C | – | real → trap |
| 97 | SGS | loc_32BF | – | build singleton set `[a]` |
| 98 | SQI | loc_2657 | – | square int (trace-hooked) |
| 99 | SQR | loc_268C | – | real → trap |
| 9A | STO | loc_3436 | – | `*NOS = TOS` |
| 9B | IXS | loc_36D8 | – | string index bounds check |
| 9C | UNI | loc_3394 | – | set union |
| 9D | LDE | sub_2630 | big | load word from another segment's global |
| 9E | CSP | loc_2B40 | ub | call standard procedure (sub-table below) |
| 9F | LDCN | loc_26A0 | – | push NIL (0) |
| A0 | ADJ | loc_3272 | ub | adjust set to `n` words |
| A1 | FJP | loc_27D0 | sb | jump if `TOS` false (pop) |
| A2 | INC | loc_3180 | big | `TOS += k` (address bump) |
| A3 | IND | loc_342B | big | `push *(TOS + k)` |
| A4 | IXA | loc_319D | big | index array: `TOS = NOS + idx*size` |
| A5 | LAO | loc_2706 | big | push address of global `k` |
| A6 | LSA | loc_36A0 | ub+bytes | push address of inline string constant, skip it |
| A7 | LAE | loc_264A | big? | load address extended (inter-segment) (trace-hooked) |
| A8 | MOV | loc_318A | big | move `k` bytes `NOS ← TOS` |
| A9 | LDO | loc_26F7 | big | push global word `k` |
| AA | SAS | loc_36AB | ub | string assign (with length cap) |
| AB | SRO | loc_2716 | big | pop → global word `k` |
| AC | XJP | loc_27F5 | word-aligned tbl | case jump `[min,max,default,targets…]` |
| AD | RNP | loc_28E6 | ub | return from non-base proc, drop `n` params |
| AE | CIP | loc_2861 | ub | call intermediate proc |
| AF | EQU | loc_34DA | ub type | `=` (typed: 1 EQ / 2 LES / 3 LEQ / 4 GTR / 5 GEQ / 6 NEQ) |
| B0 | GEQ | loc_34EE | ub type | `≥` |
| B1 | GTR | loc_34F3 | ub type | `>` |
| B2 | LDA | loc_2737 | ub ll, big | push address of intermediate var |
| B3 | LDC | loc_343D | ub, words | push multi-word inline constant |
| B4 | LEQ | loc_34E4 | ub type | `≤` |
| B5 | LES | loc_34E9 | ub type | `<` |
| B6 | LOD | loc_2726 | ub ll, big | push intermediate word |
| B7 | NEQ | loc_34DF | ub type | `≠` |
| B8 | STR | loc_2749 | ub ll, big | pop → intermediate word |
| B9 | UJP | loc_27D9 | sb | unconditional jump |
| BA | LDP | loc_31E7 | – | load packed field (`TOS`=shift, mask tbl `cs:[…+3215h]`) |
| BB | STP | loc_31F9 | – | store packed field |
| BC | LDM | loc_3452 | ub | push `n` words from `*TOS` |
| BD | STM | loc_3465 | ub | pop `n` words into `*(stack[n])` |
| BE | LDB | loc_36F0 | – | load byte `TOS[NOS]` |
| BF | STB | loc_36FA | – | store byte |
| C0 | IXP | loc_31C3 | ub ub | index packed array (elems/word, field width) |
| C1 | RBP | loc_28D3 | ub | return from base proc (restores global base) |
| C2 | CBP | loc_28A4 | ub | call base proc |
| C3 | EQUI | loc_3480 | – | int `=` |
| C4 | GEQI | loc_34BC | – | int `≥` |
| C5 | GTRI | loc_34CB | – | int `>` |
| C6 | LLA | loc_26C9 | big | push address of local `k` |
| C7 | LDCI | loc_26A6 | w | push inline word constant |
| C8 | LEQI | loc_349E | – | int `≤` |
| C9 | LESI | loc_34AD | – | int `<` |
| CA | LDL | loc_26BE | big | push local word `k` |
| CB | NEQI | loc_348F | – | int `≠` |
| CC | STL | loc_26D5 | big | pop → local word `k` |
| CD | CXP | loc_281A | ub ub | call external proc `(seg, proc)` — loads seg via `sub_29FD` |
| CE | CLP | loc_284C | ub | call local proc |
| CF | CGP | loc_288E | ub | call global proc |
| D0 | LPA | loc_31B8 | ub+bytes | push address of inline packed/byte constant, skip |
| D1 | STE | loc_263D | big | store word to another segment's global (trace-hooked) |
| D2 | NOP | loc_FD | – | no-op (`jmp` fetch) |
| D3 | (unimpl) | loc_2666 | – | → trap "Unimplemented Instruction" |
| D4 | (unimpl) | loc_2666 | – | → trap |
| D5 | (skip1) | loc_2671 | big | read + discard operand |
| D6 | BPT/HLT | loc_2677 | – | → `loc_E4` → hang (`loc_1307: jmp $`) |
| D7 | NOP | loc_FD | – | no-op |
| D8–E7 | SLDL 1–16 | loc_26AD | – | short push local word 1..16 (`[BP+0Ch+2*(op-0xD7)]`) |
| E8–F7 | SLDO 1–16 | loc_26E1 | – | short push global word 1..16 |
| F8–FF | SIND 0–7 | loc_3420 | – | short `push *(TOS + 2*(op&7))` |

Cross-checks that pin the numbering: the 9 real-arithmetic opcodes
(ABR/ADR/DVR/FLO/FLT/MPR/NGR/SBR/SQR) all share the "FP not supported" stub
`loc_268C`; SLDL/SLDO/SIND occupy the canonical `0xD8-0xFF` block; CSP `NEW`,
`MOVELEFT`, `MOVERIGHT`, `MARK`, `RELEASE` land on their standard numbers.

## CSP — call standard procedure (`0x9E`, table `jpt_2B4D`, 34 entries)

`AL = [SI]` selects. Confirmed: **1 NEW**, **2 MOVELEFT**, **3 MOVERIGHT**,
**4 EXIT**, **5 UNITREAD**, **6 UNITWRITE**, **10 FILLCHAR**, **11 SCAN**,
**32 MARK**, **33 RELEASE**. Others (0, 7–9, 12, 21–24) resolved to handlers
but not yet fully decoded — table:

| # | handler | # | handler | # | handler |
|--:|---|--:|---|--:|---|
| 0 | loc_2E88 | 8 | loc_2BF8 | 22 | loc_2B24 (seg release `sub_2ABF`) |
| 1 NEW | loc_2BBD | 9 | loc_2CB4 | 23 | loc_2BAC |
| 2 MVL | loc_2C86 | 10 FLC | loc_2C34 | 24 | loc_2BAF |
| 3 MVR | loc_2C73 | 11 SCN | loc_2C44 | 25–31 | loc_2BA1 (unimpl) |
| 4 XIT | loc_2AD4 | 12 | loc_2E95 | 32 MARK | loc_2BDC |
| 5 URD | loc_2CCF | 13–20 | loc_2BA1 (unimpl) | 33 RELEASE | loc_2BE9 |
| 6 UWR | loc_2DB9 | 21 | loc_2B2B (seg load `sub_29FD`) | | |
| 7 | loc_2EDB | | | | |

## Calls & segment loading

- `sub_293E` — common proc-entry: resolve `(seg#, proc#)` in the proc
  dictionary, detect native segments, build the new activation record.
- `sub_29FD` — **dynamic code-segment loader**: ref-counts in the table at
  `ds:[…+6B2h]`, load address `[…+672h]`, reads the segment off disk. `CXP`
  (`0xCD`) triggers it on first reference; "call unlinked segment" is the trap.
- Return: `RNP`/`RBP` restore `SP` from `[BP+0Ah]`, `BP` from the dynamic link.

## RANDOM  (WIZARDRY p-code proc 34)

`RANDOM` is not a CSP — it is `UNITREAD(unit 13, buf, len 0, subfn 10, …)`.
Unit 13's SBIOS handler routes subfn 10 to the generator at `0x221E`:

```
s0 = s0*0x6A2D + 0x3619        s1 = s1*0xFFF1 + 0xFF8B     (all mod 2^16)
s2 = s2*0xFFAF + 0x0183        s3 = s3*0xFFD9 + 0x7FC9
bx  = (s0<<4 & 0xFF00) ^ (s2>>4 & 0xFF) ^ (s1 & 0x0FF0) ^ (byteswap(s3) & 0xF00F)
ax  = byteswap(s3) & 0xF00F & 0x7FFF
result := ax                  ; *** stores AX, not the mixed BX -- shipped bug ***
```

So the four LCGs all advance but **only `s3` reaches the output**:
`RANDOM = byteswap(s3) & 0x700F` — **128 distinct values**, period 65536.
This is the notoriously weak PC-Wizardry RNG.  Reproduced in
`engine/wiz/rng.h` (`nextIntended()` = the store-BX version for comparison).

**Validated against a live DOSBox capture (2026‑09‑04) — bit-exact.**  See
`rng-validation.md`.  Findings:

* The `0x221E` code, the four LCG constants and the `0x143F` increment
  table (`{3619, FF8B, 0183, 7FC9}`) match the image byte-for-byte.
* During **outcome rolls** (combat resolution etc.) the state advances by
  a pure 4×LCG step — `rng.h next()` exactly.  An 11-call live combat
  trace matched `wiz1 rng-trace` on every state word.
* A **keyboard "stir"** at `0x229D` (`s0+=0183, s1+=7FC9, s2+=3619,
  s3+=FF8B`) runs once per iteration of the cursor-blink / prompt-wait
  loop, and the same loop's empty `int 16h` peek zeroes `s0`/`s1`.  Those
  rolls are thrown away — they never reach game state — but they mean the
  `s3` entering a resolution depends on player timing, so a whole real
  session is **not** deterministically replayable.  The documented ship
  seed `{5BAB, D02B, 7E15, 7351}` was already off-orbit by the title
  screen (the stir ran during boot); the seed doesn't matter to the model.
* Unit 1/2 subfn is the real keyboard (`int 16h`), used by `GETKEY`.

## CONUNIT — unit 13, the console / graphics driver

`CONUNIT` (global word 60) is fixed to **unit 13** (`WIZARDRY` proc 1:
`SLDC 13; SRO 60`).  Every screen operation and `RANDOM` go through it as
`UNITREAD/UNITWRITE(13, buf, len, blocknumber, mode, 0)` — the p-code's
"blocknumber" argument is really the **sub-function selector**.

Dispatch (analysed 2026-09-04, `ida_scripts/analyze_conunit.py`):

```
CSP UNITREAD  (loc_2CCF) ─┐  pop mode,blk,len,buf,unit
CSP UNITWRITE (loc_2DB9) ─┘  → per-unit table  0x2CF7 (URD) / 0x2DDE (UWR),
                               unit 13 → native  0x144B (URD) / 0x145B (UWR)
0x144B/0x145B: bx = blk*2; jmp [table + bx]
   URD table 0x134E     UWR table = jpt_1466
```

**URD (read) blocknumbers**

| blk | handler | effect |
|--:|---|---|
| 10 | `0x221E` | `RANDOM` — the 4×LCG generator (above) |
| 11 | `0x22CD` | "is a key buffered?" — `int 16h,AH=1`; also `s0*=k, s1*=-k` |
| 12 | `0x22FD` | returns 1 |
| 16 | `0x23E4` | `sub_1210` — ? |

**UWR (write) blocknumbers** — `jpt_1466`, 35 entries; the non-`noop` ones:

| blk | handler | effect |
|--:|---|---|
| 2  | `loc_153E` | register / lay out a window (the cell dirty-map) |
| 3  | `conunit_present` | flush dirty cells of every window to the screen (`int 10h`) |
| 5  | `loc_18C1` | **`DRAWLINE`** — plot a run of `len` points `(dH,dV)` into the offscreen cell buffer (the 3-D wireframe primitive) |
| 13 | `conunit_blit13` | **compose the combat monster portraits** — read up to 4 `PIC` ids, place a 5×6 block of tile-glyph codes per group into the offscreen buffer, then `sub_1A4A` loads their `200.MONSTERS` records (4-slot LRU cache) |
| 14 | `conunit_load_charset` | load `*.CHARSET` into a driver font slot (`word_13DC`) |
| 17 | `conunit_blit17` | clear the offscreen cell buffer |
| 18 | `conunit_load_monsters` | stash the `200.MONSTERS` file number (`word_1419`), init the 4-slot portrait-record cache |
| 19 | `conunit_scanline19` (`0x2305`) | **the timed pause** (`SETTIME`/`TIMEDLAY`) — spin `word_1449` times, each iteration polling `int 1Ah` clock ticks and `int 16h` for a key to abort |

**Monster portraits are tile-composed, not a compressed bitmap.**  Each
`200.MONSTERS` record (512 B) is a small **per-monster sub-font** (~30 tile
glyphs); `conunit_blit13` lays out char codes 1..30 in a 5×6 grid per
group and the normal cell renderer draws them from that record.  This
corrects the earlier "compressed 1bpp image" guess in `file-formats.md`.

## Native / SBIOS interface

`WIZ1.COM` stays resident and hooks **`INT 18h`** (the PC cassette/ROM-BASIC
vector) as the disk callback — all block I/O goes through it, serviced from
`WIZ1.DSK`. There is no BIOS `INT 13h` or DOS in the running game.

| service | vector | where |
|---|---|---|
| disk block read/write | `INT 18h`, `AH=18h`, `CL=9` | `sub_1108` (wraps every `UNITREAD`/segment load) |
| keyboard | `INT 16h` | `sub_676` (13-case helper) and inline |
| video | `INT 10h` | inline |
| equipment / RAM size | `INT 11h` / `INT 12h` | `sub_1309` (drive table), `sub_1276` (checks ≥128 KB, else "Not enough memory. Wizardry requires 128k…") |

`HAS.STROPS` (512 B of native x86 from the disk) is loaded as a machine-code
segment and called via `CXP` into native — linkage details are Phase-1 work.

## Diagnostics baked in

- Fatal handler `loc_AE`: `BX` = message ptr, prints, then `loc_1307: jmp $`.
  Messages at `0x3A1..0x52D` (`Stack overflow!!`, `EXIT to uncalled
  procedure`, `call unlinked segment`, `Range Error`, `Unimplemented
  Instruction`, `Bad virtual disk i/o request`, `Disk error returned during
  cache read`, …).
- `sub_374C` — printf-style formatter (`%`/`\` escapes), used for both fatals
  and (when `cs:word_3710` ≠ 0) per-opcode **trace** output; disabled in the
  shipped build (`sub_3716` inits it off, hook → `nullsub_2`).
- Trace mnemonic strings at `0x3BC2` (`LDE STE LAE SQI`), illegal-instruction
  format `"Illigal Instruction at %ha4 code=%hb2"` at `~0x3BD0`.
- Reserved-word table at `0x2FA2` (`AND ARRAY BEGIN CASE CONST DIV …`, 8-byte
  stride) — compiler leftover, unreferenced at runtime.

## Open items

- CSP 0, 7–9, 12, 21–24 exact semantics.
- `sub_29FD` segment-table layout (feeds Phase 1 `SYSTEM.PASCAL` disassembly).
- `HAS.STROPS` native-call ABI.
- `0xA7 LAE` / `0x9D LDE` / `0xD1 STE` operand widths (seg + big vs big only).
