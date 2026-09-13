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
- **2026-09-13: both `apply_renames_ultima.py` and `apply_renames_bootup.py`
  are `DRY_RUN = False`** after their respective full function-naming
  batches were verified clean end-to-end (dry-run inspected, addresses
  cross-checked via throwaway lookup scripts, applied, exported,
  re-verified via `identify.py`, 0 failures in both). The `apply_structs_*`
  scripts haven't had this decision made yet; treat as dry-run-first.

## `ULTIMA.COM` (`ultima.idb`) — function-naming sweep: COMPLETE (58/58, 2026-09-13)

Every function is named — see
[overview.md](overview.md#session-2026-09-13-full-function-naming-sweep-5454-named)
for the full findings log. Remaining loose ends:

- [x] `printHexWord`'s orphaned code (no `proc`/`endp` boundary) fixed
      via `ida_funcs.add_func()` — confirmed real, reachable-from-
      BOOTUP.BIN's-identical-copy shared code, not dead weight. See
      `ida_scripts/fix_orphaned_functions.py`.
- [ ] **2 more regions of undefined code** still need `ida_funcs.add_func()`
      before they can be named — both **confirmed real** now (their
      byte-for-byte identical, properly-bounded copies exist and are
      named in `ultima_bootup.idb`), but a first attempt at
      automatically finding their boundaries (a naive "walk backward to
      the nearest `retn`" heuristic) got them wrong — see
      `fix_orphaned_functions.py`'s docstring. Do these by hand next
      time (temporarily open the IDB in the IDA GUI read-only to see
      the boundaries visually, or write a more careful boundary finder
      that handles loops/branches inside the orphan) rather than risk
      corrupting the flow graph with a wrong `add_func` call:
  - **`promptForNumberEntry`** (around `loc_18C39`/`loc_18C7F`, calls
    `accumulateInputDigit`/`readLine`) — matches `ultima_bootup.idb`'s
    `promptForNumberEntry` (0x1498F) exactly, which has a confirmed
    real caller there (`getEntryNumber`, used for every "Entry#"
    prompt). No caller found within `ultima.idb` itself — real shared-
    runtime code this title screen just doesn't happen to use.
  - **`saveFile`** (around `0x18CD9`, calls `openFileWithRetry`,
    mirrors `loadFile` but with `AH=15h` sequential write) — matches
    `ultima_bootup.idb`'s `saveFile` (0x149E0) exactly, which writes
    PARTY.ULT/ROSTER.ULT from several confirmed call sites there. Same
    situation: real code, unused by the title screen.
- [ ] A raw `INT 13h` disk-sector-read routine (~`0x1878C`, reads track
      9 into segment `0xC000` with a retry loop) — no confirmed caller,
      and (unlike the two above) **no matching copy found in
      `ultima_bootup.idb` either** on a quick read-through, so this one
      might genuinely be `ULTIMA.COM`-specific. Given the "Wrong
      Diskette!" prompt theme elsewhere, still plausibly a **copy-
      protection check** (a non-standard/"weak" sector only present on
      the master diskette). Worth checking `EXODUS.BIN` once that's
      disassembled before concluding it's dead code.
  - Also unresolved: a small helper right before `drawTileGrid`
      (~`0x18780`) that swaps 8 words between two rows via a
      `ds:86C9h`-relative address, purpose/caller not identified.
- [ ] `checkDebugModeFlag`, `adjustAnimSpeed`, `computeAnimTableByte`,
      and the 6 `drawTitleBoxN` wrappers' exact visual roles are all
      low-confidence/generic names (see their `note` fields in
      `apply_renames_ultima.py`) — revisit once the game can actually be
      run under an emulator for visual confirmation.
- [ ] Why does `drawAnimatedPixelPath` only consume 533 of `NAME.DAT`'s
      640 bytes? See
      [file-formats.md](file-formats.md#namedat-640-bytes--a-pixel-path-animation-script-not-a-name-table).
- [ ] The `create_strlit` mystery from `fix_wind_string_array.py`: it
      returns `False` for the wind strings in *both* IDBs even after
      `del_items`, forcing a fallback to a plain `FF_BYTE` array via
      `idc.create_data`. Root cause not identified (possibly an IDA 8.3
      API quirk, possibly the leading `0x10` control byte). Not
      blocking — the fallback works — but worth a look if it recurs.
- [ ] Confirm `BLANK.IBM`/`EXOD.IBM`'s exact format — likely full-screen
      CGA/EGA image data (16,384 bytes), analogous to Ultima II's
      `PIC*` files. `loadFile` is identified; tracing its 2 call sites
      for these files plus what reads the loaded buffers afterward
      should settle this quickly.
- [ ] Why does `drawCharGlyph`'s glyph-data buffer read as all zeros in
      `ULTIMA.COM`'s own file image (see overview.md's hard-won-lesson
      section)? `ultima_bootup.idb` has an *identical* zero-filled
      buffer at the same relative offset (`byte_133E9`/`33E9h`-relative,
      the `drawCharGlyph` note in `apply_renames_bootup.py`) — so this
      isn't `ULTIMA.COM`-specific after all, it's a real open question
      about where/when `CHARSET.ULT`'s font data actually gets loaded
      into this shared buffer. `EXODUS.BIN` is now the next suspect,
      since neither title-screen-chain executable populates it before
      needing readable glyphs (both display real text — the wind
      indicator, menu labels — successfully, so it must be populated
      *somehow*, just not found yet).

## `BOOTUP.BIN` (`ultima_bootup.idb`) — function-naming sweep: COMPLETE (73/73, 2026-09-13)

Created and fully swept in the same session as the finding that
prompted it. See
[overview.md](overview.md#session-2026-09-13-bootupbin-created-and-fully-swept-7373-named)
for the full findings log — headline results:

- **Confirms `BOOTUP.BIN` is the character-creation/party-management
  program**, not the world/combat engine — its menus are "Return to the
  View" / "Organize a Party" / "Journey Onward", and "Organize a Party"
  drills into Examine the Register / Create a Character / Form the
  Party / Disperse the Party / Terminate a Character / Look at a
  Character. No overworld movement, combat, or dungeon code found here.
- **`RosterEntry` struct created** (`ida_scripts/create_roster_struct.py`,
  64 bytes) with every field confirmed against real `[bx+N]` accesses
  in `showCharacterDetails`/`handleCreateCharacter` — matches
  `docs/file-formats.md`'s externally-sourced ROSTER.ULT layout exactly
  for every field checked (name, status, 4 attributes, race, class,
  sex, HP/max HP, experience, food, gold, armour index+owned array,
  weapon index+owned array).
- **Confirms the shared-runtime hypothesis completely**: ~40 of
  `BOOTUP.BIN`'s 73 functions are byte-for-byte structurally identical
  to functions already named in `ultima.idb` (text/graphics output,
  file I/O, the boot/idle animation cluster, the sound-effect table,
  the wind-direction display) — same logic, recompiled at different
  addresses. Confirms two `ultima.idb` open questions as real, reachable
  code rather than dead weight (`printHexWord`, `promptForNumberEntry`
  — see above).
- **`EXODUS.BIN` is chained into from `handleJourneyOnward`** via a
  *different* self-modifying-stub mechanism than the `ULTIMA.COM` →
  `BOOTUP.BIN` chain: after the FCB-read loads the whole file over
  `BOOTUP.BIN`'s own code, execution jumps **indirectly** through a
  2-byte vector stored at a fixed offset (`0x1328`, i.e. file offset
  `0x1228`) within `EXODUS.BIN`'s own freshly-loaded bytes, rather than
  just falling through to offset `0x100`. This is now the single
  highest-value next target — see below.

Remaining loose ends specific to this IDB:

- [ ] Apply the `RosterEntry` struct type to the actual `[bx+N]`
      instruction operands (`ida_bytes.op_stroff` per site) so the
      `.asm` shows `player.RosterEntry._hitPoints`-style field names
      instead of raw offsets, matching the sibling projects' `Savegame`
      struct treatment. Not done yet — defining the struct and
      confirming its layout was the priority this pass; wiring it into
      every access site (~15+ sites across 5 functions) is a
      mechanical follow-up.
- [ ] `_maxHitPoints` (offset `0x1C`) is a low-confidence label — only
      confirmed that *a* second HP-shaped word lives there (set
      alongside `_hitPoints` at character creation), not independently
      displayed/read anywhere to confirm the name. See
      `create_roster_struct.py`'s note.
- [ ] The exact letter/index encodings for `_status`/`_race`/`_class`/
      `_sex` are referenced via fixed lookup tables in the disassembly
      (e.g. `byte_11045`/`byte_1106B`/`byte_110D0`) but not yet
      individually decoded value-by-value — worth a dedicated pass
      cross-referencing against `docs/file-formats.md`'s external
      field-name lists (Elf/Dwarf/Fuzzy/etc. for race, Wizard/Ranger/
      etc. for class).

## High value / next session: disassemble `EXODUS.BIN`

**This is now the top priority.** Neither `ULTIMA.COM` nor `BOOTUP.BIN`
contains any overworld movement, combat, or dungeon code — that has to
be in `EXODUS.BIN` (44,234 bytes), reached via `handleJourneyOnward`'s
indirect-jump-through-a-file-offset-vector chain (see above).

- [ ] **Create a third IDB, `ultima_exodus.idb`**, same recipe as
      `ultima_bootup.idb` (copy to a temp `.com` file for IDA's
      auto-detection, since `EXODUS.BIN` is presumably also a raw
      memory image). **Caveat**: confirm the load address assumption
      still holds — `EXODUS.BIN` is loaded via `handleJourneyOnward`'s
      DTA-at-`0x100` trick same as the other two, so it should occupy
      the same `0x10100`+ range, but the *indirect* jump-through-vector
      entry mechanism (unique to this chain-load) means its actual
      entry point is wherever the 2-byte value at file offset `0x1228`
      points, not necessarily offset `0x100` itself — find and follow
      that vector once loaded, don't assume execution starts at the
      segment's first byte the way it does for `ULTIMA.COM`/
      `BOOTUP.BIN`.
- [ ] Cross-reference the shared runtime immediately (same approach
      that worked for `BOOTUP.BIN`): `drawTileGrid`, `playSoundEffect`/
      `SOUND_EFFECT_TABLE`, `loadFile`/`saveFile`, the boot/idle
      animation cluster, `writeCharacter`/`writeString`, etc. are very
      likely present again, recompiled at yet another set of addresses.
- [ ] Identify the top-level command dispatch loop (Ultima III's
      classic single-key command set, per the game manual
      `ULTIMA3.TXT`). Cross-reference LairWare's `UltimaMain.c` main
      loop for the expected command set/shape before assuming Ultima
      II's exact 26-command table transfers over unchanged.
- [ ] Trace the overworld/town/dungeon map file loader — `SOSARIA.ULT`
      etc. (4,648 bytes), dungeon/interior `.ULT` files (2,192 bytes),
      `CNFLCT_*.ULT` combat arenas (176 bytes). `drawTileGrid`'s
      confirmed 64-byte-tile/11×11-grid shape (already named in both
      existing IDBs) is a strong lead for how this renders — the CNFLCT
      arena dimensions match exactly.
- [ ] `SHAPES.ULT`/`CHARSET.ULT` — this is also where the
      `drawCharGlyph`-buffer-is-all-zeros mystery (see `ULTIMA.COM`'s
      open items above) most likely resolves, if `EXODUS.BIN` is what
      actually loads `CHARSET.ULT` into the shared buffer.
- [ ] Confirm/extend the `RosterEntry` struct against whatever
      character-state manipulation `EXODUS.BIN` does during actual
      play (HP loss in combat, gold/food changes, etc.) — this is
      where the fields only inferred so far (armour/weapon owned
      arrays beyond index 0, `_maxHitPoints`) will get real evidence.
- [ ] `EXODUS.BIN`'s own internal data tables (per the external
      documentation in file-formats.md: castle/town/dungeon/moongate
      coordinates, "look" command strings) — cross-reference those
      fixed offsets (`0x15E1`, `0x15E5`, `0x15F9`, `0x184D`, `0x1855`,
      `0x6566`, `0x7445`, `0x7450`) against the disassembly once loaded.
- [ ] `DUNGEON.DAT` (1,866 bytes) / `MOVES.ULT` (1,024 bytes) — smaller,
      less obviously-structured data files, lower priority.

## Open questions

- Exact relationship between this DOS port and the original 1983 Apple
  II release. **Partially resolved this session**: `BOOTUP.BIN`'s intro
  screen credits "(C)-1983 By James R. Van Artsdalen and Lord British"
  — Van Artsdalen is a known name in early Origin Systems DOS ports,
  confirming this codebase's authorship dates to 1983 despite the
  1991-10-01 file dates on disk (presumably just a later recompilation/
  re-release, not a rewrite). Still open: which specific DOS release
  this corresponds to.
- Whether the raw disk-sector reader (flagged above, `ULTIMA.COM`-only
  so far) is reachable from `EXODUS.BIN` or genuinely dead code —
  undetermined until that IDB exists.
