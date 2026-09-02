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
- [~] `200.MONSTERS` — 29 records × 512 B, `TENEMY.PIC` indexed. Load path +
      `CONUNIT` subfn table mapped; payload is compressed (raw geometry = noise),
      decompressor is native code in `SYSTEM.INTERP`. See `docs/file-formats.md`.
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
- [x] Global-variable map — `tools/globals.py`, `docs/globals.md`. Key
      finding: `DOS_word = Apple_word + 289` for word ≥ 363 (CHARACTR /
      SCNTOC / IOCACHE / CHARSET all verified), so the record layouts come
      for free. ~34 globals named; `pcode_dis` annotates LDO/SRO/LAO/SLDO
      (1087 annotations across the listings).
- [ ] Name more procs (auto-matcher + globals context; DOS numbering drifts
      above ~proc 15) and the ~6 tokenised quest-item names (`0x77` byte).

## Phase 3 — standalone C++ engine

- [x] `engine/` skeleton: CMake, `wizcore` static lib (no deps) + `wiz1` CLI.
      Builds with MSVC (`engine/build.ps1`).
- [x] Data layer, verified byte-identical to the Python tools:
      `wiz/ucsd_volume` (mount `.DSK`), `wiz/string_pool` (`ASCII.KRN`),
      `wiz/scenario` (TOC + record grid), `wiz/types` (`WizLong`, enums).
- [x] `ROLLER` rules ported — `engine/wiz/rng.h`, `engine/wiz/roller.h`
      (race base attrs, `classEligibility` = GTCHGLST, bonus/HP/gold/age
      rolls, starting spells).
- [x] `TCHAR` (de)serialiser — `engine/wiz/character.{h,cpp}`. Full field
      word-offset map recovered from the ROLLER p-code; all 20 shipped
      roster records round-trip byte-identically. `wiz1 roster` / `wiz1 roll`.
- [x] `ROLLER` interactive flow — `engine/wiz/roller_ui.{h,cpp}` +
      `wiz/roster.{h,cpp}`. The Training Grounds: create (password ×2, race,
      alignment, live point-allocation with class eligibility, keep Y/N),
      `*ROSTER`, TRAINING (inspect / delete / reroll / set password). Roster
      persisted to `roster.dat` (seeded from the scenario). `wiz1 roller`
      (SDL) / `wiz1 roller-test <keyscript>` (headless, a CMake test).
      Also fixed: `SCENARIO.DATA` STATUS/ALIGN string arrays are 10 B/entry.
- [x] RNG cross-checked against SYSTEM.INTERP — RANDOM = `UNITREAD(unit 13,
      subfn 10)` → generator at `0x221E`. Found a **shipped bug**: it mixes 4
      LCG states in BX but returns AX = `byteswap(s3) & 0x700F` (128 values,
      period 65536 — the weak PC-Wizardry RNG). `engine/wiz/rng.h` reproduces
      it exactly (matches a byte-accurate sim). A live sequence diff vs the
      real interpreter would be the final confirmation.
- [ ] Record field structs (`TMAZE`, `TENEMY`, `TOBJREC`, `TCHAR`) per the
      +289 layout; maze bit-packing validated vs a known map.
- [x] Platform layer: `engine/wiz/surface.h` (8bpp framebuffer + primitives),
      `wiz/font.h` (`*.CHARSET` = 16×8, 512 glyphs), `wiz/platform.h` abstract
      + `NullPlatform` (PPM dumps) + `SdlPlatform` (SDL2 window).
- [x] Text-grid layer: `engine/wiz/textscreen.h` — 40×24 cell grid, windows
      (`setWindow`, clamped GOTOXY), UCSD control codes (CHR 12/11/29/8/13/10),
      `CLRRECT`, Pascal `': w'` right-justify, inverse video, font render.
      `wiz1 mockup` renders the MAKEMENU screen.
- [x] `200/400.TITLE` decoded — 320×64 2bpp CGA (the "Wizardry" logo).
      `engine/wiz/bitmap.h` (`loadCga2bpp`/`loadTitle`); `wiz1 show title`;
      shown at boot by `wiz1 roller`.
- [~] `200.MONSTERS` — grid + load path mapped; compressed payload TBD (needs
      `SYSTEM.INTERP` console-driver RE). Doesn't block the town/maze work.
- [~] `CASTLE`/`SHOPS` — the town. `docs/town.md` maps the `XGOTO` state
      machine + both segments. Ported so far: the Castle hub menu, Gilgamesh's
      Tavern (add / remove / see a member, alignment + password gates), the
      **Adventurer's Inn** (rest / heal / age / spell-refill / level-up chain:
      `CHNEWLEV`/`MADELEV`/`MOREHP`/`TRYLEARN`/`GAINLOST` — `engine/wiz/inn.h`),
      **Boltac's Trading Post** (buy / sell / uncurse / identify; per-object
      stock in `shop.dat`; `TOBJREC` decoded — `engine/wiz/shop.h`), the
      **Temple of Radiant Cant** (cure / resurrect roster characters, paid by
      a party member; `engine/wiz/temple.h`), and the Edge of Town.
      `engine/wiz/party.{h,cpp}` (`Party` = 6 slots + roster indices +
      `GETALIGN` + `resyncFromRoster`), `engine/wiz/town_ui.{h,cpp}`
      (`runTown(Ui&, TownWorld&)` → `TownExit`). `wiz1 town` (SDL) /
      `wiz1 town-test` + `wiz1 inn-test` + `wiz1 temple-test` (headless, CMake
      tests). **The town is complete.**
- [~] `RUNNER` — the maze. `docs/maze.md` maps the whole segment. The
      **`TMAZE` bit packing is cracked** (`docs/file-formats.md`) and validated
      by rendering level 1. Ported: `engine/wiz/maze.h`/`.cpp` (`MazeLevel`
      reader) + `engine/wiz/runner.h` (`MazePos`, `SHFTPOS`, `FORWRD`/`KICK`/
      `DOTURN`, the FRWD/LEFT/RIGH view helpers). `wiz1 maze <SCENARIO.DATA>
      [level] [F/L/R/K script]` walks a level with a top-down ASCII view
      (CMake test `maze_walk`). TODO: `DRAWMAZE` — the first-person wireframe.
- [ ] Continue: `RUNNER` `DRAWMAZE` 3D view → `SPECSQAR` traps/stairs →
      `COMBAT` family → `REWARDS` → `SPECIALS` → save/restore.
- [ ] Validate against the real interpreter: same PRNG, same seeded outcomes.

## Phase 4 — ScummVM engine

- [ ] Fold the platform layer onto ScummVM `OSystem`; engine skeleton,
      detection tables for the Archives disk set, metaengine.

## Conventions

- IDA: `idat.exe` headless via `ida_scripts/run_ida_script.ps1`; `.asm`/`.idc`
  are the committed backups, `.idb` is gitignored but kept locally.
- `extracted/` is regenerable and gitignored.
- Commit + push after each documented increment.
