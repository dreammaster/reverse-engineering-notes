# Legacy of the Ancients (DOS) — Disassembly Overview

Working notes on the Legacy of the Ancients reverse-engineering effort.
Goal: document each DOS executable (and the shared run-time module) well
enough to write a clean C++ reimplementation, then a ScummVM engine
module — same approach as the sibling [`ultima1`](../../ultima1) and
[`ultima2`](../../ultima2) projects.

This file is the entry point into `docs/`. See also:
- [roadmap.md](roadmap.md) — prioritized list of what's investigated vs.
  still open, per executable.
- [file-formats.md](file-formats.md) — on-disk data formats (maps, saves,
  graphics, music, etc.). Barely started.

## The game is compiled Microsoft BASIC, not C

The startup splash credits three copyrights:

```
Game Program Copyright (c) 1987 - 1989 Quest Software, Inc.
Installation Program Copyright (c) 1989 Electronic Arts
Program Compiler Copyright (c) 1982 - 1988 Microsoft Corp.
```

The "Program Compiler" line is **Microsoft BASIC Compiler 6.0** (1988 —
the release between QuickBASIC 4.5 and BASIC PDS 7.0, sometimes called
BASCOM 6.0), **not** the Microsoft C compiler. Proof is embedded in
`LEGLIB.EXE`:

```
"Microsoft (R) BASIC Compiler Runtime Version 6.00"
"Copyright (C) Microsoft Corp 1982-1988. All rights reserved."
"Illegal function call"  "Subscript out of range"  "Overflow"  "Redo from start"
```

— the stock Microsoft BASIC runtime banner and error-message table. The
C-looking `start` stub in each module (SETBLOCK, PSP/`PATH=` scan, MZ
header check, `C_FILE_INFO`) is just the BC 6.0 EXE bootstrap; BASIC PDS
shares a lot of runtime plumbing with MSC, which is why it reads as C at
first glance.

Consequence for RE: compiled BASIC 6 turns nearly every source statement
into a `call far` into the runtime. The per-module `.EXE`s are therefore
*thin* — mostly `push args` / `call far` streams routed through an
`int 3Fh` thunk table into `LEGLIB.EXE`. Once `LEGLIB`'s entry points are
mapped to `B$…` runtime names + game-specific engine routines, every
module's call stream becomes readable pseudo-BASIC. IDA's FLIRT has good
MSC coverage but weak BASIC-runtime coverage — expect to identify `B$…`
entry points by hand, cross-referenced against QuickBASIC 4.5 / BASCOM 6
/ BASIC PDS 7 documentation and the QB reverse-engineering community's
notes on BRUN.

## Architecture: one shared RTM + thin per-subsystem modules

| File | Size | Role (best guess unless noted) |
|---|---|---|
| `LEGLIB.EXE` | 109,716 | **Shared run-time module.** Custom BRUN60 (BASIC runtime) bundled with the game's common engine — the `bmXXXX` bitmap/graphics system, file I/O, input, etc. The real payload. Confirmed: contains the BASIC runtime banner; loaded as the "run-time module" by every module below. |
| `MENU.EXE` | 45,056 | Main menu / launcher. Also carries the EA 1989 "Installation Program". Chains to `OUT.EXE` / `MUS.EXE` / `DUN.EXE`. Oversized header (3664 bytes, 909 relocs) and entry `038F:00DF` vs. the others' `xxxx:0010` — may have been unpacked before import. |
| `OUT.EXE` | 37,109 | Overworld / outdoor engine. References `MUS.EXE`, `SAVER.EXE`, `TWNDR.EXE`, `CASDR.EXE`, `DUN.EXE`. |
| `DUN.EXE` | 25,353 | Dungeon engine. `DUNDATA.BSV`, `DUNM*.BSV`, `DUNMON*.BSV`, `DUNOBJ.BSV`. |
| `TWNDR.EXE` | 47,315 | Town driver. `TWNMSG.TXT`, `TOWN0.BSV`…`TOWNB.BSV`. |
| `CASDR.EXE` | 36,845 | Castle driver. `CASTLE.BS1`/`.BS2`, `TCASOBJ.BSV`. |
| `MUS.EXE` | 20,387 | Music player / jukebox. `MUSDATA.BSV`, `MUSMSG.TXT`, `MUSOBJ.BSV`. |
| `SDEFENDR.EXE` | 15,443 | "Space Defender" arcade minigame (one of the in-world arcade cabinets). `SDMAP.GLB`, `SDOBJ.GLB`, `SDMAP.GMP`. |
| `GMB1.EXE` / `GMB2.EXE` | 13,285 / 21,079 | Gambling / casino minigames. |
| `CELDRV.EXE` | 8,967 | Cel-animation driver. `CEL0.BSV`…`CEL3.BSV`. |
| `STDRV.EXE` | 24,923 | Driver (story? "ST"?). `STDRVSCR.DAT`. |
| `SAVER.EXE` | 5,903 | Savegame handler. |
| `CONFIGUR.EXE` | 10,349 | Standalone config utility — plain Microsoft C, **no LEGLIB dependency**. `DRCONFIG.DAT`. |

