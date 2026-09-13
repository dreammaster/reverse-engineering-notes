# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current
across sessions, unlike a one-off todo list.

## Decisions made

- **2026-09-13 (Paul's call): function naming is camelCase**, matching
  `ultima1` rather than `ultima2` (snake_case), despite this project's
  single-executable/single-IDB shape otherwise being closer to
  `ultima2`'s project template. See
  [overview.md](overview.md#naming-convention).
- **2026-09-13 (Paul's call): `source/` (the LairWare Mac remake) is
  untracked** from this git repo — kept on disk locally as a reference,
  `.gitignore`d, matching `ultima1`'s `ultima_old/` precedent for large
  reference-only trees that aren't part of the RE-notes history.
- **2026-09-13: `apply_renames.py` flipped to `DRY_RUN = False`** after
  the first full 54-function batch was verified clean end-to-end
  (dry-run inspected, addresses cross-checked, applied, exported,
  re-verified via `identify.py` showing 57/57 named, 0 failures). Kept
  `False` going forward per the ultima2 precedent — the pattern is now
  trusted. `apply_structs.py` hasn't had this decision made yet (no
  structs defined so far); treat it as dry-run-first until then.

## `ULTIMA.COM` — function-naming sweep: COMPLETE (57/57, 2026-09-13)

Every function in this IDB is now named — see
[overview.md](overview.md#session-2026-09-13-full-function-naming-sweep-5454-named)
for the full findings log. Remaining loose ends specific to this
executable:

- [ ] **4 regions of undefined code need `ida_funcs.add_func()` before
      they can be named** — IDA never recognized them as functions at
      all (no `proc`/`endp`), so `apply_renames.py`'s `idc.set_name`
      can't reach them yet:
  - A numeric hex/decimal-entry prompt (~`0x18C79`) that calls the
    already-named `accumulateInputDigit`/`readLine`/`printHexByte`/
    `printHexNibble` — no confirmed caller within this file. Possibly a
    copy-protection code-entry prompt (a common Origin Systems-era
    anti-piracy technique), possibly dead/leftover library code. Worth
    checking whether `BOOTUP.BIN` calls into it once that's
    disassembled, before spending more time guessing here.
  - A raw `INT 13h` disk-sector-read routine (~`0x1878C`, reads track
    9 into segment `0xC000` with a retry loop) — no confirmed caller
    either. Given the "Wrong Diskette!" prompt theme elsewhere in this
    file, plausibly a **copy-protection check** (reading a
    non-standard/"weak" sector that only exists on the master
    diskette) rather than normal file I/O, which always goes through
    `loadFile`/FCB calls instead of raw `INT 13h`. Flagged as a
    hypothesis, not confirmed.
  - A small helper right before `drawTileGrid`/`sub_18808` (~`0x18780`)
    that swaps 8 words between two rows via a `ds:86C9h`-relative
    address — purpose and caller not identified.
  - An FCB-based file-write routine (~`0x18CD9`, mirrors `loadFile` but
    with `AH=15h` sequential write) — this is presumably the savegame
    writer, but nothing in `ULTIMA.COM` itself calls it (plausible:
    only `BOOTUP.BIN` saves games). Once found, name it to match
    `loadFile`'s role (`saveFile`?).
- [ ] `checkDebugModeFlag`, `adjustAnimSpeed`, `computeAnimTableByte`,
      and the 6 `drawTitleBoxN` wrappers' exact visual roles are all
      low-confidence/generic names (see their `note` fields in
      `apply_renames.py`) — revisit if `BOOTUP.BIN`'s disassembly or
      actually running the game under an emulator clarifies them.
- [ ] Why does `drawAnimatedPixelPath` only consume 533 of `NAME.DAT`'s
      640 bytes? What are the remaining 107 bytes for? See
      [file-formats.md](file-formats.md#namedat-640-bytes--a-pixel-path-animation-script-not-a-name-table).
- [ ] The `create_strlit` mystery from `fix_wind_string_array.py`: it
      returned `False` for all 5 wind strings even after `del_items`,
      forcing a fallback to a plain `FF_BYTE` array via
      `idc.create_data`. Root cause not identified (possibly IDA 8.3
      API quirk, possibly something about the leading `0x10`
      control-byte). Not blocking (the fallback works fine and the
      names took), but worth a quick look if it recurs elsewhere.
- [ ] Confirm `BLANK.IBM`/`EXOD.IBM`'s exact format — likely full-screen
      CGA/EGA image data (16,384 bytes matches a plausible full-screen
      bitmap size), analogous to Ultima II's `PIC*` files. `loadFile`
      is now identified and named; tracing its 2 call sites for these
      files plus what reads the loaded buffers afterward should settle
      this quickly.

## High value / next session: disassemble `BOOTUP.BIN`

**This is now the top priority.** Per
[overview.md](overview.md#ultimacoms-real-role-a-title-screen-loader-not-the-game),
`ULTIMA.COM` is confirmed to be only a title-screen loader that chains
into `BOOTUP.BIN` — the actual game (character creation, main loop,
combat, everything) almost certainly lives there, undisassembled, no
IDB yet.

- [ ] **Create a second IDB for `BOOTUP.BIN`.** This makes ultima3's
      project shape closer to `ultima1`'s multi-executable pattern than
      `ultima2`'s single-IDB one, despite the initial scaffolding
      assuming the latter (a reasonable call at the time, before this
      finding) — the `ida_scripts/` driver may need generalizing to
      take an `-Idb`-style parameter like `ultima1`'s, rather than
      staying hardcoded to `ultima.idb`, once a second database exists.
      `BOOTUP.BIN` is a raw memory image (no `MZ`/EXE header, loaded
      directly at paragraph `0x1000` offset `0x100` by the FCB-read
      trick — same addressing as `ULTIMA.COM`), so creating its IDB
      will need the same tiny-model/base-address setup as `ultima.idb`
      rather than relying on IDA's automatic EXE-format detection.
- [ ] Once loaded, cross-reference the shared runtime immediately:
      `drawTileGrid`, `playSoundEffect`/`SOUND_EFFECT_TABLE`, and the
      other primitives named in `ULTIMA.COM` are very likely called
      from `BOOTUP.BIN` too (same binary layout, same absolute
      addresses, since it overwrites `ULTIMA.COM`'s memory in place) —
      confirm this and pull over the same names via BinDiff or direct
      address matching before re-deriving them from scratch.
- [ ] Identify the top-level command dispatch loop (Ultima III's
      classic single-key command set — likely similar A-Z letter
      commands to Ultima I/II, per the game manual `ULTIMA3.TXT`).
      Cross-reference against LairWare's `UltimaMain.c` main loop for
      the expected command set/shape before assuming Ultima II's exact
      26-command table transfers over unchanged.
- [ ] Trace the overworld/town/dungeon map file loader — the `.ULT`
      files in `C:\games\ultima3` are a strong, concrete target
      (`SOSARIA.ULT` etc. at 4,648 bytes, dungeon/interior `.ULT` files
      at 2,192 bytes, `CNFLCT_*.ULT` combat arenas at 176 bytes) with
      obvious fixed-size-record structure to reverse from both ends
      (file layout + the code that reads them). See
      [file-formats.md](file-formats.md) for size tables to start from.
      `drawTileGrid`'s confirmed 64-byte-tile/11×11-grid shape (already
      named in `ULTIMA.COM`) is a strong lead for how `BOOTUP.BIN`
      renders these.
- [ ] Trace `ROSTER.ULT`/`PARTY.ULT` (save data, 1,280/274 bytes) —
      character record layout, party state. External documentation
      (see file-formats.md) already gives a detailed byte-level
      hypothesis for both; the job is confirming it against
      `BOOTUP.BIN`'s actual read/write code, not re-deriving from
      scratch. Compare against LairWare's `Player[21][65]`/`Party[64]`
      shapes mentioned in its source (Apple II original sizes — expect
      the DOS port's byte layout to differ, but the *field list* is a
      strong hint of what to look for).
- [ ] `SHAPES.ULT` (5,120 bytes) / `CHARSET.ULT` (2,048 bytes) — tile
      graphics and font. External documentation (file-formats.md)
      already gives a detailed byte-level hypothesis (80 tiles × 64
      bytes CGA-2bpp; 128 chars × 16 bytes). `ULTIMA.COM`'s own
      `drawCharGlyph` reads glyph data from a buffer that's
      **unexpectedly all zeros** in `ULTIMA.COM`'s own file image (see
      overview.md's hard-won-lesson section and drawCharGlyph's note in
      apply_renames.py) — chasing where `BOOTUP.BIN` populates that
      buffer (presumably loading `CHARSET.ULT` into it) would resolve
      this open question.
- [ ] `DUNGEON.DAT` (1,866 bytes) / `MOVES.ULT` (1,024 bytes) — smaller,
      less obviously-structured data files, lower priority until the
      bigger map/save formats are settled. (`ANIMATE.DAT`'s consumer is
      now identified in `ULTIMA.COM` itself — see file-formats.md.)
- [ ] Define the core `Savegame`/player-state struct once enough
      fields are traced (mirrors `ultima1`/`ultima2`'s `Savegame`
      struct) — don't force this early; wait for real evidence per
      field like the sibling projects did. This will live in
      `BOOTUP.BIN`'s IDB/`apply_structs.py`, not `ULTIMA.COM`'s (this
      title-screen program has no player-state fields at all).
- [ ] Determine `EXODUS.BIN`'s (44,234 bytes) real nature — separate
      chained executable, or pure data blob read by `BOOTUP.BIN`? Check
      whether it starts with `MZ`. External documentation (see
      file-formats.md) suggests fixed data tables (castle/town/dungeon/
      moongate coordinates, "look" command strings) rather than code,
      but that's not yet confirmed against the file's own bytes.

## Open questions

- Exact relationship between this DOS port (files dated 1991-10-01 for
  the 3 largest binaries, but most `.ULT`/`.DAT` data files dated
  1983-1987) and the original 1983 Apple II release — is this a
  straight port of an earlier DOS release, or the "gold" 1991
  re-release? Worth checking `ULTIMA3.TXT`'s copyright text and any
  version strings found during disassembly.
- Whether the numeric hex/decimal-entry prompt and the raw disk-sector
  reader (both flagged above) are reachable from anywhere, or dead
  leftover library code — undetermined until `BOOTUP.BIN` is
  disassembled or the game is run under an emulator to observe actual
  behavior.
