# The "Ultima II Upgrade" fan patch — findings for the ScummVM port

Scope, per Paul's request (2026-09-30): identify where the 3 named
bugfixes live and how each was fixed, and identify the tileset
read/render code. The map patch and general usability changes
(savegames, speed, hotkeys, frame limiter) are explicitly **not**
covered in depth here except where they turned out to be entangled
with the above (noted inline).

Source files examined: `c:\games\ultima2\ultima2.com` (the patch's
launcher stub, IDA-exported as `ultima2_enhanced.asm/idb/idc` in this
repo) and `c:\games\ultima2\ULTIMAII.EXE` (the patched game binary,
diffed byte-for-byte against `ULTIMAII_original.EXE`, both 37,344
bytes — **no IDA database exists for either EXE side of the patch**;
everything below on `ULTIMAII.EXE` was worked out from the binary diff
plus manual/`ndisasm` disassembly of the changed regions, cross-checked
against this project's existing full disassembly of the original EXE
(`ultima2.asm`/`ultima2.idb`). See "Methodology" at the bottom for how
to reproduce this and its limits.

## 1. `ultima2.com` is not the game — it's a driver-loading launcher

Confirmed from `ultima2_enhanced.asm`. It's a tiny (1,989-byte) `.COM`
stub that:

1. Reads `U2.CFG` (11 bytes on disk) to pick a video driver
   (`CGA.DRV`/`CGACOMP.DRV`/`EGA.DRV`/`VGA.DRV` — filenames patched
   from a template, same "patch a placeholder character in an inline
   string" idiom already documented for `MAPXFF` etc. in the original
   game) and a music/SFX driver (`NOMIDI.DRV`/`MIDIPAK.DRV`/
   `SFX.DRV`/`SFXTIMED.DRV`, plus references to `.MOD` tracker music
   and tune names `ULTIMA2`/`SOSARIA`).
2. Loads the selected video driver file into memory and installs it as
   the handler for **`int 65h`** (`sub_1069F`/`sub_106EA`: saves/
   restores the previous `int 65h`/`int 66h` vectors via `AH=35h`/
   `25h`), similarly for a couple of PIT-timer/clock interrupts
   (`int 8h`/`1Ch`/`64h`) used for driver timing.
3. `EXEC`s (`INT 21h AH=4B00h`) `ULTIMAII.EXE` — the actual game.
4. On exit, restores every interrupt vector it touched and frees the
   driver's memory.

**Practical upshot: `ultima2.com` itself contains none of the game
logic, bugfixes, or tile-rendering code.** Its only relevance is that
it establishes `int 65h` as the game's interface to a video driver
before the game ever runs — see §3.

## 2. The three named bugfixes

All three were found by diffing `ULTIMAII.EXE` against
`ULTIMAII_original.EXE` (7,102 differing bytes; 4,128 of those are a
pure uppercase→lowercase ASCII shift from the "complete conversion of
text to lower-case" feature and were filtered out, leaving 2,974 real
bytes across ~178 clusters to work through).

### 2.1 Vendor prices resetting with high stats — `compute_item_price`

**Root cause**: `compute_item_price` (asm ~3553, EA `0x120AB`) computes
a shop discount from `player._intelligence + player._charisma`. The
original code:

```asm
mov al, player._intelligence
adc al, player._charisma      ; plain binary add, no BCD adjust
mov bh, 0
mov bl, 0
mov si, bx
loc_120CF:
inc si
clc
rcr al, 1                     ; (clc first, so this is really >>1)
or al, al
jnz loc_120CF                 ; si = bit-length of the summed byte
```

Both attributes are BCD-encoded (0–99 stored as `0x00`–`0x99`, per
this project's existing docs). Summing two BCD bytes with a plain
`adc` (no `daa`) is only valid while the true decimal sum stays under
100 — the *byte-length* loop above then uses that raw sum as the
number of discount "steps." With high enough `INT`+`CHA` (a decimal
sum ≥ 100), the un-adjusted binary sum lands on an arbitrary invalid
byte, producing a much smaller bit-length than intended → less
discount → **prices reset toward full price for exactly the players
who should get the best discount.**

**The fix** (confirmed via `ndisasm` on the exact same bytes,
`ULTIMAII.EXE` file offset `0x22C9`, EA `0x120C9`):

```asm
mov al, player._intelligence
adc al, player._charisma
daa                 ; NEW: decimal-adjust the sum
jnc skip             ; NEW: if the decimal sum was < 100, done
mov al, 0x99         ; NEW: otherwise clamp to the max valid BCD value
skip:
and si, si           ; (si no longer explicitly zeroed here — see note)
inc si
shr al, 1             ; rcr+clc folded into a plain shr, same effect
or al, al
jnz loc_120CF
```

One loose end, noted honestly rather than guessed: the original's
explicit `mov bh,0/bl,0/si,bx` (zeroing `si`) is gone, replaced by
`and si,si` (a flags-only test, doesn't zero anything). The fixed code
must be relying on `si` already being `0` on entry from every one of
`compute_item_price`'s 6 call sites in `transact` — plausible but not
independently verified against all 6 sites.

### 2.2 Time gates corrupting terrain — `normal_movement`'s era-change code

**Root cause**: walking onto the "patrol waypoint" tile in
`normal_movement` (asm ~4475, EA `0x126B5`) advances
`player._mapEra` (the game's "which of the 5 Sosaria time periods /
other-planet map set" digit — see `docs/file-formats.md`), then
**unconditionally**:

```asm
mov player._mapEra, al      ; new era
call save_game
call load_map                ; blind reload from disk, no safeguards
```

i.e., a straight save-then-reload-from-disk with no caching, no
validation that the target map/monster files are even well-formed,
and nothing accounting for the well-documented fact (see
`docs/file-formats.md`) that some of the original DOS map files ship
genuinely corrupt. That combination — reload from possibly-bad data,
mid-transition, no rollback — is the "time gates corrupt the terrain"
bug.

**The fix** replaces both calls with calls into two brand-new
functions injected into space reclaimed from the original embedded
CGA tile graphics (EA `0x17B10` in place of `save_game`, `0x17B47` in
place of `load_map`; the original graphics data lived at `TILE_OFFSETS`
`0x179C0` onward and has been superseded — see §3). Traced behavior:

- **`0x17B10`**: queries `player._mapEra` plus a new flag byte at
  `DATA:0x135`, computes a **per-era buffer slot** in a *different*
  memory segment (`ds_new = ds + 0x528` paragraphs — i.e. a bank of
  fixed-size slots, one per era, kept resident in RAM), and
  block-copies a cached 4096-byte map snapshot plus a cached 256-byte
  `_mapMonsters` snapshot for that era back into the live buffers via
  `rep movsb`.
- **`0x17B47`**: calls a helper (`0x17ADD`, itself doing the mirror
  operation — snapshotting the *current* era's live map/monster state
  into its slot before switching away) then falls into a large new
  routine at `0x179C0`/`load_map`'s old table space that:
  - Queries the loaded video/data driver via **`int 65h, AH=0`**
    (present since `ultima2.com` installed the handler) to ask if a
    driver-side step is needed before/after the reload.
  - Contains a **retry loop that calls `save_game` repeatedly** and,
    on success, **patches the `MAPXFF`/`MONXFF`/`TLKXFF` filename
    templates' `'X'` byte to `'G'`** (`mov byte [cs:0x233B], 0x47`
    and `mov byte [cs:0x234D], 0x47` — confirmed literal `'G'` =
    `0x47`). The install directory has a full parallel `MAPG??`/
    `MONG??`/`TLKG??` file set alongside the original `MAPX??` etc.

**Correction to the "map patch needs no new code" assumption**: it
does. The `'G'`-prefixed files (the corrected/completed map set) are
selected by this same code path, gated by the `DATA:0x135` flag and a
"which prefix character to use" helper (`0x17B68`, called via
`patch_map_filename`) — this is architecturally the *same* subsystem
as the terrain-corruption fix, not a separate, code-free asset swap.
For the ScummVM port this mostly doesn't matter (you'd just always
load the corrected map data), but it explains why this particular
bugfix and "the map patch" show up interleaved in the same bytes.

Left open (didn't chase further — not needed for the ScummVM port,
which won't reproduce this DOS-specific RAM-caching scheme at all):
the exact semantics of the `DATA:0x135` flag, and the full behavior of
the `int 65h AH=0` query in this path.

### 2.3 Attributes rolling past 99 back toward 0 — `offer`'s "ALAKAZAM!" event

**Root cause**: found this is **not** in character creation (checked
exhaustively — see "ruled out" below). It's in `offer`'s random
magical-boost event (asm ~7407, EA `0x13D7E`, reached via a 6-in-8
`rand_byte` roll):

```asm
call rand_byte
and al, 7
cmp al, 6
jb short loc_13D8A     ; 6/8 chance to proceed
...
loc_13D8A:
mov ah, 0
mov di, ax                    ; di = 0-5, indexes STR..INT
mov al, player._strength[di]  ; the randomly-chosen attribute
adc al, [_sleepFlag2?]        ; add a bonus amount (reused scratch global)
daa
adc al, [_sleepFlag2?]        ; repeated a total of 4 TIMES
daa
adc al, [_sleepFlag2?]
daa
adc al, [_sleepFlag2?]
daa
mov [di+4Bh], al              ; write back (di+4Bh == player._strength[di])
call write_string             ; "ALAKAZAM!"
```

Four consecutive unclamped BCD additions to a random attribute, each
one able to overflow past 99 and wrap independently — a much more
aggressive version of the same class of bug as §2.1, and a direct,
literal match for "attributes roll beyond 99 (and back to 00)."

**The fix** (EA `0x13D9A` onward): replaces the four raw
`adc`/`daa` pairs with a loop that calls a new shared helper,
**`0x149EB`**:

```asm
; 0x149EB: al += [0x27]; daa; if carry, clamp al to 0x99
pushf
add al, [0x27]
daa
jnc skip
mov al, 0x99
skip:
popf
ret
```

```asm
; offer's fixed call site, EA 0x13D9A
mov al, [di+4Bh]
mov cx, 4
loop_top:
call 0x149EB        ; safely-clamped add, run 4 times
loop loop_top
mov [di+4Bh], al
```

This is the same fix *shape* as §2.1 (`daa` + carry-check + clamp to
`0x99`) but implemented as a reusable helper instead of inlined, and
proven via a full scan of every `call` in the patched binary that this
helper has exactly one caller — this site. High confidence, fully
traced end to end.

**Ruled out while searching** (documented so nobody re-treads this):
character creation's attribute entry (`get_number`, EA `0x151B0`) is
inherently bounded to 0–99 by construction (reads exactly 2 decimal
digits into a BCD byte) and is byte-identical between original and
patched; the sex-bonus and all 6 race-bonus `add`/`daa`/store sites in
character creation (EA range `0x1575B`–`0x1533E`ish) are byte-identical
between versions; `update_points_remaining` (the "points left to
distribute" pool logic, EA `0x15A66`) is byte-identical; the starting
90-point pool constant is unchanged. None of these are where the bug
or the fix lives.

## 3. The tileset read/render code

**Headline finding: tile/pixel rendering has been moved almost
entirely out of `ULTIMAII.EXE` and into the loaded `.DRV` file**
(`cga.drv`/`cgacomp.drv`/`ega.drv`), reached through a new `int 65h`
ABI that `ultima2.com` sets up before launching the game (§1).

### 3.1 The driver ABI

A new routine (EA `0x17D10`) runs at startup and calls **`int 65h`
with `AH` = 0, 1, 2, 3, 4, 5, 7, 8** — 8 distinct sub-functions,
storing each result into a small fixed table at `DATA:0x8C4`–`0x8FB`:

| `AH` | Result | Stored at | Apparent purpose |
|---|---|---|---|
| 0 | `AL` | `0x8F9` | capability/flag |
| 1 | `AL` | `0x8F8` | capability/flag |
| 2 | `AX:DX` (far ptr) | `0x8C4:0x8C6` | driver entry point #1 |
| 3 | `AX:DX` (far ptr) | `0x8C8:0x8CA` | driver entry point #2 |
| 4 | `AL` | `0x8FB` | capability/flag |
| 5 | `AX:DX` (far ptr) | `0x8D0:0x8D2` | driver entry point #4 |
| 7 | `AL` | `0x8FA` | capability/flag |
| 8 | `AX:DX` (far ptr) | `0x8CC:0x8CE` | driver entry point #3 |

The 4 entry points are **far call slots**; 4 tiny dispatcher stubs
(EAs `0x17CE4`, `0x17CED`, `0x17CF6`, `0x17CFF`, each just
`mov [slot],bp` / `call far [slot]` / `ret`) call through them.

### 3.2 Every low-level drawing primitive is now a thunk into that dispatcher

Confirmed by diffing the whole `setPalette`/`draw_tile`/
`draw_map_content`/`clear_screen`/`clear_cga_bank`/`erase_point`/
`draw_sprite_row`/`xorSpriteDraw` cluster (original EAs
`0x14A3E`–`0x14C53`, the single largest diff region in the file at
1,468 changed bytes). In the patched binary, functions that used to
contain real CGA framebuffer-writing code (e.g. the original
`draw_tile`, which unpacked a tile's pixels via the `cs:[bx+0x48DC]`
segment-lookup table directly into `0xB800`/`0xBA00`) are now just:

```asm
push bp
mov bp, 0xC          ; a driver-side opcode/function ID
call 0x17CE4          ; dispatch through driver entry point #1
pop bp
ret
```

Every such primitive (`draw_tile`, `clear_cga_bank`'s two halves,
`plot_point`/`erase_point`'s pixel writers, `draw_sprite_row`, the
XOR-drawing routines) got this same treatment, each with its own `bp`
opcode (`0x6`, `0x9`, `0xC`, `0xF`, `0x12`, `0x15`, `0x18`, `0x1B`,
`0x1E`, `0x24`, `0x27`, `0x2A`, `0x2D`, `0x30`, `0x33`, `0x36`, `0x39`
were all observed). **None of the actual pixel-format/tile-decoding
logic remains in `ULTIMAII.EXE`** — it's entirely inside whichever
`.drv` was loaded.

### 3.3 The `EGATILES`/`CGATILES` files

Confirmed a literal inline filename `EGATILES` (the "inline data right
after a CALL" convention already used throughout the original game —
see `docs/overview.md`) at old `draw_map_content+0x1D`
(`ULTIMAII.EXE` offset `0x981`), loaded via `mov dx, 0x2000` then a
call into new code. `CGATILES` does **not** appear as a literal string
anywhere in `ULTIMAII.EXE` — confirmed below (§3.5) that it's loaded
*from inside* `cga.drv`/`cgacomp.drv` instead, not by the EXE.

### 3.4 `cga.drv`, `cgacomp.drv`, `ega.drv` — disassembled (2026-10-01)

No IDA database — these are small enough that direct `ndisasm` plus
manual tracing was practical. Each is a flat, headerless binary file
(no MZ header — loaded verbatim into an allocated memory block by
`ultima2.com`, as described in §1) with an **identical structure**
across all three files, confirmed by checking all three:

- **A 20-entry jump table at offset 0**, each entry exactly 3 bytes
  (`jmp near rel16`). This **is** the ABI target of `ULTIMAII.EXE`'s
  `mov bp, N` / `call far [dispatch_slot]` thunks from §3.2 — `bp`'s
  value is *literally the byte offset into this table*
  (`0, 3, 6, 9, ...`), and the far pointer `ULTIMAII.EXE` got from
  `int 65h AH=2/3/5/8` points at this driver's segment:0 — the dispatch
  stub only overwrites the low word (the offset half) of that far
  pointer with `bp` before calling it. Confirmed every `bp` value
  observed in `ULTIMAII.EXE` (`6, 9, 12, 15, 18, 21, 24, 27, 30, 36,
  39, 42, 45, 48, 51, 54, 57`) lines up exactly with one of this
  table's 20 three-byte-aligned slots.
- Each jump-table entry lands on a tiny trampoline (`push
  ds`/`push es`/swap `ds`↔`cs`/`call <real function>`/`pop es`/`pop
  ds`/`retf`) — the `ds`↔`cs` swap is what lets the driver address its
  own data tables normally while still being able to reach the
  caller's segment via `es`. `retf` confirms these are genuinely
  far-called, matching `ultima2.com`'s own `call dword ptr [bp]`
  convention from §1.
- **Right after the jump table**: a table of word offsets into a block
  of literal, null-terminated filenames — in `cga.drv`/`cgacomp.drv`:
  the 7 original `PICDRA`/`PICOUT`/`PICTWN`/`PICCAS`/`PICDNG`/
  `PICSPA`/`PICMIN` title-slideshow picture names (unchanged from the
  original game); in `ega.drv`: `PICDRA.EGA`/`PICDNG.EGA`/`PICSPA.EGA`
  (3 of the 7 got full EGA remakes, matching the 64,000-byte `.EGA`
  files in the install dir) but `PICOUT.IDX`/`PICTWN.IDX`/
  `PICCAS.IDX`/`PICMIN.IDX` for the other 4 (200-byte files — these
  look like a palette/remap index applied to the *original* 16,384-byte
  CGA picture data rather than full redraws).
- Then (confirmed fully in `cga.drv` only — not re-verified byte-for-
  byte in the other two, though the same overall layout is present):
  `CGATILES`/`MONSTERS`/`CGATHEME.` filenames, a tile-size word pair
  (`0x10, 0x10` = 16×16, matching the original tile dimensions), and a
  **copy of the dungeon-monster "band offset" table** (`0x80, 0xC0,
  0xE0, 0xF0, 0xF8, 0xFC`) — the exact same 6 values this project
  already decoded as `_dungeonMonsterBandOffsets` in the original EXE
  (see `docs/file-formats.md`'s `MONSTERS` section). The driver has its
  own copy because it has taken over dungeon-monster-marker rendering
  too, not just tile blitting (see below).

### 3.5 What the driver's real code does (traced in `cga.drv`)

Found and matched up against this project's existing documentation of
the *original* game's drawing routines — every one of these is a
reimplementation of an original-game function, now living in the
driver instead:

| Driver function | Matches original | Evidence |
|---|---|---|
| one entry point | `clear_screen` | zero-fills video memory via `rep stosw`, called twice (two interleaved CGA banks) |
| another entry point | `flash_screen` | identical shape but XORs `0xFFFF` instead of zeroing — the invert-flash effect |
| another entry point | `setPalette` | `int 10h AH=0,AL=4` (set CGA mode 4) + two `AH=0Bh` palette-select calls, byte-for-byte the same BIOS call shape as the original |
| another entry point | `plot_point`/`erase_point` | computes a CGA byte address + 2-bit pixel mask from (x,y), then `and`s/`or`s a single pixel in |
| another entry point | `draw_sprite_row` | writes one full byte (4 pixels) across the interlaced-bank pair, 4 rows at a time |
| another entry point | `draw_ship_marker` | draws a 6+6-point `+` crosshair around a center point |
| another entry point | `draw_world_map_overview`'s glyph dispatch | **identical bit-pattern thresholds** (`0x10`, `0x78`/`0xF0`, `8`/`0xC`/`4`/`0x70`) to the original's tile-type dispatch for the "VIEW WITH MAGICAL HELM" simplified map, each branch calling a `plot_map_icon_point`-equivalent with the same sub-cell offset convention |
| another entry point | **`draw_dungeon_monster`** | reads a lookup-table-selected record from an `es:si`-addressed buffer (the loaded `MONSTERS` data), decodes a byte as **low 5 bits = Y position, top 3 bits (`>>5`) = facing/type** — the *exact* packed-record format this project already decoded for `_dungeonMonsterRecordPtr`/`_dungeonMonsterFacing` in the original EXE (see `docs/file-formats.md`) — then calls the sprite-row writer with the position scaled `×4`, matching the original's scaling exactly |
| another entry point | title-slideshow picture display | copies 16,384 bytes (`rep movsb`, `cx=0x4000`) from a loaded picture buffer into video memory — matches the original `PIC???` format's confirmed size exactly |
| 3 more entry points | BIOS text helpers | `int 10h AH=6` (scroll), `AH=2` (cursor position), `AH=9` (write char) — used for the driver's own on-screen text/status, not tile rendering |

**The pixel-address math** (the function every drawing primitive
ultimately calls through): given a byte-column `bx` and a row `dl`,

```
cl = (bl & 3) ^ 3; cl <<= 1          ; which 2-bit pixel slot within the byte
di = (dl is odd) ? 0x2000 : 0        ; interlaced-scanline bank selector
di += 80 * (dl >> 1)                  ; 80 bytes/scanline, CGA-standard
di += bx >> 2                         ; byte-column
```

This is the same interlaced-CGA-scanline math the original game uses
(`screen_rows`-style addressing, 80 bytes/row, odd/even scanlines in
separate banks) — but with one real architectural difference: the
original keeps the two banks as **two different real-mode segments**
(`0xB800`/`0xBA00`) and switches `es` between them; this driver instead
keeps **one segment and adds `0x2000` to the offset** for odd
scanlines. Didn't chase why (could be an off-screen/shadow buffer this
driver renders into before a real blit, relevant for CGA Composite
emulation in particular — not confirmed).

Also found: the actual **`CGATILES`/`MONSTERS` file loader** (`cga.drv`
calls it with the string address of each filename as an argument) and
a **once-loaded cache check** (skips the reload if a cached segment
pointer is already non-null) — so the driver, not the EXE, now owns
reading `CGATILES`.

**Not done**: `cgacomp.drv` (CGA Composite 16-color emulation) and
`ega.drv` weren't traced to this same depth — only their header/string
tables were confirmed to match `cga.drv`'s layout. `cgacomp.drv` in
particular (almost 2× the size of `cga.drv`) almost certainly has
additional logic for the composite-color emulation that `cga.drv`
doesn't need, and `ega.drv`'s tile format/size wasn't independently
re-verified (EGA tiles are presumably larger per `EGATILES`' file size
of 8,320 bytes vs. `CGATILES`' 4,160 — both roughly double the
original DOS CGA tile data's 4,224 bytes, consistent with a similar
tile count at larger per-tile size, but not confirmed byte-for-byte).

### 3.6 What this means for the ScummVM port

The pixel format and addressing scheme for CGA mode's tiles is now
fully understood (§3.5's address-math table, 16×16 tiles, `CGATILES`
loaded by the driver). For `cgacomp.drv`'s 16-color emulation and
`ega.drv`'s EGA tile format specifically, the *architecture* (same
jump-table ABI, same `MONSTERS`/picture-loading patterns) is confirmed
shared with `cga.drv`, but the mode-specific pixel/tile decode in each
has not been traced to the same depth — see "Next steps."

## 4. Next steps (need Paul / IDA, not done here)

1. Trace `cgacomp.drv`'s composite-color-specific logic and confirm
   `ega.drv`'s EGA tile pixel format/size — both only had their
   header/string tables checked against `cga.drv`'s confirmed layout
   (§3.4), not their actual drawing code (§3.5 is `cga.drv`-only).
2. Figure out why the driver's pixel-address math uses a single
   segment + `0x2000` offset for the interlaced scanline banks instead
   of the original's two-segment convention (§3.5) — may matter for
   getting CGA Composite emulation pixel-exact.
3. If useful: a full IDA database for `ULTIMAII.EXE` (patched) would
   let the remaining ~170 diff clusters (mostly small, UI-context or
   address-relocation noise per the sampling done here) be swept
   properly instead of by manual `ndisasm` spot-checks. Not needed for
   the 3 bugfixes or the tileset architecture above, which are already
   fully traced.
4. `player.full`, `u2-*.pat`, `u2up*.exe/ini` and the `u2cfg*.exe`
   tools in the install directory weren't examined at all — out of
   scope here (installer/config tooling, not runtime game code).

## Methodology (for reproducing or extending this)

- `ULTIMAII.EXE` (patched) and `ULTIMAII_original.EXE` are
  byte-identical in size (37,344 bytes) and — critically — **share the
  same `EA = file_offset + 0xFE00` addressing constant** as this
  project's existing `ultima2.idb` for the *unmodified* regions (i.e.
  most of the file). That let every diff cluster be mapped straight
  onto this project's already-fully-documented original disassembly
  without needing a fresh IDA database for the patched EXE — get a
  target file offset from a diff, add `0xFE00`, and look it up in
  `ultima2.idb`/`ultima2.asm`.
- New code the patch injected lives in space reclaimed from the
  original embedded CGA tile graphics (roughly EA `0x179C0` onward,
  i.e. file offset `0x7BC0` onward) — the single largest diff cluster
  (1,468 of 2,974 real changed bytes) is almost entirely this.
- Relative `call`/`jmp` targets were resolved with a small Python
  snippet reading the raw `E8`/rel16 bytes and applying the
  `+0xFE00` constant directly — **do not trust `ndisasm`'s own printed
  absolute target** for a `call`/`jmp` when using `-o` with an origin
  above `0x10000`; its internal IP tracking wraps at 64 KB and prints
  a wrong (silently plausible-looking) address. Decode the mnemonic
  from `ndisasm`, but compute the target yourself.
- 4,128 of the raw 7,102 diffed bytes are the "complete conversion of
  text to lower-case" feature (a pure `+0x20` ASCII shift on every
  uppercase letter) and were filtered out up front — worth doing this
  filter first in any future diff of these two files, since it removes
  more than half the noise immediately.
- The `.drv` files (§3.4-3.5) are a different, much simpler case: flat
  headerless binaries with no shared addressing convention to anything
  else, so `ndisasm -b 16 -o 0 <file>` directly (file offset == address
  offset) is all that's needed — no `+0xFE00`-style constant, no
  cross-referencing against `ultima2.idb`. The main hazard is the usual
  one with linear disassembly of a file that mixes code and data (the
  jump table, filename strings, and small lookup tables at the start of
  each file all disassemble as plausible-looking garbage if you don't
  recognize them as data first) — `python3`-side slicing + manual
  decoding of the data regions (word-table-of-offsets-into-a-string-
  blob, in this case) before trusting `ndisasm`'s output caught this
  quickly.