`LEGACY.BAT` is 4 bytes: `menu` — it just runs `MENU.EXE`.

All are 16-bit real-mode MS-DOS `MZ` executables. The `.GLB`/`.GMP`/`.BSV`/
`.BS1`/`.BS2`/`.DAT`/`.GLB` data files are engine-wide — see
[file-formats.md](file-formats.md).

## IDBs

One `.idb` per executable, created on demand as work starts on it. No
cross-IDB symbol sharing in IDA, so a `LEGLIB` routine identified in one
module's context still has to be independently confirmed in
`leglib.idb`. Because `LEGLIB` is shared, though, work done there is the
one place that pays off across every module.

| IDB | Root file | Functions named | Structs | Notes |
|---|---|---|---|---|
| `leglib.idb` | `LEGLIB.EXE` | ~445 / 773 (14 real, rest `rtm_*` provisional) | 0 | 10 segments; `seg003` (53 KB) + `seg004` (18 KB) are the code, `seg007`/`seg008` the `bm*` graphics. Every int-3Fh run-time entry resolved; the 14 hot BASIC-runtime primitives named (`apply_renames_leglib.py`). |
| `menu.idb` | `MENU.EXE` | 25 / 25 seg000 funcs (+ 467 `rt_*` thunks) | 0 | `seg000` coerced + fully named. Layout: `seg000` code, `seg001` thunk table, `seg002` RTM bootstrap, `seg003` DGROUP text, `seg004` stack. |
| `out.idb` | `OUT.EXE` | ~40 / ~95 seg000 funcs (+ `rt_*` thunks) | 0 | Overworld/towns/dungeons engine; chains to `MUS`/`SAVER`/`TWNDR`/`CASDR`/`DUN`. Rebuilt from the UNP-unpacked OUT.EXE (5 clean segments like menu); `seg000` coerced to 100%, 1297 run-time calls resolved. ~40 functions named (`doMovement`, `creatureAttack`, `shopBuy`, `chainTo*`, …) from the decoded screen text (`dump_strings.py`); ~55 helpers still `sub_`. |
| `dun.idb` | `DUN.EXE` | 24 / 72 seg000 funcs (+ `rt_*` thunks) | 0 | Dungeon engine; chains back to `OUT`/`MUS`/`SAVER`. UNP-unpacked; 6 segments — **two** compiled-BASIC code segs: `seg000` "bmDUN" (main) + `seg001` "bmDUNG" (graphics helpers, 9 funcs), thunk table in `seg002`. Both coerced to ~100%. 24 `seg000` functions named from the screen text (`dunMain`, `openChest`, `monsterAttack`, `useMagicMenu`, `castSpell`, `loadDungeonLevel`, …). |

(Counts via `ida_scripts/identify.py -NoExport`; re-run any time as a
sanity check.)

### Segment layout, per module

IDA's MZ loader leaves most segments tagged `'UNK'` even where they hold
code. `menu.idb`:

| Seg | Range | Contents |
|---|---|---|
| `seg000` | `10000`–`13160` | Root menu/intro code (compiled BASIC). Coerced to code + all 25 functions named 2026-08-30. Header (`0`–`0x31`) is the `bmMENU` name + BSS descriptor. |
| `seg001` | `13160`–`138F0` | `int 3Fh` run-time thunk table (467 entries) + a few small resident helpers at the top. Decoded — see "int 3Fh run-time dispatch" below. |
| `seg002` | `138F0`–`13F30` | BC 6.0 EXE bootstrap + RTM loader (`start` at `139CF`). "Error in loading RTM…" strings live here. |
| `seg003` | `13F30`–`1A1B0` | DGROUP data. Text block (menu items, credits, instructions, "poor peasant on the world of Tarmalon…" intro, MML music strings, chained-EXE names) at offset `21D0h`+ (file `0x6ED2`–`0x7F10`). |
| `seg004` | `1A1B0`–`1A9B0` | Stack. |

`out.idb` (rebuilt from the **UNP-unpacked** `OUT.EXE`, 2026-08-30) has
the same 5-segment layout as menu:

