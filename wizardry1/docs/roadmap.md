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
- [ ] Load `SYSTEM.INTERP` into IDA (`ida_scripts/`), analyze:
      - opcode dispatch table → exact p-code dialect
      - SBIOS / native call interface, `HAS.STROPS` linkage
      - runtime layout (E-reg, stack, segment table, KP/SP/IPC)
- [ ] Document the p-machine in `docs/pmachine.md`.

## Phase 1 — data formats

- [ ] `SCENARIO.DATA`: title, header table, then MAZE / MONSTER / OBJECT /
      REWARD / IMAGE / SPELL / MESSAGE record arrays. Map against the Apple
      `TMAZEREC` / `TOBJREC` / monster records in `Wiz1WizardryPascal.txt`.
- [ ] `200/400.CHARSET` — glyph format (looks like 256 × 32 bytes).
- [ ] `200/400.TITLE` — title bitmap encoding.
- [ ] `200.MONSTERS` — monster portrait display-list opcodes.
- [ ] `ASCII.KRN`, `HAS.CACHE`, `KANA.KEYMAP` — identify.
- [ ] `PLAYER.DATA` / `SAVEn.DSK` — party save format (UCSD volumes too).
- [ ] Write `docs/file-formats.md` with byte-level layouts + a validating
      extractor/renderer for each.

## Phase 2 — p-code map

- [ ] p-code disassembler (`tools/pcode_dis.py`) for `SYSTEM.PASCAL` segments,
      dialect per Phase 0.
- [ ] Per-segment: recover procedure boundaries, jump tables, constant pools;
      align each procedure to its Apple Pascal counterpart; log divergences.
- [ ] Annotated listing checked into `docs/pcode/`.

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
