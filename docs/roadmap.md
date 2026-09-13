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
      — DONE, all 33/33 commands named as of 2026-09-14 (`mainGameLoop`,
      `OVERWORLD_COMMAND_TABLE`/`OVERWORLD_COMMAND_KEYS`). The last
      handful (`cmdZtats`, `cmdYell`, `cmdWear`, `cmdHandEquipment`) were
      confirmed via `ida_scripts/dump_overworld_labels.py`, a read-only
      dump of the per-command prompt-string table at `DS:18C9h`
      (linear `0x118C9` — the operand's raw `18C9h` is *not* itself a
      valid linear address in this segment, since `[bx+18C9h]` is
      DS-relative and DS=`0x1000` here; same near-pointer/segment
      convention applies to the string pointers the table holds). That
      dump also caught one earlier misnamed handler: `cmdOrder`
      (`0x174D5`, 'O') was renamed to **`cmdOtherCommand`** once the
      real on-screen prompt turned out to be "Other command!", nothing
      to do with party order (party reordering is `cmdExchange`/'M',
      "Modify order!", already correctly named). See overview.md's
      findings log for the full table dump and evidence per command.
      `cmdHandEquipment` (`0x11E55`, 'H') — the very last letter
      resolved — has a confirmed prompt ("Hand Equipment!\nFrom
      Player: ") but an unconfirmed mechanism: it selects two distinct
      players then makes a genuine re-entrant `call` back into the top-
      level dispatcher `sub_17B54` (which saves/restores every
      register, so this isn't parameter-passing) — the actual item
      hand-off almost certainly happens through a not-yet-located
      global "hand mode" flag that a subsequent command (a good bet:
      `cmdWear`) checks. That follow-on mechanism is the concrete next
      lead, not yet traced.
- [x] `cmdDisabledOnSurface` (D and K, both `loc_15CC3`) — both are
      no-ops on the overworld, consistent with Descend/Klimb being
      dungeon-only commands.
- [x] `cmdIgniteTorch` ('I') — confirms `_torches` (`RosterEntry`
      `+0x0F`) independently of external documentation.
- [x] **`dungeonMainLoop` and `DUNGEON_COMMAND_TABLE`/
      `DUNGEON_COMMAND_KEYS`** — done, 2026-09-14. `jpt_18389` (real
      address `0x1774A`) is the dungeon counterpart to
      `OVERWORLD_COMMAND_TABLE`, entered via `initDungeonState` right
      after `cmdEnter`'s dungeon branch. Confirmed via the "It's dark!"
      message tying directly to `cmdIgniteTorch`'s own flag
      (`byte_115CE`) — not just structural similarity to the overworld
      loop. See overview.md for the full writeup.
- [x] **`DUNGEON_COMMAND_TABLE`'s 6 dungeon-specific handlers** — done,
      2026-09-14. An earlier note here (same session) had the wrong
      addresses for these, from a disposable exploratory script's bug
      reading past the table's real boundary — corrected before
      anything wrong was applied to the IDB; see overview.md's
      correction writeup. The real handlers form a complete, clean
      first-person dungeon movement system: `cmdKlimb`/`cmdDescend`
      (confirms a new `_dungeonLevel` field), `cmdTurnLeft`/
      `cmdTurnRight` (rotate `_facingDirection`), `cmdMoveForward`/
      `cmdMoveBackward` (via a confirmed `_dungeonFacingDeltaX`/`Y`
      table). 'S' (Steal) turned out to be disabled in dungeons too,
      not a 7th unique handler as first miscounted.
- [x] **`DUNGEON_COMMAND_LABELS` (`off_1778C`) confirmed** — done,
      2026-09-14, via `ida_scripts/dump_dungeon_labels.py` (read-only,
      same technique as `dump_overworld_labels.py`). It's exactly the
      per-command prompt-string table hypothesized: all 10 disabled
      letters (`B`, `A`, `E`, `F`, `L`, `Q`, `T`, `U`, `X`, `S`) print
      the shared string `"Not a DNG cmd!\n"`, and every enabled
      letter's prompt matches its handler exactly, including a free
      cross-check of the earlier movement-handler correction:
      `cmdKlimb`="Klimb", `cmdDescend`="Descend", `cmdTurnRight`="Turn
      right", `cmdTurnLeft`="Turn left", `cmdMoveForward`="Advance",
      `cmdMoveBackward`="Retreat" — all line up perfectly, independently
      confirming that fix was correct.
- [ ] **Why is Unlock ('U') disabled in dungeons?** `DUNGEON_COMMAND_TABLE`
      routes 10 letters (B, A, E, F, L, Q, T, U, X, S — `cmdDisabledInDungeon`)
      to one shared disabled-command stub — most make obvious sense (no
      vehicles/locations-within-locations/surface-only-save/nothing to
      steal underground), but Unlock being disabled specifically is
      surprising given Ultima dungeons are full of locked doors. Not
      explained this pass — worth checking whether locked-door
      interaction in dungeons happens automatically on movement
      instead of via a dedicated command, or whether this finding
      needs re-verification.
- [ ] Confirm whether `_locationTypeTable` (`byte_1259D`) is really a
      scalar or (more likely, given it's indexed alongside the
      19-entry `LOCATION_TILE_TABLE`) a 19-byte parallel array —
      applied at low-medium confidence this pass, worth a quick
      `ida_bytes.get_word()`-style direct check against the actual
      indexed access pattern before trusting it as-is.
- [x] **`WIZARD_SPELL_TABLE`/`CLERIC_SPELL_TABLE` — all 32 spell slots
      named**, done 2026-09-14. Confirmed both tables are exactly 16
      entries (32 bytes) each directly from `castSpell`'s own letter
      bounds-check ('A'..'P'), not from data layout — an earlier
      boundary-walk attempt over-read `CLERIC_SPELL_TABLE` by 16 bytes
      into an unrelated table, the same class of mistake as the
      dungeon-command mixup, caught the same way. Every spell's magic
      word was read directly from a 32-entry name-pointer table in the
      binary (`SPELL_NAME_TABLE`, linear `0x1590B`) and cross-checked
      against `C:\games\ultima3\ULTIMA3.TXT`'s in-game spellbook manual
      — all 32 words match the manual exactly (mod one likely manual
      typo, "SANTU MANI" vs. the binary's "Sanctu Mani"). See
      overview.md for the full name/address table and two notable
      findings: 6 of the 32 (letter, class) slots share their effect
      routine with a same-purpose spell in the *other* class (e.g.
      Wizard "Lorum" and Cleric "Luminae", both short-light spells,
      are literally the same routine), and Wizard slot 'P' — not a
      documented spell in the manual, whose name-table entry is an
      empty string — is nonetheless selectable per `castSpell`'s
      generic 'A'..'P' range check, and invokes Cleric's most powerful
      attack spell ("Zxkuqyb") when selected. **Still open**: each
      routine's actual mechanic is sourced from `ULTIMA3.TXT`'s flavor
      text, not independently confirmed by reading the routine's own
      code — a good next target now that all addresses are named,
      along with cross-referencing LairWare's `UltimaSpellCombat.c`
      for its own effect implementation as a secondary source.
- [x] **Combat damage-resolution helpers** — done, 2026-09-14:
      `findCombatantAtPosition` (`0x18E46`, look up the occupied
      8-slot arena combatant at a given (X,Y), returning its slot
      index via a `lea bx,entryFromBootup; sub si,bx` trick that
      cancels the array's fixed base against `entryFromBootup`'s own
      near-offset), `fireProjectileAcrossArena` (`0x18E7A`, steps a
      ranged attack across the 11×11 arena by a fixed delta,
      redrawing and hit-testing each step), `applyCombatDamage`
      (`0x18F5A`, subtracts damage from a slot's HP/count field as
      plain binary, not BCD, handles death + `addExperienceClamped`
      (BCD amount from the now-identified `MONSTER_EXP_TABLE`), with
      an unexplained skip when `_conflictMonsterClass == 0x13`), and
      `applyRandomGroupDamage` (`0x15EAA`, confirmed shared by
      `spellRespond` and `spellNoxum` via address-range containment of
      their `CODE XREF` offsets — loops all 8 arena slots, ~75% chance
      per occupied slot via `and dl,3`). The **melee** to-hit/damage
      formula itself (inline in `updateMonsterAI`'s attack chunk, not
      a separate function) is now fully traced too: to-hit chance is a
      BCD roll against the defender's `_dexterity`
      (`RosterEntry+0x13`), and damage on a hit is
      `4 + weaponIndex*3 + floor(strength/2) + random(0..strength|1)`,
      strength/weapon read from `RosterEntry+0x12`/`+0x30`. Still
      open items closed while chasing this down, same session: found
      `MONSTER_EXP_TABLE` (linear `0x18615`, the address
      `applyCombatDamage`'s `[bx-79EBh]` resolves to) and the adjacent
      `MONSTER_HP_TABLE` (linear `0x18605`,
      `beginCombatEncounter`'s `[bx-79FBh]`, used as a PRNG upper bound
      for a monster's starting `+0x98` HP-like counter, then OR'd with
      `0x0F`) — both indexed by `_conflictMonsterClass & 0xFh`, 16
      entries each, dumped via `ida_scripts/dump_monster_tables.py`.
      **Self-correction**: `applyCombatDamage`'s subtraction was
      initially documented as BCD by analogy with the rest of the
      codebase's stat fields; re-checking the actual instructions
      showed no `das` follows the `sub` (and the HP-counter's own
      init ORs in `0x0F`, an invalid BCD nibble), so it's plain binary
      — fixed before this was committed anywhere, per the same
      re-verification discipline as the earlier dungeon-table
      correction.
- [x] **Level vs. Experience offset conflict — RESOLVED, 2026-09-14**:
      there is no conflict; the two functions read the same bytes for
      different display purposes. `showCharacterDetails`
      (`ultima_bootup.idb`) does `mov ax, [bx+1Eh]; call printHexWord`
      — a plain 2-byte read of the *entire* `+0x1E`/`+0x1F` word,
      confirming `_experience` really is a full word as already
      modeled. `drawPartyStatusBar` (`ultima_exodus.idb`) separately
      does `mov al, [bx+1Fh]; add al,1; daa; ...; call printHexByte`
      labeled "L:" — this is just reading the **high byte of that same
      word** (little-endian, so `+0x1F` is Experience's high BCD
      digit-pair) and displaying it as a derived Level indicator: `Level
      = high_byte_of_Experience + 1` (BCD, clamped to 99). No
      "live copy caches a derived byte" mechanism exists (the
      hypothesis floated earlier) — Level was never a separate stored
      field, on disk or in memory; it's computed on the fly from
      Experience's top digits every time the status bar redraws.
      Sensible design too: with Experience clamped to 9999,
      `floor(exp/100)+1` gives a 1-100 level range from a single
      100-XP-per-level curve, all without spending a dedicated byte on
      it in the 64-byte record.
- [x] **`sub_16366` (shrine entry) renamed and fully traced**, done
      2026-09-14: **`enterShrine`**. Prompts a player, loads
      `SHRINE.IMG`, sets the game-mode byte to 4, and announces the
      shrine's name via the newly-identified `SHRINE_ATTRIBUTE_NAME_TABLE`
      (linear `0x15993`, 4 entries — 'Strength'/'Dexterity'/
      'Intelligence'/'Wisdom' — indexed by `_partyPosition & 3`,
      confirming Ultima III's 4 shrines correspond 1:1 to the 4
      primary attributes). Then prompts an "Offering\*100-" gold
      amount, rejects one above a shrine-specific max ("You can't
      cheat the Gods!"), spends the gold, and raises an attribute
      before printing "Shazam!". **Not fully resolved**: the exact
      formula for *which* attribute gets raised involves a
      character-race lookup (`[bx+16h]` against a 5-entry
      `byte_158CD` table) combined with the shrine index in a way not
      fully disentangled, and the per-shrine max-offering table
      (`[bx+di+58D2h]`) isn't independently confirmed — flagged rather
      than guessed at further.
- [x] **`sub_162FD` renamed**: **`drawDungeonStatusBar`** — prints
      "LVL:"+`_dungeonLevel`+1 and "Head-"+facing-direction name via
      the newly-identified `FACING_DIRECTION_NAME_TABLE` (linear
      `0x1598B`, 4 entries: 'North'/'-East'/'South'/'-West', indexed
      by `_facingDirection`). Both this table and
      `SHRINE_ATTRIBUTE_NAME_TABLE` sit immediately after
      `CLERIC_SPELL_TABLE`'s real 16-entry end — the exact region an
      earlier, wrong version of `dump_spell_tables.py` mistakenly
      over-read (see the spell-table findings above); dumping them
      properly by address, rather than by assumed table membership,
      resolved both in one pass.
- [x] **`sub_15B28` renamed**: **`isSpecialEncounterLocation`**, done
      2026-09-14. Fully traced its boolean formula: true exactly when
      `_savedOverworldPosition`'s low byte equals a fixed constant
      (`byte_116E3` = `0x0A`, confirmed a true constant — never
      written anywhere else in the binary) AND either game mode
      (`byte_114BC`) `== 3`, or game mode `== 0x80` (combat) with
      `byte_158CB == 3`. In `beginCombatEncounter`, a true result
      means the encounter always spawns a full, randomly-sized group
      (up to 8 monsters); false falls through to a separate check that
      can instead force a single monster. **Left open**: *why*
      position `0x0A`/game mode 3 is special — i.e. what specific named
      location this constant refers to — isn't confirmed; named for
      the condition it checks, not for an asserted location identity.
- [x] **Overworld monster breath attack traced and named**, done
      2026-09-14: `sub_1232F` → **`monsterBreathAttack`** (the
      special-case handler for monster types `'t'`/`'<'` — dragon-type
      monsters — in `updateMonsterAI`; 50% chance per call, steps a
      breath effect toward the party's screen-center position, blocked
      by impassable terrain, damaging the party on a hit) and
      `sub_1633B` → **`computeStepTowardParty`** (shared aiming helper,
      also called directly by `updateMonsterAI` for ordinary monster
      chasing — computes a single-step direction toward
      `_partyPosition` with wraparound on the overworld's 64-tile-per-
      axis map). Following the hit path also named `sub_182C6` →
      **`damagePartyAll`** (loops all 4 live party members, applying
      `random(0..0x77) + (_dungeonLevel+1)*8` BCD damage to each alive
      one — the dungeon-depth term suggests this helper is shared with
      a dungeon-context caller too, not traced) and `sub_16BC9` →
      **`damageCharacterHP`** (the general BCD HP-subtract primitive,
      also used by `processPartyTurnEffects`/poison-hunger effects;
      handles death by setting `_status='D'` and calling the
      not-yet-traced `sub_16B91`).
- [x] **Auto-save-on-death traced**, done 2026-09-14: `sub_16B91` →
      **`autoSaveOnDeath`**, called from `damageCharacterHP` whenever
      *any* character dies (not just on a full party wipe). Depending
      on game mode, calls either `saveSosariaAndParty` (`sub_1207D` —
      saves `SOSARIA.ULT` then `PARTY.ULT`, sizes matching both files'
      confirmed on-disk sizes exactly) or just `savePartyFile`
      (`sub_12097` — `PARTY.ULT` alone, e.g. inside a dungeon/town
      where the overworld map hasn't changed). Confirms Ultima III
      permanently persists a character's death immediately, not only
      at an explicit Quit & Save — a real "hardcore" mechanic worth
      replicating exactly in the eventual reimplementation.
      `checkPartyWipedOut` itself (already named) is the actual
      game-over check: loops all 4 party slots, and if every one is
      dead, prints "All Players Out!", calls `autoSaveOnDeath`, and
      jumps to a small 2-byte function chunk at `loc_17252` — likely
      the actual reset-to-title/re-chain-load point, not yet traced.
- [ ] Identify `sub_17F96`/`sub_128F2` (helpers `canMoveToTile` calls
      into), `sub_17233`/`sub_17254` (movement-blocked checks
      `cmdMoveNorth` calls), and the `loc_17252` game-over destination
      `checkPartyWipedOut` jumps to on a full party wipe — all found
      in passing this pass but not chased down.
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