| Seg | Range | Contents |
|---|---|---|
| `seg000` | `10000`–`167E0` | BASIC code (26 KB). Coerced to 100%. |
| `seg001` | `167E0`–`16E80` | `int 3Fh` thunk table (467 entries — matches menu's namespace exactly). |
| `seg002` | `16E80`–`174C0` | BC 6.0 EXE bootstrap + RTM loader. |
| `seg003` | `174C0`–`1A690` | DGROUP — screen-string pool (`~0x2150`+, in place) + engine state. |
| `seg004` | `1A690`–`1AE90` | Stack. |

### Packing

`OUT` / `DUN` / `TWNDR` / `CASDR` ship **packed** — a "Packed file is
corrupt" stub, `relocs=0`, entry `:0010`. The code/data payload is
literal but the relocation table is compressed, so `idat -B` loads them
raw with **no relocations applied** and the far-pointer segment words are
wrong (and DGROUP is BSS). Unpack with `UNP.EXE` (via DOSBox) first.
Paul unpacked all four in place in `C:\games\lota` on 2026-08-30
(`OUT.EXE`: 37 KB → 49 KB, 0 → 1512 relocs, entry `:00DF`). Because the
packing is reloc-only, the rebuilt `out.idb` is **byte-stable at the
code EAs**, so the `apply_renames_out.py` entries carried straight over.
`MENU.EXE` was already unpacked (a different packer, unpacked earlier);
`LEGLIB.EXE` is plain (1392 relocs); `CONFIGUR.EXE` still packed (skip —
standalone C util).

Segment names/numbers are **not** renamed or restructured — Paul
correlates them with the DOSBox debugger at runtime (sibling `ultima1`
convention).

## Headless IDA pipeline

Set up 2026-08-30, copied from `ultima1/ida_scripts` (IDA Pro 8.3,
`idat.exe`, no GUI — the GUI locks the `.idb`).

- **`ida_scripts/run_ida_script.ps1`** — entry point.
  ```powershell
  .\run_ida_script.ps1 -Idb leglib -ScriptName identify.py -NoExport
  .\run_ida_script.ps1 -Idb menu   -ScriptName apply_renames_menu.py -NoExport
  ```
  `-Idb` takes a bare stem (`leglib`, `menu`, `out`, …) or an explicit
  path. Refuses to run if the `.idb` is open in the GUI. `-NoExport`
  skips the `.asm`/`.idc` export + `save_database` (for read-only
  scripts).
- **`ida_scripts/batch_run_and_export.py`** — the driver
  (`idat.exe -A -S"batch_run_and_export.py <target.py> [noexport]"`).
  Execs the target script (as if under Alt+F7), then exports
  `<idb-stem>.asm`/`.idc` and saves. Everything — including the target
  script's stdout and any traceback — goes to
  `ida_scripts/batch_run_and_export.log` (idat's own console output in
  `-A` mode isn't reliably flushed).
- **`ida_scripts/identify.py`** — read-only report (root file, hash,
  segments, naming progress, raw-vs-code byte coverage).
- **`ida_scripts/rank_unnamed_functions.py`** — read-only, ranks
  `sub_XXXXX` functions by call-site count.

### Per-module coercion pipeline (generic)

For each client module, in this exact order (all idempotent):

1. **`resolve_thunks.py`** — auto-locate the int-3Fh thunk table (own
   segment or embedded), register the frame selector if embedded, itemise
   + name each thunk `rt_<key>`, comment it with its `rtm_*` target from
   `rtm_map.py`.
2. **`coerce_code.py`** — auto-detect the BASIC code segment + header +
   thunk hole; anchor every `call far`, sweep the gaps, protect
   `$`/NUL-terminated strings, make called thunks returning functions,
   carve + merge functions, fold epilogue stubs, and add a fall-through
   cref past every `call far` (last, so the listing reads continuously).
3. **`resolve_thunks.py`** again — non-destructive; re-asserts the
   `-> rtm_*` comments now that step 2 promoted the hot thunks to
   functions (needs `set_func_cmt` to propagate).
4. **`apply_renames_<module>.py`** — names + repeatable comments only;
   never triggers a reanalysis (that would drop the crefs).

- **`ida_scripts/profile_module.py`** — read-only; per-function callers,
  callees, `rtm_*` calls, and resolved string immediates, to support
  naming.
- `resolve_rtm_leglib.py` / `rtm_map.py` build the shared
  `(prefix,ordinal) → leglib address` map once from `leglib.idb`.

## Conventions (from `ultima1`/`ultima2`)

- **Renames accumulate per-executable** in `apply_renames_<stem>.py`
  (e.g. `apply_renames_leglib.py`), a growing git-diffable list of
  `(ea, new_name, note)` tuples — not a new script per finding. Created
  on demand.
- **Struct edits** go in `apply_structs_<stem>.py` (different IDA API).
- **`DRY_RUN` defaults to `True`** until a module's pass is clearly
  stable.
- **One-off structural scripts** (splitting an array, flattening the
  int 3Fh thunk table, fixing inline-data-after-CALL) get their own
  dedicated script.
- Findings get written up here (or in `file-formats.md` for on-disk
  formats) so the `note` field in a rename entry can stay short.
- **Segments are never renamed** (see above).

## int 3Fh run-time dispatch (decoded 2026-08-30)

Every cross-module call in a client `.EXE` is a `call far` into that
module's thunk segment (`menu.idb`: `seg001`), where each entry is a 3- or
4-byte trampoline:

```
CD 3F nn        bare ordinal nn         (nn = 0x00..0xFD, 254 of them)
CD 3F FF nn     FF-prefixed ordinal nn  (nn = 0x00..0x67)
CD 3F FE nn     FE-prefixed ordinal nn  (nn = 0x00..0x6D)
```

`FE` / `FF` are always prefixes, never bare ordinals. `menu.exe` has 467
thunks total; the `(prefix, ordinal)` namespace is **flat** (no ordinal
reused across a prefix) and **identical across every client module** —
they all link against the same `LEGLIB` — so a name learned in one module
applies everywhere.

`LEGLIB.EXE` installs the `int 3Fh` handler (`leglib.idb` `seg003:7383h`,
set via DOS `int 21h`/`AX=253Fh` from `~seg003:734Bh`). On the **first**
execution of each call site it:

1. reads the ordinal byte(s) after `CD 3F` in the caller;
2. resolves the target:
   - **bare `nn`** → `seg003 : word[seg003:73F6h + 2·nn]`
   - **`FF nn`** → `word[seg003:75F2h + 2·nn]`, in `seg004` when
     `0x19 ≤ nn < 0x62`, else `seg003`
   - **`FE nn`** → full `seg:off` far pointer from the 4-byte entry at
     `seg003:15Ch + 4·nn` (lands in `seg004` / `seg007` / `seg008` — the
     bitmap/graphics segments; `FE` ordinals are the `bm*` graphics calls)
3. **rewrites the caller's `CALL FAR` operands in place** and `retf`s, so
   every call site self-patches to a direct far call after first use.
   Nothing in `LEGLIB`'s own image is modified.

The bare/`FF` tables store offsets only; the handler supplies the segment
as the constant `0x2A9` / `0xF9C` (file value; `+ 0x1000` = IDA
paragraph of `seg003` / `seg004`). The `FE` table holds real relocated
far pointers.

### Tooling

- **`ida_scripts/resolve_rtm_leglib.py`** — reads the three tables in
  `leglib.idb`, turns each of the 468 targets into a named function
  (`rtm_<key>`, e.g. `rtm_C2`, `rtm_FE26`), and writes
  **`ida_scripts/rtm_map.py`** (`(prefix,ordinal) → {ea, seg, name}`).
  First run 2026-08-30: 344 functions created, 434 names, 49 were already
  function heads, 65 land mid-function (shared-tail / multi-entry
  routines — normal for a compiled-BASIC runtime; commented
  `[mid-func: verify]`).
- **`ida_scripts/resolve_thunks.py`** (generic) — see the per-module
  pipeline above. `probe_rtm_tables.py` is the read-only helper this was
  worked out with.

### Provisional names → real BASIC runtime names

`rtm_*` are placeholders. Next step is identifying the actual Microsoft
BASIC 6.0 runtime routines (`B$…`) behind the hot ordinals. From
`menu.idb` call counts:

| key | calls | `leglib` addr | shape (first look) |
|---|---|---|---|
| `rtm_C2` | 171 | `seg003:1B572` | `push bp` frame, 2 stack args, copies a 4-byte descriptor (`[bx]`,`[bx+2]`) — string assignment / `LET`? |
| `rtm_D1` | 149 | `seg003:1B9B0` | — |
| `rtm_FE26` | 98 | graphics seg | bitmap blit (menu draws heavily) |
| `rtm_AF` | 26 | `seg003:13608` | multi-entry (`xor al,al` / `mov al,0FFh` fall-through with `rtm_0F`) |
| `rtm_F0` `rtm_F4` | 21 each | `seg003:1BBA7` / `1BB7C` | adjacent — paired variants |

## Findings log

Decided 2026-08-30 (with Paul): work `LEGLIB.EXE` first (or alongside
`menu`), since it's the shared payload.

- **2026-08-30** — Decoded the `int 3Fh` run-time dispatch (above).
  `leglib.idb`: 7 → ~440 named (all `rtm_*` provisional). `menu.idb`: all
  467 `seg001` thunks named + cross-referenced to `leglib`.
- **2026-08-30** — `menu.idb` `seg000` coerced to code
  (`coerce_code.py`): 99.5% instruction coverage, 0 bad insns, 25
  functions. The `menu.idb` rebuild lost the original `.idb` (recreated
  via `idat -B` from a copy of `MENU.EXE` — input path in the DB now
  reads `C:\dev\lota\menu.exe`).
- **2026-08-30** — Named all 25 `seg000` functions
  (`apply_renames_menu.py`), from the `seg003` text each one prints:

  ```
  menu_main -> mainMenuLoop  (SELECT CASE dispatch, self-looping)
    +- showStartupSplash       the 3-copyright splash (called from menu_main)
    +- drawMainMenuScreen -> drawCancelOption
    +- showQuestCopyright      "...Quest Software, Inc."
    +- showGameCredits         the credits screen
    +- showInstructions        SIMPLE INSTRUCTIONS / COMMANDS / MOVEMENT
    +- showTitleScreen -> loadTitleImage (TITLE.GLB/GMP->B800h), playMusicTick
    +- readLegacyDat
    +- eraseCharacterMenu  -.
    +- startNewGameMenu     +-> showCharacterRoster, showEmptyCharacterSlots,
    +- restartGameMenu     -'   promptCharacterNumber, sub_12055/12778/128A9
         startNewGameMenu -> promptNewCharacterName, playIntroAndLaunchGame
           (the "poor peasant on Tarmalon" intro, then chains to OUT.EXE)
  ```

  6 small CHAR.DAT-record / string-input helpers (`sub_11A15`,
  `sub_11A1E`, `sub_1210E`, `sub_12055`, `sub_12778`, `sub_128A9`) left
  `sub_` — genuinely hard to tell apart.

  Cosmetic: added a fall-through cref past every `call far` so the
  listing reads continuously (IDA's `int 3Fh` overlay special-casing
  otherwise chops a block after each) — 1914 breaks down to 141.

- **2026-08-30** — Generalised the menu-specific scripts into
  `resolve_thunks.py` / `coerce_code.py` / `profile_module.py` (deleted
  `resolve_thunks.py`, `coerce_code.py`, `dump_thunk_table.py`,
  `profile_seg000_menu.py`). Verified menu output unchanged.
- **2026-08-30** — Built `out.idb` (OUT.EXE, overworld engine). First
  from the still-packed `OUT.EXE` — worked well enough to decode the
  thunks / strings / ~40 functions, but the far pointers were
  un-relocated and DGROUP was BSS. Paul then **UNP-unpacked** OUT/DUN/
  TWNDR/CASDR in place; `out.idb` rebuilt from the unpacked `OUT.EXE`:
  5 clean segments like menu, `seg000` **100%** coerced, 0 bad insns,
  ~97 functions, 467 thunks (== menu's namespace), 1297 run-time calls
  resolved. The `apply_renames_out.py` EAs carried over unchanged
  (reloc-only packing → byte-stable code).
- **2026-08-30** — Decoded the screen-string pool format
  ([file-formats.md](file-formats.md#screen-string-pool-in-the-exe-not-a-file--decoded-2026-08-30));
  `dump_strings.py` recovers + annotates it. Named ~14 LEGLIB runtime
  primitives (`basProcEnter`/`Leave`, `basStrAssign`, …,
  `apply_renames_leglib.py`) and ~40 `out` functions (`doMovement`,
  `creatureAttack`, `shopBuy`, `chainTo*`, `museumAccessPrompt`, …).
- **2026-08-30** — Built `dun.idb` (DUN.EXE, dungeon engine) from the
  unpacked exe. 6 segments — **two** compiled-BASIC code segs (`seg000`
  "bmDUN" + `seg001` "bmDUNG"), so `coerce_code.py` gained a
  `$env:COERCE_SEG` override for the 2nd pass. Both ~100% coerced; 24/72
  `seg000` functions named from the screen text (`dunMain`, `openChest`,
  `monsterAttack`, `useMagicMenu`, `castSpell`, `loadDungeonLevel`, …).
- Next: (a) build `twndr.idb` / `casdr.idb` (unpacked, ready — same
  pipeline); (b) map `out`/`dun` `ds:` engine state vars to name the
  remaining helpers; (c) continue the `rtm_*` → `B$…` identification in
  `leglib.idb` (the `FF4B`/`FF20`/… value-stack cluster next).
