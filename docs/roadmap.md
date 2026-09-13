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

## `EXODUS.BIN` (`ultima_exodus.idb`) — shared runtime named (47/142, 2026-09-13); game logic pending

**This is now the top priority and the largest remaining phase of the
whole project** — comparable in scope to `ultima1`'s or `ultima2`'s
entire sibling efforts. Created 2026-09-13 (same recipe as
`ultima_bootup.idb`: copy to a temp `.com` file for IDA's loader
auto-detection). 144 functions total (142 after `entryFromBootup`'s
manual `add_func` absorbed 2 stray auto-detected fragments), ~14,600
`.asm` lines. The entire shared low-level runtime (~40 functions:
`writeString`/`writeCharacter`/`drawCharGlyph`/`drawTileGrid`/
`readLine`/`loadFile`/`saveFile`/`openFileWithRetry`/the boot-animation
cluster/the wind display/the 12-entry sound table) is named — see
[overview.md](overview.md#session-2026-09-13-ultima_exodusidb-created-shared-runtime-named-47142)
for the full findings log. **95 functions remain, all game-specific.**

Confirmed via a string-table scan (not yet via reading the actual
disassembly) that this executable holds: overworld/town/dungeon
movement, a large single-key command dispatcher (`sub_17B54` — spans
thousands of bytes, likely THE overworld command loop, analogous to
ultima1/ultima2's A-Z dispatcher), a separate combat dispatcher
(`sub_123A5`), spellcasting (`sub_15D83`), shops (`sub_1A630`), temples
(`sub_1A692`), and the game's ending sequence.

Next-session priorities, roughly in order:

- [x] Investigated `sub_123A5` — **turned out to be `updateMonsterAI`
      (per-turn monster/NPC movement AI), not the combat dispatcher.**
      The "Attack"/"Cast Spell"/"Ztats"/"Pass" strings that pointed
      here were a false lead: they actually belong to a separate code
      chunk at absolute address **`~0x18D0B`**, ~0x6500 bytes away, that
      IDA had merged into `sub_123A5`'s function-chunk list (a real IDA
      behavior for disjoint/cold-path chunks). Confirmed a new
      `Monster` parallel-array layout in the process: `+0x1280` type,
      `+0x12A0` display tile, `+0x12C0` X, `+0x12E0` Y, 32 slots — same
      convention as `ultima2`'s monster tracking. Two supporting
      helpers named at low/medium confidence: `getMapTileAt` (returns
      a tile *value*, not a pointer, from a packed coordinate — exact
      bit-level derivation not fully worked out) and `canMoveToTile`
      (movement-legality check, exact tile-code meanings unconfirmed).
- [x] **Combat command dispatcher** — done. Was reachable as a
      function chunk of `updateMonsterAI`, which turned out to be
      correct/intentional (not a merge bug — see overview.md). Full
      33-entry `COMBAT_COMMAND_TABLE` identified with 8 commands named:
      `combatCmdPass`/`combatCmdReady`/`combatCmdZtats`/
      `combatCmdNegateTime`/`combatCmdCastSpell`/`combatCmdAttack`/
      `combatHandleMovement`/`combatCmdInvalid`.
- [x] **`castSpell`** — done, class-gating and MP-cost/dispatch
      mechanism fully traced. `WIZARD_SPELL_TABLE`/`CLERIC_SPELL_TABLE`
      entries (the individual spell effects) are the natural
      continuation — see below.
- [x] **The overworld main game loop and its 33-entry command table**
      — found (`mainGameLoop`, `OVERWORLD_COMMAND_TABLE`/
      `OVERWORLD_COMMAND_KEYS`). 16 of 33 commands confirmed so far
      (movement x4, Pass, Board, Exit vehicle, Toggle sound, Enter
      (dungeon/town/castle/shrine), Cast Spell, Exchange, Peer, Quit,
      Steal, Unlock — see overview.md). **17 remain**, each a small,
      self-contained, well-bounded target — far more tractable now
      than reading `sub_17B54` linearly, since every handler's address
      and trigger key are already known:

      | Key | Handler address | Key | Handler address |
      |---|---|---|---|
      | A | `loc_1888B` | N | `loc_15CF8` |
      | F | `loc_15BCF` | O | `loc_174D5` |
      | G | `loc_18190` | R | `loc_17E33` |
      | H | `loc_11E55` (partially read, see below) | T | `loc_17FC6` |
      | J | `loc_15C73` | W | `loc_17EE4` |
      | L | `loc_11E7A` | Y | `loc_17458` |
      | | | Z | `loc_12068` |

      H (`loc_11E55`) is partially read: prompts "To Player: ", involves
      `sub_16C76` (a player-selection prompt, also used by `cmdCastSpell`)
      and a recursive self-call into `sub_17B54` — purpose not pinned
      down, flagged rather than guessed.
- [x] `cmdDisabledOnSurface` (D and K, both `loc_15CC3` — genuinely
      shared, confirmed via `ida_bytes.get_word()` against
      `OVERWORLD_COMMAND_TABLE`) — both are no-ops on the overworld,
      consistent with Descend/Klimb being dungeon-only commands. The
      likely real in-dungeon command set is reached through a
      **different** jump table, `jpt_18389`, referenced repeatedly
      across this session's other finds (case numbers cited alongside
      `OVERWORLD_COMMAND_TABLE`'s in several handlers' comments) but
      never itself identified — **worth dedicated attention next**,
      since it's plausibly where Descend/Klimb and other dungeon-only
      commands actually live.
