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

## `ULTIMA.COM` (`ultima.idb`) — function-naming sweep: COMPLETE (61/61, 2026-09-14)

Every function is named — see
[overview.md](overview.md#session-2026-09-13-full-function-naming-sweep-5454-named)
for the full findings log. Remaining loose ends:

- [x] `printHexWord`'s orphaned code (no `proc`/`endp` boundary) fixed
      via `ida_funcs.add_func()` — confirmed real, reachable-from-
      BOOTUP.BIN's-identical-copy shared code, not dead weight. See
      `ida_scripts/fix_orphaned_functions.py`.
- [x] **2 more regions of undefined code** fixed via `ida_funcs.add_func()`
      — both **confirmed real** (byte-for-byte identical, properly-
      bounded copies already named in `ultima_bootup.idb`). The earlier
      heuristic ("walk backward to the nearest `retn`") had guessed
      wrong boundaries (`~0x18C39`/`~0x18CD9`); this time the exact
      boundary was found the safe way — dump the confirmed
      `ultima_bootup.idb` copy's raw bytes
      (`dump_bootup_func_bytes.py`), dump a wide raw-byte window around
      the candidate region in `ultima.idb`
      (`dump_ultima_orphan_bytes.py`), and locate the byte-for-byte
      match by eye (identical opcodes throughout, differing only in one
      relocated near-pointer/immediate word operand per function — an
      expected artifact of the two binaries having different data
      segment layouts). See `apply_orphan_funcs_ultima.py`.
  - **`promptForNumberEntry`**: confirmed `0x18C6F`-`0x18CC0` (81 bytes, matching
    `ultima_bootup.idb`'s `0x1498F`-`0x149E0` exactly) — **not**
    `~0x18C39` as first guessed; that address falls inside an unrelated
    preceding code fragment. Calls `accumulateInputDigit`/`readLine`.
    Matches `ultima_bootup.idb`'s copy, which has a confirmed real
    caller there (`getEntryNumber`, used for every "Entry#" prompt). No
    caller found within `ultima.idb` itself — real shared-runtime code
    this title screen just doesn't happen to use.
  - **`saveFile`**: confirmed `0x18CC0`-`0x18D10` (80 bytes, matching
    `ultima_bootup.idb`'s `0x149E0`-`0x14A30` exactly) — starts exactly
    where `promptForNumberEntry` ends, no gap between them. Calls
    `openFileWithRetry`, mirrors `loadFile` but with `AH=15h`
    sequential write. Matches `ultima_bootup.idb`'s `saveFile`, which
    writes PARTY.ULT/ROSTER.ULT from several confirmed call sites
    there. Same situation: real code, unused by the title screen.
      `ultima.idb`: 60/60 functions named, 0 `sub_XXXXX` remaining
      (confirmed via `identify.py`).
- [x] **RESOLVED 2026-09-14**: the raw `INT 13h` disk-sector-read
      routine (was guessed at `~0x1878C`; real address `0x18793` —
      that guess landed inside `drawCharGlyph` instead, another stale
      address from before this session's own edits). Found for real by
      scanning for the raw `CD 13` opcode bytes directly rather than
      trusting a remembered address. Reads track 9, head 0, **sector
      16** (drive from `AH=19h`/`INT 21h`, `AH=19h` "get default
      drive"), into segment `0xC000`, with up to 4 retries, returning
      0=success/-1=fail in AL. Sector 16 doesn't exist on a standard
      9-sectors-per-track floppy — consistent with a **copy-protection
      "weak sector" check**, matching the "Wrong Diskette!" theme
      elsewhere. Named `checkDiskCopyProtection` and given a real
      `ida_funcs.add_func()` boundary (`0x18793`-`0x187E3`, 80 bytes,
      confirmed byte-for-byte).
  - **Checked all three binaries, as this item asked**: byte-for-byte
      identical copies exist in `ultima.idb` (`0x18793`) *and*
      `ultima_bootup.idb` (`0x144B3`, also named+defined this session)
      — contradicting the earlier "no matching copy in
      `ultima_bootup.idb`" note, which was based on an incomplete
      read-through. `ultima_exodus.idb` has a structurally-different
      assembly of the same logic at `0x15003`, but it's even more
      clearly dead: it has **no `retn` of its own** and falls straight
      through into that IDB's own `plotPixel2bpp` — not left
      `add_func`'d there, since forcing a function boundary on code
      that doesn't even return properly would be exactly the kind of
      guess this project has learned to avoid.
  - **Zero callers found for this routine in any of the three
      binaries** (confirmed via exhaustive xref search, not just "none
      spotted") — it's real, shared, copy-protection-shaped code that
      the shipped product never actually invokes anywhere.
  - The "small helper right before `drawTileGrid`" from the same note
      turned out to be a **third already-resolved mystery**: it's not
      a mysterious word-swap, it's `plotPixel2bpp` itself (already
      named and actively used — 5 callers in `drawWindowBorder` in
      `ultima_bootup.idb`, 9 in `drawScreenBorder` in
      `ultima_exodus.idb`). `ultima.idb` happens to have a second,
      *unused* orphaned copy of the same code sitting right next to
      its own dead `checkDiskCopyProtection`, separate from its own
      live `plotPixel2bpp` elsewhere — left undefined since naming a
      second `plotPixel2bpp` in the same IDB adds no value.
- [x] **RESOLVED 2026-09-14**: `checkDebugModeFlag`'s exact role,
      confirmed from disassembly alone (no emulator needed). Its body
      really is just `mov al, 0 / retn` (3 bytes) — a hardcoded
      "debug mode is off" stub. Its one caller
      (`titleScreenAndChainToBootup`) stores the result in
      `byte_1432C`, then later does `cmp byte_1432C, 0FFh; jz $` — a
      busy-wait-on-self loop that would spin forever **if** debug mode
      were ever signaled (`0FFh`), gating entry into
      `runBootFlagAnimation`. Since the flag is hardcoded to 0, that
      wait never triggers in the shipped build; this reads as a
      genuine, deliberately-disabled developer hook (e.g. a debugger
      breakpoint stand-in), not a generic guess. `adjustAnimSpeed`,
      `computeAnimTableByte`, and the 6 `drawTitleBoxN` wrappers still
      need visual confirmation and remain open.
- [x] **RESOLVED 2026-09-14**: why `drawAnimatedPixelPath` only
      consumes 533 of `NAME.DAT`'s 640 bytes. Read the raw file
      directly: the meaningful `(length, row)` script data ends with
      its own `0x00 0x00` terminator at exactly file offset
      `0x212`-`0x213` (533 bytes total including the terminator,
      matching the `loadFile` call's `cx = 0x215` precisely). The
      remaining 107 bytes are literal zero padding, nothing hidden.
      See
      [file-formats.md](file-formats.md#namedat-640-bytes--a-pixel-path-animation-script-not-a-name-table).
- [ ] The `create_strlit` mystery from `fix_wind_string_array.py`: it
      returns `False` for the wind strings in *both* IDBs even after
      `del_items`, forcing a fallback to a plain `FF_BYTE` array via
      `idc.create_data`. Root cause not identified (possibly an IDA 8.3
      API quirk, possibly the leading `0x10` control byte). Not
      blocking — the fallback works — but worth a look if it recurs.
- [x] **RESOLVED, 2026-09-15**: `BLANK.IBM`/`EXOD.IBM` confirmed as
      raw CGA framebuffer images (16,384 bytes = one full 4-color
      graphics-mode bank), exactly the guess. `BLANK.IBM` is copied
      directly into CGA video memory (`es=0xB800`) right after
      loading; `EXOD.IBM` (Exodus's own portrait, loaded the same way
      right after) isn't blitted at its own load site — the title
      sequence calls `drawAnimatedPixelPath` shortly after with a
      *different* source buffer (`ANIMATE.DAT`), consistent with a
      progressive reveal rather than an instant blit, though the exact
      site that finally draws `EXOD.IBM` itself wasn't traced. See
      file-formats.md.
- [x] **RESOLVED, 2026-09-14**: why does `drawCharGlyph`'s glyph-data
      buffer read as all zeros in `ULTIMA.COM`'s own file image? Found
      it: `ultima_exodus.idb`'s `entryFromBootup` is exactly where
      `CHARSET.ULT` gets loaded — `loadFile` with `bx = byte_13B39 +
      0x400`, `cx = 0x800` (2048 bytes) — immediately after loading
      `SHAPES.ULT` into `byte_12B39` with `cx = 0x1400` (5120 bytes).
      The two buffers are exactly contiguous: `0x12B39 + 0x1400 =
      0x13F39 = 0x13B39 + 0x400`, so `SHAPES.ULT` and `CHARSET.ULT`
      load back-to-back into one combined 7,168-byte graphics-asset
      region. Since `ULTIMA.COM`/`BOOTUP.BIN` never load `CHARSET.ULT`
      themselves, their own static file images correctly show that
      shared buffer as all zeros — it's simply populated later, once
      `EXODUS.BIN` runs. Since both earlier executables display real
      text successfully before that point, they must not be routing it
      through `drawCharGlyph` at all — most likely plain DOS/BIOS text
      output for their own (non-graphics-mode) screens, with
      `drawCharGlyph` being an EXODUS-only, graphics-mode glyph
      renderer that's simply never exercised until its buffer is
      actually populated. Not independently confirmed which text
      routine `ULTIMA.COM`/`BOOTUP.BIN` use instead, but the "zero
      buffer" mystery itself is fully explained.

## `BOOTUP.BIN` (`ultima_bootup.idb`) — function-naming sweep: COMPLETE (74/74, 2026-09-14)

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

- [x] **`RosterEntry` wired into real instruction operands**, done
      2026-09-15 (`idc.op_stroff` per site, both IDBs): 39 sites across
      7 functions in `ultima_bootup.idb`
      (`showCharacterDetails`/`handleCreateCharacter`/
      `gatherCharacterCreationInput`/`showRegister`/`handleFormParty`/
      `clearPartySelection`/`handleTerminateCharacter`), 206 sites
      across 26 functions in `ultima_exodus.idb`. The `.asm` now shows
      `[bx+RosterEntry._hitPoints]`-style field names instead of raw
      offsets at every confirmed site.

      Found via a data-driven scan (`find_roster_stroff_candidates.py`)
      rather than guessing which functions to target — it flags every
      `[reg+N]` operand in the IDB whose `N` matches a RosterEntry
      member offset, grouped by function, ranked by how many *distinct*
      offsets a function touches (a function hitting several different
      confirmed offsets is very likely a real RosterEntry accessor;
      one hitting only a single common small offset is far more likely
      coincidental). Applied only to a manually-vetted allowlist, not
      blindly to every match.

      **Two categories of false positive excluded on sight, both
      recurring across both IDBs**: (1) offset `0x0` (`_name`) matches
      are *always* noise here — real `_name` access loops over
      multiple bytes (`readLine`), it's never touched via a single
      `[reg+0]`, and 0 is an extremely common displacement in
      unrelated code (stack-frame locals like `[bp+0]`, array bases).
      (2) `updateLogoAnimationB`/`swapAnimTableRows` matching
      `_foodSubCounter`/`_keys` — boot-animation code, topically
      unrelated to character records, coincidental offset overlap.

      **A real mistake happened and was caught before it mattered**:
      a first, uncurated pass over `ultima_exodus.idb` also matched 21
      sites inside a genuinely mis-disassembled data region around
      `0x16700`-`0x16900` (garbage 386-only instructions like `arpl`/
      `gs:`/`fs:` prefixes and jumps into mid-instruction addresses —
      impossible in this program's real 8086/80186-era code — see the
      new roadmap item below). That pass ran with IDA's own headless
      driver's `-NoExport` flag, which was assumed to make it a safe,
      non-persisting dry run — it doesn't: `.idb` files are live paged
      databases that commit edits as they happen regardless of an
      explicit `save_database()` call, so all 21 bad annotations
      landed on disk anyway. Caught immediately by hand-reviewing the
      full result list before trusting it (not by assuming success),
      and reverted cleanly via `idc.op_hex` (`revert_bad_stroff.py`)
      before re-applying the corrected, excluded-range version. Worth
      remembering generally: **`-NoExport` prevents the `.asm`/`.idc`
      export and the final `save_database()` call, but does NOT make
      IDB edits transient** — there is no safe "try it and discard" for
      a headless run once `idc.*` write calls have executed; review
      before trusting, the same discipline as every other rename batch
      this project has used, not a special exemption for structural
      edits.
- [x] **RESOLVED, 2026-09-15 — and it turned out to be a landmark
      find.** The mis-disassembled region around linear
      `0x16700`-`0x16900` was `printNameByIndex`'s own backing data: a
      0x88-entry pointer table at `0x16556` (already known from that
      function's own `[bx+6556h]` operand) indexing into a large
      null-terminated ASCII string blob, confirmed and fixed via
      `fix_game_name_table.py`. Named the table `GAME_NAME_TABLE` and
      defined all 146 strings it addresses. This is a single, unified
      "describe anything" table the whole game shares: **terrain
      names** (1-9: Water/Grass/Brush/Forest/Mountains/Dungeon/Towne/
      Castle/Floor — exactly `cmdLook`'s tile categories, confirmed
      independently for the first time), objects/vehicles (`0xA`-`0xC`),
      the Whirlpool (`0xD`), NPC/monster-class names (`0xE`-`0x20`,
      ending at 'Exodus'), dungeon features (`0x21`-`0x26`: Force
      Field/Lava/Moon Gate/Wall/Void), single-letter rune/sign tiles
      (`0x27`-`~0x42`), weapon names matching `_weaponOwned`'s 15-entry
      array, armour names matching `_armourIndex`'s range (an extra
      leading 'Skin'/unarmored entry beyond `_armourOwned`'s own
      7-entry list), all 32 spell names again (a second copy, distinct
      from `SPELL_NAME_TABLE`), and — new — **a confirmed 16-entry
      monster bestiary** (`0x79`-`0x88`: Brigand/Cutpurse/Goblin/Troll/
      Ghoul/Zombie/Golem/Titan/Gargoyle/Mane/Snatch/Bradle/Griffon/
      Wyvern/Orcus/Devil), Ultima III's actual named monster list,
      confirmed directly from the game's own data for the first time
      this project.
- [x] **RESOLVED, 2026-09-15 — the exact D/S/L/M-to-card mapping,
      Ultima III's Exodus puzzle SOLVED end-to-end.** A second, small
      mis-disassembled patch right next to the above (`0x16AE8`-`0x16B18`)
      turned out to hold the puzzle's literal answer key. Fixed via
      `fix_exodus_sequence_table.py`: the 4 card names ('Moons',
      'Death', 'Love', 'Sol'), the menu's valid keys
      (`EXODUS_SEQUENCE_MENU_KEYS` = 'M','D','L','S','Q',Esc), and —
      the answer itself — `EXODUS_SEQUENCE_ANSWER`, a 4-byte lookup
      array holding the literal bytes `'L','S','M','D'`, which
      `attemptExodusSequence`'s own code (`cmp al, [bx+6B14h]`,
      `bx=word_164A0`) indexes by step number to check the player's
      answer against. **The solution is Love, Sol, Moons, Death, in
      that exact order** — confirmed two independent ways at once:
      directly from this lookup table's raw bytes, and from the
      Time Lord's in-game vision text found in the same investigation
      (`aYouSeeAVisionO`: "You see a vision of the Time Lord. He tells
      you: The one way is Love, Sol, Moons & Death, All else fails.")
      — word-for-word identical. Also recovered Radrion the Oracle's
      full riddle while reading the surrounding strings (previously
      only partially quoted): the 4 Marks are fire/force/snake/king,
      obtained "in Devil Guard", and the 4 Cards are "Sol, Moon,
      Death and Love" — all of Ultima III's central puzzle lore is
      now captured directly from the game's own disassembly.
- [x] **RESOLVED** (this checkbox was just stale — the confirmation
      itself already happened 2026-09-13, and was re-verified directly
      2026-09-14): `_maxHitPoints` (offset `0x1C`) is not just a
      second HP-shaped word set at creation. `healHitPoints` in
      `ultima_exodus.idb` (`0x15D2B`-`0x15D4B`) BCD-adds to
      `_hitPoints`, then does `mov ax, [bx+_maxHitPoints];
      cmp ax, [bx+_hitPoints]; jnb short loc_15D48;
      mov [bx+_hitPoints], ax` — clamping current HP down to max HP
      whenever healing would push it over. That's an unambiguous
      "current can't exceed max" relationship, confirming the name
      directly from real arithmetic, not just parallel-write proximity.
- [x] **`_race`/`_class`/`_sex` letter encodings fully decoded**, done
      2026-09-15 (`dump_char_creation_tables.py`, applied via
      `fix_char_creation_tables.py`; `_status`'s 4 letters were
      already confirmed earlier via direct code logic, not a lookup
      table — see its own struct note). Walked the pointer tables
      programmatically rather than hand-parsing the raw hex (the same
      discipline as every other table this project has decoded, after
      getting burned once by manual byte-counting on the dungeon
      command table):
      - **`SEX_KEYS`** (`0x11045`) → `SEX_NAME_PTRS`: `'M'`=Male,
        `'F'`=Female, `'O'`=Other.
      - **`RACE_KEYS`** (`0x1106B`) → `RACE_NAME_PTRS`: `'H'`=Human,
        `'E'`=Elf, `'D'`=Dwarf, `'B'`=Bobbit, `'F'`=Fuzzy.
      - **`CLASS_KEYS`** (`0x110D0`) → `CLASS_NAME_PTRS`: `'F'`=Fighter,
        `'C'`=Cleric, `'W'`=Wizard, `'T'`=Thief, `'P'`=Paladin,
        `'L'`=Lark, `'B'`=Barbarian, `'D'`=Druid, `'I'`=Illusionist,
        `'A'`=Alchemist, `'R'`=Ranger.

      Also confirmed, from `getMenuChoice`'s own code, that
      `RosterEntry._sex`/`._race`/`._class` store the raw **ASCII key
      letter** itself (e.g. literally `'M'`), not a 0-based index —
      `getMenuChoice` only uses the index internally for a live-preview
      name lookup while the player is choosing, but returns (and
      `showCharacterDetails` independently re-derives the display name
      from) the original letter.

## `EXODUS.BIN` (`ultima_exodus.idb`) — function-naming sweep: COMPLETE (145/145, 2026-09-14)

**DONE as of 2026-09-14 — every one of the 144 functions in this IDB
has a real name.** Created 2026-09-13 (same recipe as
`ultima_bootup.idb`: copy to a temp `.com` file for IDA's loader
auto-detection), ~14,600 `.asm` lines. This was comparable in scope to
`ultima1`'s or `ultima2`'s entire sibling efforts, and covers
everything: the shared low-level runtime, the overworld command
dispatcher (`readAndDispatchCommand`, formerly `sub_17B54` — the
single massive multi-chunk function every `cmdX`/`combatCmdX`/
`templeX`/shop handler lives inside as a label, not a separate
procedure), the combat dispatcher (inside `updateMonsterAI`'s chunk
list), spellcasting (`castSpell`, all 32 spell effects), all 8 town
shops/NPCs, temples, the dungeon-monster AI and its own separate
combat-damage system, the Peer/Vieda map-overview rendering, the
whirlpool mechanics (including the secret Ambrosia gateway), the win
condition, and Ultima III's legendary Exodus endgame puzzle. See
overview.md's findings log for the full narrative, and
`apply_renames_exodus.py` for the complete rename list with per-entry
evidence.

The checklist below is kept as a historical record of how this sweep
actually happened — a genuine, sometimes winding, evidence-first
process — not as a to-do list; nothing under `EXODUS.BIN` remains
open at the function-naming level. Three deeper mechanism questions
were flagged inline as the legitimate remaining frontier beyond simple
naming gaps — **all three are now resolved**: the exact D/S/L/M-to-card
mapping in the Exodus puzzle (`EXODUS_SEQUENCE_ANSWER` — see below),
`cmdHandEquipment`'s exact item-transfer mechanism (a thorough,
documented negative result — see below), and `drawDungeonView`'s
self-modifying `start` call target, confirmed 2026-09-14:
`DUNGEON.DAT` is itself a raw machine-code overlay (the dungeon
first-person-view renderer), loaded into the `start` buffer and
called directly as code when a torch is lit — see
[file-formats.md](file-formats.md#files-with-no-external-documentation-found-2026-09-13-search)
for the full disassembly-confirmed mechanism.

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
      players (`si`=from, `di`=to, both real `RosterEntry` pointers
      from `selectPlayer`) then makes a genuine re-entrant `call` back
      into the top-level dispatcher `readAndDispatchCommand` (which
      saves/restores every register including `si`/`di`, so this
      isn't parameter-passing in the usual sense — it only works if
      some handler running *inside* that nested call reads `si`/`di`
      directly, since they're otherwise just callee-saved and
      restored on return). **Investigated further, 2026-09-15, ruled
      out more than confirmed**: `wearArmour`/`readyWeapon` push/pop
      `si`/`di` as pure scratch registers and never read the inherited
      values (confirmed by re-reading both start-to-end). Went
      further this time — searched every `[si+RosterEntry.field]`
      access in the whole IDB (only 10 total, small enough to check by
      hand): all 10 belong to unrelated code (`printCombatReactionMessage`'s
      status/class check, `attemptSpecialMonsterAttack`'s armour
      to-hit check, and `combatCmdAttack`'s own ammo-consumption logic,
      each computing `si` fresh from `_currentCombatant`, not
      inheriting it from a caller). **No consumer of the inherited
      `si`/`di` found anywhere via a direct `[reg+N]` RosterEntry
      access.** Either the actual transfer happens through a more
      indirect path this search wouldn't catch (e.g. `si`/`di` copied
      to another register or a global before use), or the command
      really is non-functional/vestigial in the shipped game — neither
      confirmed. A comprehensive negative result, not a dead end: the
      search space for "which command consumes this" is now much
      smaller than it was.
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
- [x] **RESOLVED 2026-09-14: why Unlock ('U') is disabled in
      dungeons.** Dumped `OVERWORLD_COMMAND_KEYS` (`0x11887`) and
      `DUNGEON_COMMAND_KEYS` (`0x17708`) side by side: `'U'` (`0x55`)
      is present in the overworld table but **entirely absent** from
      the dungeon one. `readAndDispatchCommand`'s dungeon-mode
      dispatcher never recognizes 'U' as a valid keypress in the first
      place — it's not that `cmdUnlock` runs and silently no-ops (the
      earlier `getMapTileAt() == 0xB8` theory, now superseded), it's
      that the key lookup itself rejects 'U' before any handler is
      ever reached. Simpler and directly decisive, confirmed straight
      from the two tables' raw bytes rather than inferred from tile
      semantics. Whether dungeon locked doors exist at all (and how
      they'd be opened, if so) remains a separate, still-open
      question.
- [x] **`_locationTypeTable` renamed to `_locationType`**, done
      2026-09-14: confirmed a plain scalar (`db 0`, declared alone,
      never accessed with an index anywhere in the binary) — not the
      19-byte parallel array the old name implied. Clean reads (`cmp
      _locationType,5/6/7` in `sub_17B54`; `cmp al,9` in
      `beginCombatEncounter`) match a simple "current location type"
      value. One write site inside a self-modifying-code setup block
      (which reads opcode bytes from several `loc_` addresses as raw
      data) stores something that looks like a code byte at this same
      address rather than a location type — not explained, flagged as
      a loose end rather than glossed over.
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
- [x] **Movement-blocked checks named**, done 2026-09-14:
      `sub_17233` → **`isShipMovementBlockedByWind`** — Ultima III's
      classic sailing-against-the-wind restriction, confirmed via
      `word_12A92`'s low byte (independently confirmed elsewhere as
      the current wind direction, via `updateWindDisplay`): a ship
      can't move at all when the wind is calm, and can't sail directly
      into the wind. `sub_17254` → **`checkTerrainMovementBlocked`** —
      general transport-dependent terrain passability; a few specific
      tile values don't block movement but play a distinct warning
      sound with a timed delay (an extra one when mounted), reading
      like a hazard-terrain audio cue rather than a hard block; the
      specific tile-to-hazard mapping isn't independently confirmed.
      Also confirmed (not renamed — a trivial 2-byte chunk, not worth
      a function name): `loc_17252`, the destination
      `checkPartyWipedOut` jumps to on a full party wipe, is a literal
      `jmp short loc_17252` to itself — Ultima III's game-over screen
      is a deliberate infinite hang after the "All Players Out!"
      message and auto-save, requiring the player to reboot/restart
      `ULTIMA.COM`. No further resolution needed there.
- [x] **`canMoveToTile`'s real helper named, and a stale note
      corrected**, done 2026-09-14: `sub_17F96` →
      **`findMonsterAtPosition`** — loops `updateMonsterAI`'s 32-slot
      overworld monster arrays looking for an occupant at a given
      (X,Y); `canMoveToTile` calls it to refuse moving onto a tile
      another monster already occupies. This is the overworld
      counterpart to combat's `findCombatantAtPosition`.
      **Correction**: an earlier note guessed `sub_128F2` was
      `canMoveToTile`'s *other* helper — it isn't (`canMoveToTile`
      only calls `findMonsterAtPosition`). `sub_128F2` turned out to
      be something unrelated but useful on its own: **`getDungeonTileAt`**,
      the dungeon-map counterpart to `getMapTileAt`, reading a tile
      byte from a loaded per-level dungeon buffer at a computed
      offset (packed X/Y plus `_dungeonLevel<<8`, base `0x900`).
- [ ] Trace the overworld/town/dungeon map file loader against the
      confirmed filename list (all 19 `.ULT` files, `DUNGEON.DAT`) —
      `drawTileGrid`'s confirmed 64-byte-tile/11×11-grid shape is a
      strong lead for the combat-arena renderer specifically (exact
      dimension match with `CNFLCT_*.ULT`).
- [x] **`SHAPES.ULT`/`CHARSET.ULT` load sites found** — done, see the
      resolved `drawCharGlyph`-buffer mystery in `ULTIMA.COM`'s open
      items above (`entryFromBootup` loads both, back-to-back into one
      contiguous buffer). Still open: what each byte of `SHAPES.ULT`'s
      5,120-byte tile-shape data actually encodes hasn't been traced
      (just the load site and size).
- [x] **The main map-drawing routine found and named**, done
      2026-09-14: `drawMapViewport` (`0x127CD`, a labeled location
      inside `entryFromBootup`'s own function chunks, called
      extremely widely) draws the visible 11×11 tile grid centered on
      `_partyPosition-(5,5)`, with the confirmed 64-tile-per-axis
      wraparound. Its tail end is where the self-modifying-code
      bookkeeping flagged as an open mystery earlier in this project
      actually lives — it backs up several code bytes and patches
      `_currentTransport`'s value directly into an instruction operand
      at `loc_124FF+1`, presumably parameterizing a later re-entry.
      The exact purpose of that patch isn't traced further. Also
      named `invertFullScreen` (`0x171C1`, `xorScreenRegionWithPattern`'s
      hardcoded-0xFFFF full-screen twin, used by `enterShrine`'s
      "Shazam!" fanfare and others) and, at moderate confidence,
      `teleportPartyWithFanfare` (`0x15B51`) — redraws the map,
      double-flashes the full screen with a sound cue, and moves
      `_partyPosition` to a new position from a lookup table. Shape
      strongly suggests a scripted "magically transported" effect
      (a whirlpool being the obvious candidate) but the index it's
      keyed on (`word_11324`) wasn't traced back far enough to confirm
      that specifically.
- [x] **`castSpell` note updated** — its real address is `0x15D83`;
      already identified and fully traced earlier this session (see
      the spell-table and combat-damage findings above).
- [x] **Temple interactions named**, done 2026-09-14:
      `showTempleMenu` (`0x1A692`, the "Clerical Healing\nSacraments:"
      screen reached from `cmdEnter` at a temple location) dispatches
      through `TEMPLE_COMMAND_TABLE` (`0x1A6C0`) to `templeCure`
      (`0x1A6D4`, gold cost `0x100`), `templeHeal` (`0x1A761`,
      `0x200`), `templeResurrect` (`0x1A7B6`), and `templeRecall`
      (`0x1A833`) — the temple counterparts to `spellAlcort`/
      `spellSanctuMani`/`spellAnjuSermani`'s Cleric-spell effects,
      each confirming its own gold cost via `promptYesNo` (`0x1A561`,
      a shared Y/N confirmation) and `deductGoldIfAffordable`
      (`0x1A587`, a shared cost-check-and-pay helper — BCD-subtracts
      `_gold` if affordable, refuses otherwise). **Left open**: each
      handler's actual character-effect *past* the payment gate wasn't
      independently re-traced against its corresponding spell's own
      code — presumed equivalent by name/shape, not verified
      byte-for-byte identical.
- [x] **`RosterEntry` extended with `_foodSubCounter` (offset `0x20`)**,
      done 2026-09-14 — real character-state-manipulation evidence,
      exactly the kind this item asked for. Tracing
      `processPartyTurnEffects`'s per-turn hunger handler
      (`applyHungerTick`) showed the previously-unlabeled 1-byte gap
      between `_experience` (ends `0x20`) and `_food` (starts `0x21`)
      is a real, meaningful field: a fixed-point accumulator
      decremented every turn, only cascading into `_food`'s own
      decrement on underflow — food drops slower than 1 unit/turn.
      Applied to both `ultima_exodus.idb` and `ultima_bootup.idb`,
      struct size unchanged (`0x40`, filled an existing gap). Also
      confirmed the full per-turn party effects cycle in one pass:
      class-gated MP regen (`computeMaxMagicPointsFromAttribute`,
      newly named — `floor(decimal(attribute)/2)` as each class's MP
      regen ceiling), hunger/starvation (5 damage + a status flash on
      `_food` reaching 0), poison damage (1 damage + flash + "Poisoned!"
      each turn while `_status == 'P'`), and natural HP regeneration
      (BCD `+1` toward `_maxHitPoints` when not poisoned). Still open:
      `_maxHitPoints` itself remains a same-session inference (see its
      own struct note) and armour/weapon-owned array entries beyond
      index 0 are still unconfirmed.
- [x] **Town shops/NPCs identified**, done 2026-09-14 — a big batch,
      all found via `cmdEnter`'s building-tile branch (tile `'@'` =
      `0x40`), which dispatches through a newly-named 8-entry
      `TOWN_BUILDING_TABLE` (`0x18028`) indexed by the party's Y
      position `& 7` — which of 8 shop/NPC types a town building is
      depends on its Y coordinate mod 8, a fixed pattern reused across
      every town. Every handler was identified from its own on-screen
      welcome text, not guessed from position: `showTavernMenu`
      (buy drinks for gold, each tier printing a different rumor —
      classic Ultima tavern-rumors mechanic), `showGrocerMenu` (buy
      food, confirms `_food` again), `showTempleMenu` (already named),
      `showWeaponsShopMenu`/`showArmourShopMenu` (buy/sell, structurally
      identical to each other), `showGuildMenu` (Keys/Torches/
      Powders/Gems), `showOracleMenu` ("Radrion, Prophet of Life!" —
      the cryptic-hint NPC; its dialogue directly confirms the
      Marks/Cards quest matching the `_marksAndCards` RosterEntry field
      found earlier this session), and `showStableMenu` (buy horses,
      `partySize*200gp`). Function-naming jumped from 99/144 to
      106/144 in this one batch. `showGrocerMenu`'s cost-calc helper
      (`promptForQuantity`) and the weapons/armour shops' inventory
      listings (`listWeaponsShopInventory`/`listArmourShopInventory`)
      were named in later commits — see below for a genuine
      progression-mechanic discovery those turned up: both shops
      unlock a second, better-goods page ('+2 Chain'/'+2 Plate' armour,
      '+2 Axe'/'+2 Bow'/'+4 Swd' weapons) only when
      `_savedOverworldPosition == 0x25h` — one specific, secret town
      — permanently setting `byte_114CA`/`byte_114CB`, the exact flags
      `readyWeapon`/`wearArmour` separately check to widen a
      character's max equippable tier. A real "find the secret shop
      to unlock better gear" mechanic, confirmed end-to-end from both
      the unlock side and the consumption side of the same two flags.
- [x] **The win condition / ending sequence found and named**, done
      2026-09-14: `victorySequence` (`0x1A4B9`, a labeled location
      inside `sub_17B54`, reached via `sub_17B54-498`). Prints
      "Congratulations!\n Thou hast\n compleated\nExodus: Ultima 3\n
      in", the move count (`printMoveCount`, `0x1A46F` — the same
      4-byte BCD counter `incrementMoveCounter` ticks), "Report thy
      feat!", a 21-flash screen fanfare (`xorScreenRegionWithPattern`,
      `0x1A491`, a generic large-region CGA XOR helper parameterized
      by a caller-supplied pattern — distinct from the simpler,
      always-0xFFFF `invertScreenRegion`), then the complete epilogue
      text line by line: "And so it came to pass that on this day
      EXODUS, hell-born incarnate of evil, was vanquished from
      Sosaria. What now lies ahead in the ULTIMA saga can only be pure
      speculation! Onward to ULTIMA IV!" — before calling
      `autoSaveGameState` (see below) and setting game mode to 1.
      **What triggers entry found immediately after**: `attemptExodusSequence`
      (`0x1764A`) — **Ultima III's legendary Exodus puzzle**. Combat
      cannot defeat Exodus at all; instead, standing on one of 4
      control-panel tiles (`'|'`, `0x7C`) and choosing a letter from a
      "D, S, L, M:" menu only advances a step counter
      (`word_164A0`) if you're saying the right word AND facing the
      right tile AND — checked directly against `_marksAndCards`
      (`RosterEntry+0x0E`) — actually holding the matching Card for
      that step. All 4 cards come from `obtainCard` (`0x17607`),
      which sets one `_marksAndCards` bit based on where you pick a
      card up. Reaching all 4 correct steps in sequence
      (`word_164A0 == 4`) jumps straight into `victorySequence`. This
      directly explains `showOracleMenu`'s riddle dialogue found
      earlier this session ("The cards their\nsuits do number...").
      Confirmed at the mechanism level; the exact
      letter-to-card/position mapping (which of D/S/L/M goes with
      which of the 4 steps) wasn't decoded digit-by-digit.
      **Renamed `autoSaveOnDeath` → `autoSaveGameState`**: finding it
      called from the win sequence too (not just death/party-wipe)
      showed the old name was too narrow — it's a general "commit
      game state to disk at a major transition" primitive.
      Also named `promptForQuantity` (`0x17D1A`, a 4-digit number-entry
      prompt used by `showGrocerMenu` and one direct `sub_17B54` call
      site, similar in shape to `promptForNumberEntry` but wider).
- [x] **A whole second combat-damage system found: dungeon monster
      special attacks**, done 2026-09-14. `attemptSpecialMonsterAttack`
      (`0x19244`, called twice from `sub_17B54`) dispatches by
      `_conflictMonsterClass` to `attemptPoisonAttack` (`0x19360`,
      classes `0x0E`/`0x1C`/`0x1E` — 25% chance to poison the target)
      or `attemptStealAttack` (`0x193A2`, class `0x17` — a Thief-type
      monster stealing a random owned weapon or armour item,
      confirmed via its own "Pilfered!" string), then rolls a to-hit
      check scaled by the target's `_armourIndex` and applies
      `applyDungeonMonsterDamage` (`0x192AF`) on a hit. That last one
      is a genuinely separate damage-and-kill resolution path from
      `applyCombatDamage` (the arena-combat system documented much
      earlier this session) — it rolls damage from `MONSTER_HP_TABLE`
      plus a dungeon-level-scaled bonus, applies it via
      `damageCharacterHP` twice, and on death converts the character
      to Ashes on the map display before calling `checkPartyWipedOut`.
      This confirms dungeon monsters that reach the party while
      wandering a level (as opposed to a formal arena encounter) use
      an entirely different attack/damage pipeline. Function-naming
      jumped from 125/144 to 129/144 in this one batch.
- [x] **RESOLVED 2026-09-14**: cross-referenced the external doc's
      guessed `EXODUS.BIN` fixed-data-table file offsets against the
      now-fully-disassembled `ultima_exodus.idb` (file offset 0 =
      linear `0x10100`, a `.COM`-style load). All five resolved to
      already-understood structures, none needed new investigation
      from scratch:
  - **"look" command strings** (`0x6566` → linear `0x16666`): lands
      exactly on `GAME_NAME_TABLE`'s string blob, already fully
      cataloged this session (`fix_game_name_table.py`) — 146 strings
      covering terrain/objects/NPCs/monsters/spells/etc.
  - **castle/town/dungeon coordinates** (`0x15E1`/`0x15E5`/`0x15F9` →
      linear `0x116E1`/`0x116E5`/`0x116F9`): these three "separate"
      offsets are just three different indices into the single,
      already-named 19-entry `LOCATION_TILE_TABLE` (word array of
      `_partyPosition`-format positions for every named overworld
      location) — not three distinct tables as the external doc's
      byte-level guess implied.
  - **"moongate" coordinates** (`0x184D`/`0x1855` → linear
      `0x1194D`/`0x11955`): genuinely new — these were undefined bytes
      in the IDB, referenced only via raw hex offsets
      (`[bx+194Dh]`/`[bx+1955h]`) inside `teleportPartyWithFanfare`
      and `updateWhirlpoolPosition`, whose rename notes already
      explain the mechanism in full (the game's **moving whirlpool**,
      not a moongate): 8 possible on-map positions, cycled over time,
      selected by `word_11324`'s two independently-timed index bytes.
      Named the two 8-byte tables `WHIRLPOOL_X_TABLE`/
      `WHIRLPOOL_Y_TABLE` to close the loop — the external doc's
      "moongate" label was simply a mismatch with Ultima III's actual
      mechanic (there's no moongate system in this game).
- [x] **`AMBROSIA.ULT`'s role confirmed** — done 2026-09-14, while
      naming the last few functions (see `teleportToAmbrosia` below):
      it's loaded directly by name when the party falls into a
      specific whirlpool, replacing the loaded Sosaria map with
      Ambrosia's. `FAWN.ULT`/`EXODUS.ULT` remain unresolved — not
      found referenced by name in any code reached this session; may
      be loaded indirectly (a computed filename) or simply unused
      leftover strings.
- [x] **Two distinct whirlpool-type map features found and named**,
      done 2026-09-14, closing out the last few unnamed functions in
      `ultima_exodus.idb`:
      - The regular whirlpool: `updateWhirlpoolPosition` (`0x120AE`)
        animates it on two independent modular timers (X every 11
        turns, Y every 3), and `teleportPartyWithFanfare` (`0x15B51`,
        confirmed earlier) triggers when the party's tile reads
        `0x88` — the whirlpool's own tile value, restored to plain
        terrain (`4`) once it moves on. **Self-correction**: an
        earlier note here had the old/new tile values backwards
        (said the old tile got `4` and confirmed nothing about
        `0x88`'s meaning); re-reading the code the right way round
        shows `4` marks the vacated position and `0x88` marks the
        whirlpool's current one — which is also now independently
        confirmed as the exact value `teleportPartyWithFanfare`
        checks for.
      - A **second, distinct** whirlpool: `updateAmbrosiaWhirlpoolPosition`
        (`0x17347`) moves via a random walk (not fixed timers), and
        calls `teleportToAmbrosia` (`0x12168`) directly when it lands
        on the party. `teleportToAmbrosia` is the secret-continent
        transition — prints the "huge swirling WhirlPool" text, loads
        `AMBROSIA.ULT` in place of the current map, and drops the
        party at a fixed landing point. It also handles the return
        trip: falling into this same whirlpool while already on
        Ambrosia instead reloads `SOSARIA.ULT` and restores the
        saved position ("You made it!"). Ultima III apparently has
        more than one whirlpool on the map, and only this specific
        one is the Ambrosia gateway.
- [x] `DUNGEON.DAT` (1,866 bytes) — **confirmed 2026-09-14: raw x86
      machine code, not data** — the dungeon first-person-view
      renderer, loaded straight into the `start` buffer and executed
      directly. See [file-formats.md](file-formats.md) and
      `drawDungeonView`'s entry above.
- [x] **RESOLVED 2026-09-14**: `MOVES.ULT`'s internal format. Confirmed
      from `titleScreenAndMainMenuLoop` in `ultima_bootup.idb`: its two
      `0x200`-byte halves are an index table and a value table that
      together script a 512-step "attract mode" poke sequence into
      `DEMO.ULT`'s loaded buffer, one (offset, halved value) pair per
      title-screen frame, with index `0xFF` reserved as a pause
      marker. See [file-formats.md](file-formats.md#movesult--scripted-demo-playback-poke-table-1024-bytes).
- [x] **RESOLVED 2026-09-14**: `checkDebugModeFlag`'s equivalent in
      this IDB, found and named. It's at `0x15000`-`0x15003`, right
      after `drawCharGlyph` (`0x14F90`-`0x15000`) — exactly the same
      relative layout as `ultima.idb` and `ultima_bootup.idb`. Body is
      `mov al, 0FFh; retn` (opposite polarity from the other two
      binaries' `mov al, 0`, though moot either way). **Zero callers
      anywhere in `EXODUS.BIN`** — unlike its counterparts, which are
      each called once from their respective title/boot sequences.
      Consistent with `EXODUS.BIN` (the game engine, entered only
      after the title screen/character creation are done) simply
      having no boot-logo-animation step for this hook to gate.
      Defined via `ida_funcs.add_func()`; `ultima_exodus.idb` now
      145/145 functions named.

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
