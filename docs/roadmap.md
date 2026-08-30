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
      `resolve_thunks.py` on the client side. Mechanism written up in
      [overview.md](overview.md#int-3fh-run-time-dispatch-decoded-2026-08-30).
- [x] Extended the FE table to `FE70` (2026-08-30) — `out.exe` uses
      `FE6E`–`FE70`; `FE71`+ is past the table.
- [~] Attach real names to `rtm_*` (`apply_renames_leglib.py`). Done: 14
      hot ones — `basProcEnter`/`basProcLeave` (SUB frame enter/leave, cx
      = frame size), `basProcExit1`/`basProcExit2` (outer exit wrapper),
      `basStrAssign` (164x in out), `basStrConcat`, `basStrClear`,
      `basStrBuild`, `basArrayCopy`, `basPlayMusic`, `basScreenInit`
      (provisional), `drawString`/`drawStringInner`/`screenRefresh`
      (engine text). Continue with the FF-cluster `out` leans on
      (`FF4B` 154x, `FF20`, `FF1F`, `FF44`, `FF4E`, `FF50` — value/screen
      stack ops around `ds:111Ch`).
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
      (2026-08-30, `resolve_thunks.py`).
- [x] Force `seg000` to code (2026-08-30, `coerce_code.py`):
      99.5%, 0 bad insns, 25 functions, full call graph.
- [x] Name the 25 `seg000` functions (2026-08-30, `apply_renames_menu.py`).
      6 CHAR.DAT-record helpers left `sub_` (hard to distinguish).
- [x] Cosmetic block-chopping: post-process fixed via fall-through crefs
      past each `call far` (`apply_renames_menu.py`, final step). 1914 → 141.
- [ ] Identify the 6 remaining `sub_` helpers (`sub_11A15` 8-byte shared
      wrapper, `sub_11A1E`/`sub_1210E` self-callers, `sub_12055`/`12778`/
      `128A9` near-clone CHAR.DAT enumerators).
- [ ] Confirm `playMusicTick` / `showTitleScreen` naming (the LEGACY.DAT
      touch in `playMusicTick` is unexplained).
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

## OUT.EXE — open questions

- [x] Build `out.idb` (2026-08-30). `seg000` BASIC code coerced to
      99.7%, 0 bad insns, ~97 functions, 1308 run-time calls resolved to
      `rt_*`. Thunk table embedded mid-`seg000`; frame selector `0x67E`
      registered (no segment carve).
- [~] Name the `seg000` functions. Done so far (`apply_renames_out.py`):
      `out_entry` → `outInit` → `mainDispatch` (3.8 KB central loop),
      `updateGameState` (dispatch on `ds:1F2Ah`), the `setFlag_*` /
      `setMode_*` / `applyGameFlag` families. ~80 helpers still `sub_` —
      OUT's DGROUP text is position-coded (no anchors), so each needs
      tracing its `ds:` state vars + `rtm_*` pattern. Recurring shapes
      catalogued in the `apply_renames_out.py` header.
- [x] Fixed the call-far fragmentation merge (2026-08-30) — was
      orphaning ~3.8 KB (`mainDispatch` came out 15 bytes). Now merges
      only truly-adjacent fragments + re-sweeps; 1 unowned byte left.
- [ ] Map the `ds:21XXh` / `ds:1F0Xh` / `ds:1F2Ah` / `ds:2234h` engine
      state variables — the key to naming the helper cloud.
- [ ] Decode the position-coded DGROUP string format (leading
      byte(s) = screen position/attr, `%` etc = control codes).
- [ ] The post-thunk RTM-loader stub (`seg000:16E8C`+) is left unswept
      (`$`-terminated DOS strings + boilerplate) — disassemble if needed.
- [ ] Trace the `BLOAD` sites for `OUTDATA.BSV` / `OUTM*.BSV` / `OUTOBJ.BSV`.

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