- [x] `cmdIgniteTorch` ('I') — confirms `_torches` (`RosterEntry`
      `+0x0F`) independently of external documentation.
- [ ] Confirm whether `_locationTypeTable` (`byte_1259D`) is really a
      scalar or (more likely, given it's indexed alongside the
      19-entry `LOCATION_TILE_TABLE`) a 19-byte parallel array —
      applied at low-medium confidence this pass, worth a quick
      `ida_bytes.get_word()`-style direct check against the actual
      indexed access pattern before trusting it as-is.
- [ ] Individual spell effects in `WIZARD_SPELL_TABLE`/
      `CLERIC_SPELL_TABLE` — now that `castSpell`'s dispatch mechanism
      is understood, each entry is a small, self-contained function.
      Cross-reference `ULTIMA3.TXT`'s spell list and LairWare's
      `UltimaSpellCombat.c` for expected spell names/effects.
- [ ] `combatCmdAttack`'s damage-resolution helpers (`sub_18E46`,
      `sub_18E7A`) — the actual to-hit/damage formula, not yet traced
      past "the overall shape."
- [ ] Resolve the **Level vs. Experience offset conflict**: `drawPartyStatusBar`
      (`ultima_exodus.idb`) reads `+0x1Fh` alone as a "Level" byte
      (BCD-displayed as value+1, clamped to 99), but
      `ultima_bootup.idb`'s `showCharacterDetails` reads `+0x1Eh` as a
      **2-byte word** for "Experience:" — these can't both be literally
      true of the same on-disk record. Leading hypothesis, not yet
      confirmed: the in-memory "live" combat/play copy of a character
      record (the `[bx*40h+14CCh]`-style arrays) may cache a derived
      Level byte at an offset the true ROSTER.ULT save-file format
      doesn't have, overwriting what would otherwise be Experience's
      high byte, with Level recomputed from Experience (and not
      persisted) each time a fresh copy is loaded from the roster.
      Confirm by finding wherever a live combat record gets populated
      from a raw ROSTER.ULT/PARTY.ULT record (candidate:
      `beginCombatEncounter`'s per-slot copy loop) and checking whether
      `+0x1Fh` gets an explicit derived write there, distinct from
      whatever raw byte the roster file itself holds at that offset.
- [ ] Identify `sub_1633B` (called from `updateMonsterAI` and from
      `sub_1232F`, compares against `_partyPosition`) and `sub_1232F`
      itself (the special-case handler for monster types `'t'`/`'<'`
      in `updateMonsterAI`), `sub_17F96`/`sub_128F2` (helpers
      `canMoveToTile` calls into), `sub_17233`/`sub_17254`
      (movement-blocked checks `cmdMoveNorth` calls), `sub_16366`
      (confirmed as shrine entry via its own strings, not yet renamed),
      and `sub_15B28` (low confidence, checks `_gameMode`/`byte_158CB`/
      `word_114C2` vs `byte_116E3` — read once, purpose not pinned
      down) — all found in passing this pass but not chased down.
- [ ] Trace the overworld/town/dungeon map file loader against the
      confirmed filename list (all 19 `.ULT` files, `DUNGEON.DAT`) —
      `drawTileGrid`'s confirmed 64-byte-tile/11×11-grid shape is a
      strong lead for the combat-arena renderer specifically (exact
      dimension match with `CNFLCT_*.ULT`).
- [ ] `SHAPES.ULT`/`CHARSET.ULT` — this is also where the
      `drawCharGlyph`-buffer-is-all-zeros mystery (see `ULTIMA.COM`'s
      open items above) most likely resolves, since `entryFromBootup`
      does load both files directly.
- [ ] `sub_15D83` (spells) and the `Cure`/`Heal`/`Resurrect`/`Recall`
      temple interactions (`sub_1A692`) — good self-contained targets
      once the main dispatchers are underway, since Ultima III's
      cleric/wizard spell list and temple mechanics are well-documented
      externally (`ULTIMA3.TXT`, LairWare's `UltimaSpellCombat.c`) to
      cross-check against.
- [ ] Confirm/extend the `RosterEntry` struct against actual character-
      state manipulation during play (HP loss in combat, gold/food
      changes) — this is where fields only inferred so far
      (`_maxHitPoints`, armour/weapon owned arrays beyond index 0) will
      get real evidence.
- [ ] `EXODUS.BIN`'s own internal fixed data tables (per external
      documentation in file-formats.md: castle/town/dungeon/moongate
      coordinates at `0x15E1`/`0x15E5`/`0x15F9`/`0x184D`/`0x1855`, "look"
      command strings at `0x6566`) — cross-reference those file offsets
      against the disassembly now that it's loaded.
- [ ] Resolve the `AMBROSIA.ULT` (on disk) vs. `FAWN.ULT`/`EXODUS.ULT`
      (referenced as strings here, not present on disk) discrepancy —
      trace whichever function loads town/castle maps by name to see
      how/whether these are actually reached.
- [ ] `DUNGEON.DAT` (1,866 bytes) / `MOVES.ULT` (1,024 bytes) — smaller,
      less obviously-structured data files, lower priority.
- [ ] Locate and name `checkDebugModeFlag`'s equivalent in this IDB (a
      `mov al,0FFh; retn`-shaped stub near `drawCharGlyph`/`clearFramebuffer`
      per the other two IDBs' layout) — a first address guess was
      wrong and wasn't worth further chasing this pass; low priority.

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
