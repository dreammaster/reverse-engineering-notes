# Roadmap

Decided direction (2026-09-02):
- **Target:** DOS "Ultimate Wizardry Archives" version.
- **Fidelity:** p-code disassembly of `SYSTEM.PASCAL` as ground truth, cross-
  checked against the Apple Pascal source in `sources/`.
- **P-machine:** reverse `SYSTEM.INTERP` in Phase 0.
- **End state:** standalone C++ reimplementation in a subfolder, then a ScummVM
  engine. C++ directly (not via transcribed Pascal) — modern C++ expresses
  UCSD's nested procedures better than ISO Pascal would.

## Phase 0 — tooling & p-machine  *(in progress)*

- [x] UCSD p-System volume reader — `tools/ucsd_disk.py` (list / extract / block).
- [x] Extract `WIZ1.DSK` → `extracted/wiz1/`.
- [x] Parse `SYSTEM.PASCAL` segment dictionary (16 named segments).
- [x] Load `SYSTEM.INTERP` into IDA (`ida_scripts/`), analyze:
      - [x] opcode dispatch table (`off_2A1`, 128 entries) — all 128 opcodes
            mapped; standard UCSD p-code confirmed. `ida_scripts/apply_names_interp.py`.
      - [x] SBIOS interface — disk via hooked `INT 18h` (`sbios_disk_io`),
            kbd `INT 16h`, video `INT 10h`.
      - [x] runtime layout — IPC=SI, eval stack=SP, MP=BP, globals `ds:[600h]`,
            heap `ds:[604h]/[602h]`.
      - [ ] `HAS.STROPS` native-call ABI (deferred to Phase 1).
      - [ ] CSP 0, 7–9, 12, 21–24 exact semantics.
- [x] Document the p-machine in `docs/pmachine.md`.

## Phase 1 — data formats

- [x] `SCENARIO.DATA` **container** — TSCNTOC header + per-type record grid,
      fully decoded and self-validating. `tools/scenario.py` (toc/list/rec/dump).
- [~] `SCENARIO.DATA` **records** — field order per Apple structs; DOS bit
      packing for `TMAZE` still to be pinned (Phase 3 vs known maps). `TWIZLONG`
      / `TEXP` confirmed.
- [ ] Keyed string pool — monster/item/spell **names live in `ASCII.KRN`**
      (LZ-compressed), not in the records. Needs the DOS decompressor →
      **blocked on Phase 2**.
- [ ] `200/400.CHARSET` — glyph format (looks like 256 × 32 bytes).
- [ ] `200/400.TITLE` — title bitmap encoding.
- [ ] `200.MONSTERS` — monster portrait display-list opcodes.
- [ ] `HAS.CACHE`, `KANA.KEYMAP` — identify (share a 512-byte prefix).
- [ ] `PLAYER.DATA` / `SAVEn.DSK` — party save format (UCSD volumes too).
- [x] `docs/file-formats.md` started with the above.

## Phase 2 — p-code map

- [x] p-code disassembler `tools/pcode_dis.py` — codefile dir, segment table,
      per-segment proc dictionary + PATs, full opcode/operand decode incl.
      forward/backward/XJP jumps. Clean over all 16 segments / 587 procs.
      `docs/pcode.md`.
- [x] Codefile layout documented; activation-record model from `pm_proc_entry`.
- [x] Validated: DOS `WIZARDRY` procs 2/3/4 = Apple `PRINTBEL`/`GETREC`/`GETRECW`.
- [x] Recover `GetStr(KN)` + the `ASCII.KRN` cipher — `tools/strpool.py`,
      `docs/strings.txt`. WIZARDRY proc 38 = GetStr (range tree), proc 82 =
      loader/decipher, KANJIREA 8/10 = tree loader. Cipher:
      `plain[k] = (raw[k] - 67*(KN mod 51) - 23*k) mod 256`.
      Monster/item names wired into `tools/scenario.py`.
- [ ] Map the ~6 tokenised quest-item names (embedded `0x77` join byte).
- [ ] Name every segment's procs (call-graph + structure match to Apple, not
      just number — DOS numbering drifts above ~proc 15).
- [ ] Constant-pool / global-variable map per segment.

## Phase 3 — standalone C++ engine

- [ ] `engine/` subfolder: VFS over `.DSK`, framebuffer + input platform layer,
      no ScummVM dependency.
- [ ] Port in gameplay order: `ROLLER` → `CASTLE`/`SHOPS` → `RUNNER` (maze + 3D
      view) → `COMBAT` family → `REWARDS` → `SPECIALS` → save/restore.
- [ ] Validate against the real interpreter: same PRNG, same seeded outcomes.

## Phase 4 — ScummVM engine

- [ ] Fold the platform layer onto ScummVM `OSystem`; engine skeleton,
      detection tables for the Archives disk set, metaengine.

## Conventions

- IDA: `idat.exe` headless via `ida_scripts/run_ida_script.ps1`; `.asm`/`.idc`
  are the committed backups, `.idb` is gitignored but kept locally.
- `extracted/` is regenerable and gitignored.
- Commit + push after each documented increment.
