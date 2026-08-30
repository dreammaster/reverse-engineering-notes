# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current across
sessions, unlike a one-off todo list. See [overview.md](overview.md) for
the per-executable breakdown this tracks against.

## Infra (done, 2026-08-30)

- [x] Set up headless IDA pipeline
      (`ida_scripts/run_ida_script.ps1` + `batch_run_and_export.py`),
      copied from the sibling `ultima1` project. Path-derivation driver
      (works for any `<stem>.idb`), pre-flight GUI-lock check.
- [x] `ida_scripts/identify.py` (+ raw-vs-code byte coverage) and
      `rank_unnamed_functions.py`. Confirmed working end-to-end against
      `menu.idb` and `leglib.idb` (report mode).
- [x] Identified the toolchain: **Microsoft BASIC Compiler 6.0**, with
      `LEGLIB.EXE` as a shared BRUN-style run-time module that every
      other module loads via an `int 3Fh` thunk table. Not C.
- [x] Confirmed `.BSV`/`.GLB`/`.GMP`/`.BS1`/`.BS2` are Microsoft BASIC
      `BSAVE` images (`0xFD` + seg + off + len header) — see
      [file-formats.md](file-formats.md).
- [x] `.gitignore` already correct (commits `.asm`/`.idc`, ignores
      `.idb` and unpacked-db files).

## Executable order

Decided 2026-08-30 (with Paul): **`LEGLIB.EXE` first**, or alongside
`MENU.EXE`. Rationale: `LEGLIB` is the shared payload — the BASIC runtime
plus the common engine (`bmXXXX` graphics, file I/O, input). Every
per-module `.EXE` is a thin `call far` stream into it, so nothing
downstream is readable until `LEGLIB`'s entry points are mapped. `MENU`
is small and makes a good pipeline shakedown / first consumer to
validate the thunk-table → `LEGLIB` cross-reference.

Rough size order of the rest (biggest engine payoff first): `OUT` →
`DUN` → `TWNDR` → `CASDR` → `MUS` → minigames (`SDEFENDR`, `GMB1`,
`GMB2`) → drivers (`CELDRV`, `STDRV`, `SAVER`) → `CONFIGUR` (standalone,
not BASIC).

## LEGLIB.EXE — open questions

- [x] Map the `int 3Fh` thunk-table entry format and build a resolver
      (2026-08-30). `resolve_rtm_leglib.py` + `rtm_map.py`;
      `resolve_thunks_menu.py` on the client side. Mechanism written up in
      [overview.md](overview.md#int-3fh-run-time-dispatch-decoded-2026-08-30).
- [ ] Attach real names to `rtm_*`. Rank by cross-module call frequency
      (once `out`/`dun` IDBs exist), identify the Microsoft BASIC 6.0
      runtime routines (`B$…`) against QuickBASIC 4.5 / BASCOM 6 / BASIC
      PDS 7 references + QB reversing notes. Start with `rtm_C2` (171
      calls in `menu` alone), `rtm_D1`, `rtm_AF`, the `rtm_FE*` graphics
      calls.
- [ ] Verify the 65 `[mid-func: verify]` `rtm_*` entries — confirm
      they're genuine shared-tail / multi-entry routines, not resolution
      errors.
- [ ] Separate stock BASIC runtime from LotA-specific engine code in
      `seg007`/`seg008` (the `bm*` graphics — `bmCOMBLIB`, `bmTCASANIM`).
- [ ] Find and name the `bmXXXX` bitmap/graphics primitives (the
      `bmMENU`, `bmREADY`, … names are already visible as strings).
- [ ] Find the `BLOAD`/`BSAVE` implementation and the file-I/O layer —
      the key to decoding every `.BSV`/`.GLB` file.
- [ ] Input handling (keyboard poll used by the menu state machine).
- [ ] `seg003` (53 KB) vs `seg004` (18 KB): which is runtime, which is
      game engine?
- [ ] Confirm graphics mode support (CGA / EGA / Tandy — `CONFIGUR.EXE`
      writes `DRCONFIG.DAT`; the `*DRV.EXE` files are hardware drivers).

## MENU.EXE — open questions

- [x] Name all 467 `seg001` thunks + cross-reference to `leglib`
      (2026-08-30, `resolve_thunks_menu.py`).
- [x] Force `seg000` to code (2026-08-30, `coerce_seg000_menu.py`):
      99.5%, 0 bad insns, 25 functions, full call graph.
- [ ] Name the 25 `seg000` functions. `menu_main` → `sub_10580`
      (main-menu dispatch) → option handlers; `sub_10738` reads the
      GAME CREDITS text (loops over `seg003` offsets).
- [ ] Cosmetic: IDA's `int 3Fh` overlay special-casing chops blocks
      after every `call far` and mislabels some `noreturn`. Find the
      analysis/loader switch to disable it, or post-process.
- [ ] `menu.idb` input path reads `C:\dev\lota\menu.exe` (a copy; the
      original `.idb` was lost and rebuilt via `idat -B`). Harmless, but
      re-point at `C:\games\lota\MENU.EXE` if a full rebuild is ever
      needed.
- [ ] Mark up the `seg003:21D0h`+ text block as strings.
- [ ] Walk the menu state machine (main menu → play / instructions /
      credits / sound toggle; character management screens).
- [ ] Confirm whether `MENU.EXE` was unpacked before import (oversized
      header, entry `038F:00DF`), and whether the EA "Installation
      Program" is separable from the Quest menu program.
- [ ] Trace the chain-out to `OUT.EXE` / `MUS.EXE` and how the selected
      character is handed over (`CHAR.DAT`? `LEGACY.DAT`?).

## Data formats

- [ ] Field-level decode of `LEGACY.DAT` (no BSAVE header — the odd one
      out; likely game progress / roster / config).
- [ ] `CHAR.DAT` (3444 bytes) — character roster the menu edits.
- [ ] Overworld map (`OUTDATA.BSV` / `OUTM*.BSV`), once `OUT.EXE`'s
      `BLOAD` sites are traced.
- [ ] Town / castle / dungeon layouts.
- [ ] `TITLE.GLB` / `TITLE.GMP` (title-screen graphics) — good first
      target since `MENU` loads them early.
- [ ] Graphics pixel packing (CGA/EGA) and the `.GLB` palette/tile
      layout.
- [ ] Music format (`MUSDATA.BSV` + the MML strings in `MENU`).

## ScummVM engine (future)

Not started. Same end goal as `ultima1`/`ultima2`: once the modules are
documented, a clean C++ reimplementation, then a ScummVM engine module.
