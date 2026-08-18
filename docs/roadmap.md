# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current across
sessions, unlike a one-off todo list.

## Immediate (IDB hygiene)

- [x] Ran `ida_scripts/fix_inline_strings.py` with `DRY_RUN = False` —
      fully applied and verified, see the "IDB hygiene gap fixed" and
      `_gems`/`_helmsOwned` entries below.
- [x] Ran `ida_scripts/fix_access_file_calls.py` — dry-run first (all
      20 remaining sites already showed clean 8-byte filenames, no
      code-garbage left — the earlier note about raw garbage was
      resolved by other work in the meantime, likely the
      `command_jump_table` fix), then applied for real: all 20
      CALL→data graph edges fixed. Surfaced 6 new filenames
      (`PICOUT`/`PICTWN`/`PICCAS`/`PICDNG`/`PICSPA`/`PICMIN`), traced
      and documented as the title/demo attract-mode slideshow — see
      [file-formats.md](file-formats.md#pic--full-screen-cga-art).
- [x] Grepped the refreshed asm for any *other* `pop <reg>` pattern
      immediately at a `proc near` start (the giveaway of this trick —
      a `pop` with no matching `push` earlier in the same function,
      since it's popping the *caller's* return address, not a locally
      saved register) — **negative result, confirmed**: only
      `access_file` (`pop bx`) and `write_string` (`pop ax`, already
      known) match. No third function using this trick.

## High value / next session

- [x] **Run `ida_scripts/resolve_command_jump_table.py`** — done (by
      hand, in the IDB directly, not via the script's non-dry-run path).
      `off_10730` is now a fully resolved 26-entry table; all 26 targets
      are named procs (`attack` .. `zstats`). Corrected the standing
      hypothesis in the process: the orphaned `TLKXFF` loader
      (`sub_122D5`) is **not** one of the 26 targets — there's no Talk
      key (`T` = `transact`). It's called directly from `enter` on
      VILLAGE/TOWN/CASTLE. Full writeup in
      [overview.md](overview.md#resolved-a-z-command-jump-table-and-the-tlkxff-loader).
- [x] Run `name_talk_file_loader.py` — done. `off_10730` →
      `command_jump_table`, `sub_122D5` → `load_talk_file`. That
      one-off script has since been retired: renames now accumulate in
      `ida_scripts/apply_renames.py` (a single growing, git-friendly
      list) instead of a new standalone script per finding — see that
      file's docstring. Struct-member findings get the same treatment
      in `ida_scripts/apply_structs.py`.
- [x] Run `ida_scripts/apply_renames.py` — done. `sub_1154E` →
      `print_indexed_shop_string`, `sub_153F4` → `print_char`.
      **Convention going forward**: `apply_renames.py` is kept with
      `DRY_RUN = False` (Paul's call, 2026-08-17) — new entries take
      effect as soon as they're added and the script is re-run, no
      separate dry-run confirmation step. Still sanity-check a new
      entry's address/name before adding it, since there's no
      dry-run safety net anymore for this file specifically.
      `apply_structs.py` hasn't had this decision made for it yet —
      treat it as still dry-run-first until Paul says otherwise.
- [x] Chase the `tlkx???` 256-vs-384-byte discrepancy — **settled, not a
      bug**: `access_file` hardcodes FCB `record_size=1` for every
      caller so `cx=100h` is a literal 256-byte count; `random_record`
      (which would let a later read resume mid-file) is never written
      anywhere, so every read starts at offset 0 and there's no second
      read; and the destination buffer (`word_17886`=`0x2800`) only has
      256 bytes of reserved scratch space before the next buffer
      (`monsters_ptr`=`0x2900`) starts. Conclusion: 256 bytes is correct
      for this DOS port; the wiki's 384 likely describes the Apple II
      original instead. Full trace in
      [file-formats.md](file-formats.md#256-vs-384-byte-discrepancy-traced-and-settled).
- [x] **Traced the talk-buffer consumer** — `print_indexed_shop_string` (asm 3383,
      called only from `transact`) walks the buffer to the `index`-th
      null-terminated string and prints it via `write_character`, which
      unconditionally does `and al,7Fh` on every character it ever
      prints (asm 12645) — that's the ROT-128 "decrypt," and it's not a
      dedicated step, just an incidental side effect of the universal
      char-output routine (confirmed no separate decrypt loop exists
      anywhere in the binary). This means `TLKXFF` data isn't shown via
      a walk-up "talk" interaction at all (still no such command) — it's
      shop response/item text shown during `transact` with a
      shopkeeper-type NPC. Full chain and reasoning in
      [file-formats.md](file-formats.md#consumer-traced-read-out-by-transact-not-a-walk-up-talk).
      Along the way, confirmed `sub_10A30` (asm ~1710, not
      `canMoveToTile` at asm ~6821 — separate function, corrected in
      overview.md) is the movement-key dispatcher, and that
      `canMoveToTile` itself only does terrain-legality/effect checks
      (fields, ships, rockets) with no monster/NPC special-casing.
- [x] Now that the 26 command-handler procs are exposed, most of their
      internals are still unnamed `sub_XXXXX` helpers (proc/sub counts
      jumped from 93/58 to 145/79 after resolving the table) — sweep
      these for names once a few are read through. **Done** — see the
      completed sweep entry further down (73/73 named).
- [x] Locate the embedded overworld tile graphics — **found via a
      better path than the file-offset translation originally
      planned**. Paul spotted that the IDB already has a resolved
      `TILE_OFFSETS` table (asm ~19157) giving real per-tile addresses;
      using that instead of `ida_loader.get_fileregion_ea(0x7C43)`
      avoided a base/stride mismatch the first script draft had (see
      git history — the first version's visualization looked like
      noise because of it). Confirmed layout: 64 tiles, 66 bytes each
      (2-byte header `04h,10h` + 64 bytes CGA Linear 2bpp pixel data),
      contiguous from `byte_17A40`. Full writeup in
      [file-formats.md](file-formats.md#ultimaiiexe--embedded-overworld-tiles).
- [x] Visually confirmed the tile decode — all 64/64 resolved cleanly
      after the second script fix (see project memory / script
      docstring for the `DataRefsFrom` multi-word-declaration gotcha).
      Tile 6 (wiki: town) renders as an unambiguous twin-tower castle
      icon; tiles 0/1 (water/swamp) show plausible diagonal ripple
      motifs; 20/40/63 render as coherent sprites though not yet matched
      to wiki names. Full detail in
      [file-formats.md](file-formats.md#ultimaiiexe--embedded-overworld-tiles).
- [x] Applied `tile_00`..`tile_63` renames (`label_tile_graphics.py`,
      `DRY_RUN = False`) — Paul deliberately kept these generic rather
      than renaming to semantic names.
- [x] Fetched the wiki's full 64-entry tile ID legend and cross-checked
      several against our own pixel decode (20=Rocket, 40="I / Door" —
      an exact serif-I glyph match — 63=Thief, on top of the
      already-confirmed 0/1/6). Full table in
      [file-formats.md](file-formats.md#full-tile-id-legend-0-63-and-the-tileid-enum).
- [x] Ran `ida_scripts/create_tile_id_enum.py` — done. `TileId` enum
      created with all 64 members (`TILE_WATER = 0`, `TILE_TOWN = 6`,
      etc.) so semantic tile names are available wherever a raw tile-ID
      literal shows up in code, without renaming the `tile_NN` graphics
      symbols (Paul's call — enum for semantics, `tile_NN` stays as the
      graphics-data name). Two IDA 8.3 API issues hit and fixed along
      the way — `idc.hexflag()` missing (moved to `ida_bytes.hex_flag()`,
      third `idc`-wrapper casualty this project, see project memory) and
      a SWIG `OverflowError` from passing literal `-1` to
      `ida_enum.add_enum`'s `size_t idx` param (fixed: pass
      `idaapi.BADADDR` instead, the properly-unsigned form of the same
      "append" sentinel). Note as of this pass: `ultima2.idc`'s `Enums()`
      block is still empty — the `.idc` export hasn't been refreshed
      since the enum was created (git shows it unchanged while `.asm`
      is), so re-export it next time for the repo to catch up.
- [ ] Now that `TileId` exists: cross-check tile byte size against the
      "divide by 4" tile-ID encoding note elsewhere in file-formats.md,
      and consider going through `canMoveToTile`/`draw_map` etc. to
      apply the enum to existing raw tile-ID comparisons where it's
      unambiguous which ones are actually tile IDs vs. unrelated bytes.
- [x] Fetch the ModdingWiki "Ultima II Monster Format" page — **doesn't
      exist**, confirmed redlink on the summary page. No external
      source for `monx??`. Went ahead and wrote up what the
      disassembly already tells us instead (256-byte struct-of-arrays,
      5 of an estimated ~8 on-disk fields located but not decoded, plus
      7 separate runtime-only per-monster fields already reasonably
      well understood from earlier tracing work) — see
      [file-formats.md](file-formats.md#monx--monsternpc-data).
- [x] Look for a wiki page on the `player` save format — **also
      doesn't exist**, same redlink check as above ("Ultima II Save
      Game Format"). No cross-check available; the `Savegame` struct
      layout stays disassembly-only, see the item below.
- [x] Decode the on-disk `_mapMonsters` fields — **6 of them found and
      decoded** (one more than expected, `+0xA0`), by reading `attack`
      (asm 8115-8257), `cast`'s monster-finder `sub_1330C` (asm
      8950-9044), and `offer` (asm 10164-10219). Big result: monster
      type (`+0x60`) is literally `TileId × 4` — same ×4 encoding as
      `mapx??` terrain, confirmed via exact value matches
      (`TILE_MINAX`, `TILE_GUARD`, `TILE_THIEF`, `TILE_FIGHTER`).
      Bonus: decoded 3 previously-unplaced `Savegame` fields in the
      process — `field_2E`=Torches, `field_2F`=Keys,
      `field_30`=Thieves' Tools, all cross-checked against both their
      monster-kill drop site and their separate spend site elsewhere
      in the code. Full writeup in
      [file-formats.md](file-formats.md#monx--monsternpc-data).
- [x] Run `ida_scripts/apply_structs.py` — done. `field_2E`→`_torches`,
      `field_2F`→`_keys`, `field_30`→`_thievesTools`, confirmed in the
      refreshed `.asm`. **Convention update**: Paul ran this one with
      `DRY_RUN = False` too (previously only `apply_renames.py` had
      this), so both master scripts now default to applying
      immediately — still propose new entries and get sign-off in chat
      before adding them, same as `apply_renames.py`'s convention.
- [x] Follow-up leads from the decoding pass — **all resolved**:
      - `field_A5` (Fighter-kill drop) — resolved: it's `_helmsOwned`,
        spent by `view` ("VIEW WITH MAGICAL HELM!").
      - `+0xC0`-`+0xFF` (remaining 64 bytes) — resolved: they're
        `_monsterTempX`/`_monsterTempY` (temp scratch during position
        swaps), see the `_mapMonsters` split writeup.
      - On-disk `+0x00`/`+0x20` vs. runtime-only `+0x137`/`+0x157` —
        resolved, premise was wrong: same field, not two copies (see
        the `_mapMonsters` split writeup's segment-offset correction).
      - The Thief/Fighter kill branches touching `[idx+0D6h]` —
        resolved: it's `player._ringOwned[]` (the 16-element treasure
        array), and tracing it fully revealed `attack`'s complete
        monster-kill drop table (Thief also drops a random treasure
        item on top of tools; Fighter also drops torches on top of the
        helm; Wizard kills drop a Wand or Staff, 50/50 — a whole new
        finding, not previously documented at all). Full table in
        [file-formats.md](file-formats.md#monx--monsternpc-data).
      - `_monsterGlyphTile` (`+0x80`) vs. `_monsterType` (`+0x60`) —
        resolved, confirmed genuine companions: type is the
        identity/dispatch value, glyph is a separate copy for the map
        display tile, cleared together on death.
      - The "killed Minax" branch's transposed-(Y,X)-vs-(X,Y) map
        write — fully traced: turns out `attack` has two entirely
        separate "killed Minax" paths (melee vs. spell-death), and the
        transposition is deliberate, structured code unique to the
        spell-death path, not a disassembly artifact. Whether it
        reflects real design intent or an authoring slip can't be
        settled from static analysis alone (would need runtime
        observation) — treating this as closed at the "mechanism fully
        understood" level. Bonus find: spell-killing Minax gives a
        shorter victory message ("SHE'S GONE!!!") than melee-killing
        her (`minax_death_sequence`'s full "MINAX IS DEAD!!" text).
- [x] **`_mapMonsters` struct-of-arrays fully split and named**
      (`ida_scripts/split_map_monsters.py`, dry-run then applied,
      2026-08-18) — bespoke splitting script as anticipated, similar
      shape to `resolve_command_jump_table.py`. Along the way, caught
      and corrected a real error in an earlier session's notes: the
      "7 runtime-only per-monster fields at `+0x137` through `+0x217`,
      not saved to disk" turned out to be a misunderstanding —
      `_mapMonsters` itself sits at **segment-relative offset `0x137`**,
      so those "runtime" offsets are exactly the same 6 on-disk fields
      plus 2 more inside the previously-"unmapped" `+0xC0`-`+0xFF` span,
      just reached via a different (raw segment-literal) addressing
      idiom in some code paths. There's no separate unsaved region —
      the entire 256-byte buffer is exactly 8 fields × 32 bytes, no
      gaps. Two other claimed offsets (`+0x177` cooldown, `+0x1B7`
      cached-tile) didn't appear anywhere in the current `.asm` and
      were dropped as unverifiable rather than carried forward. All 8
      fields now have clean names (`_mapMonsters`/`_monsterMapY`/
      `_monsterSpellHP`/`_monsterType`/`_monsterGlyphTile`/
      `_monsterOfferFlag`/`_monsterTempX`/`_monsterTempY`), confirmed
      in the refreshed `.asm` — including the raw-literal sites, which
      automatically picked up the new symbolic names once the data
      items existed. Full writeup in
      [file-formats.md](file-formats.md#monx--monsternpc-data).
- [x] Decode the `sub_XXXXX` helpers shared across the 26 A-Z command
      handlers, ranked by reuse (functions called from more than one
      command are higher-confidence and higher-value to name first).
      8 decoded with proposed names — `rand_byte`, `find_target_monster`,
      `read_digit_keypress`, `print_indexed_menu_string`,
      `alert_town_guards`, `try_spend_gold`, `play_hit_sound`,
      `play_tone_sweep`. Full table in
      [overview.md](overview.md#command-handler-shared-helpers-decoded-and-applied).
- [x] Ran `ida_scripts/apply_renames.py` with the above 8 entries —
      confirmed in the refreshed `.asm`: `rand_byte`,
      `find_target_monster`, `read_digit_keypress`,
      `print_indexed_menu_string`, `alert_town_guards`,
      `try_spend_gold`, `play_hit_sound`, `play_tone_sweep`.
- [x] `print_indexed_menu_string`'s backing table at `cs:4C60h` —
      **Paul formatted it directly in IDA and pasted the result**,
      already named `TEXT_STRINGS` in the IDB. All 75 entries decoded
      and cross-checked against every caller's index formula (location
      descriptors, weapons, armor, spells, treasure items, attributes,
      races, classes). Full table in
      [overview.md](overview.md#text_strings--the-cs4c60h-table-fully-decoded).
      Resolved 4 more `Savegame` fields in the process:
      `field_2B`=readied weapon, `field_2C`=readied armor,
      `field_2D`=readied spell, and — closing out a loose end from the
      `_mapMonsters` decoding pass — `field_A5`=gem count (spent by
      `view` to show the world map).
- [x] Ran `ida_scripts/apply_structs.py` with the 4 new `Savegame`
      field renames — confirmed in the refreshed `.asm`:
      `field_2B`→`_readiedWeapon`, `field_2C`→`_readiedArmor`,
      `field_2D`→`_readiedSpell`, `field_A5`→`_gems`.
- [x] Traced how `_readiedSpell` drives `cast` — full 9-spell dispatch
      table decoded (Light/Down Ladder/Up Ladder/Passwall/Surface/
      Prayer/Magic Missile/Blink/Kill), plus the wand-or-staff gate and
      per-spell charge consumption. Also resolved `byte_17435` = current
      dungeon depth level (confirmed via the shared "TO LEVEL <N>"
      message) and found that "cast Surface" and `klimb` share their
      entire return-to-surface implementation. Full table in
      [overview.md](overview.md#cast--how-_readiedspell-drives-spell-effects).
- [x] Three per-item "owned"/"charges" flag arrays surfaced this
      session (`ready`/`wear_armor`/`cast` tracing): `player.field_60[di]`
      (armor-owned, already a recognized struct member, just unnamed),
      `[di+76h]` (weapon-owned, **not even a recognized struct member
      yet** — raw displacement), and `player.field_80[di]` (per-spell
      charge count). Lower-hanging fruit than `_mapMonsters`'s
      struct-of-arrays fields since these are array-typed `Savegame`
      members, not a separate loaded file. **Resolved by the two items
      right below** (`_armorOwned`/`_weaponOwned`/`_spellCharges`).
- [x] Extended `ida_scripts/apply_structs.py` to handle array-typed
      struct members — new `"add_member"` (now takes `element_size` +
      `count`) and `"resize_member"` ops (delete + recreate, for
      members that already exist but at the wrong size — `field_60`/
      `field_80` were both auto-created as 1-byte scalars when they're
      really 7- and 10-byte arrays).
- [x] Ran it with the 3 proposed entries — confirmed in the refreshed
      `.asm`: `field_60`→`_armorOwned` (7 bytes), new `_weaponOwned`
      member at `0x76` (10 bytes), `field_80`→`_spellCharges` (10
      bytes). Full detail in
      [overview.md](overview.md#three-per-item-ownedcharges-flag-arrays).
- [x] `_weaponOwned`'s reference sites fixed up via
      `ida_scripts/fixup_weaponowned_stroff.py` — now show as
      `[di+Savegame._weaponOwned]` instead of raw `[di+76h]`. Took two
      passes: the first covered `ready`/`get`/`steal`/`zstats` (7
      sites), then a `transact` "buy weapon from shopkeeper" site
      turned up that the initial proc list had missed (asm ~4160-4172)
      — added and re-run, all sites now fixed. Cosmetic only, the
      struct member itself was already correct throughout.
- [x] `field_A1`/`field_A2` renamed to `_wands`/`_staves` (wand-owned /
      staff-owned counts, gate `cast` — "NEED WAND OR STAFF!" if both
      zero) — confirmed in the refreshed `.asm`.
- [x] Traced `loc_11451` fully — it's a second entry point into the
      already-named `map_get_monster_at?` proc (asm 3283-3309), not a
      standalone function. Given a dungeon level and a position, reads
      (or, via the returned pointer, writes) the raw dungeon tile byte
      there. Confirmed via all 5 real call sites: `cast`'s Down/Up
      Ladder, Passwall, Blink (already known), plus **`attack`'s
      separate dungeon-combat code path**, which turned out to use the
      low 3 bits of dungeon tile bytes as a second, independent
      monster-presence flag (extends the dungeon tile-format doc — see
      [file-formats.md](file-formats.md#dungeontower-format-mapx-number-ends-4-5)).
      Also resolved the Prayer/Kill shared-tail mystery as a side
      effect: both route into `attack`'s generic monster-death cleanup
      (`loc_12ED1`) — Kill unconditionally, Prayer only on a random
      success roll; they're an instant-kill "smite" pair, not two
      spells that coincidentally overlap. Full writeup in
      [overview.md](overview.md#loc_11451--the-shared-dungeon-cell-accessor).
      One thing this did NOT resolve: `map_get_monster_at?`'s own
      (non-`loc_11451`) entry point, used from the monster-AI wander
      loop, passes a monster's display-tile value where `loc_11451`'s
      callers pass a dungeon level — those aren't reconciled, since a
      tile-ID value doesn't fit the 0-15 level range. Separate open
      question.
- [x] Traced `cast`'s Down Ladder/Up Ladder "non-dungeon" branches —
      **the premise was wrong, not just incomplete**. There is no
      non-dungeon case: the earlier `_mapNum2>=4` gate already
      guarantees Tower (4) or Dungeon (5) by the time these branches
      run. The real `_mapNum2==4` check distinguishes **Tower vs.
      Dungeon**, which matters because the two number their levels in
      opposite directions. Both spells route into one of two shared
      primitives (increment-capped-at-15, or decrement-with-surface-
      at-0) depending on map type + direction needed. Along the way,
      corrected an earlier wrong assumption in `docs/overview.md` that
      Tower was `_mapNum2==0` (it's `4`; `0` is the overworld; Dungeon
      is `5`, not shared with Tower) — added a proper `_mapNum2` value
      table to `docs/overview.md` and fixed the now-stale
      `alert_town_guards` description that inherited the same error.
      Full trace in
      [overview.md](overview.md#down-ladder--up-ladder--tower-vs-dungeon-fully-traced).
- [x] Traced `sub_15FC3` and `sub_15E83` — both sound effects.
      `sub_15E83` is `cast`'s fail buzzer (descending tone sweep,
      called at all 4 failure points, cast-only). `sub_15FC3` is a
      generic "something magical happened" chime, reused across
      `cast`'s attempt sound *and* three unrelated trap/curse handlers
      (leg/arm paralysis, sleep spell) plus a fourth "magic missile
      trap" hitting the player — surfaced three not-yet-named magical
      item-protection flags (`player+0xA3`/`0xA4`/`0xAE`) as a side
      finding. Full trace in
      [overview.md](overview.md#sub_15fc3-and-sub_15e83--the-two-sound-effects-traced).
      Renamed and confirmed in the refreshed `.asm`: `sub_15FC3` →
      `play_magic_sound`, `sub_15E83` → `play_fail_sound`.
- [x] Traced `player+0xA3`/`0xA4`/`0xAE` — all three are magical-item
      protection flags for `end_of_turn`'s random trap encounters, same
      mechanic: item owned + `rand_byte()>=0x40` (~75%) resists, item
      owned but unlucky roll or item not owned both still trigger the
      trap. `0xA3`=Boots (leg paralysis), `0xA4`=Cloak (arm paralysis),
      `0xAE`=Green Idol (sleep trap) — item names cross-confirmed
      against `TEXT_STRINGS`. Full trace in
      [overview.md](overview.md#player0xa30xa40xae--magical-item-protection-flags-traced).
- [x] Added and confirmed in the refreshed `.asm`: `player+0xA3`→
      `_bootsOwned`, `player+0xA4`→`_cloakOwned`, `player+0xAE`→
      `_idolOwned`.
- [x] Traced `board`'s full vehicle-boarding requirements. Two
      preconditions (`_mapNum2<4` — no vehicles underground; `_playerTileId>=0x78`
      — must be on foot, confirmed as exactly the 4 class sprite tiles
      via the ×2 `TileId` encoding), then reads the tile stood on,
      matched exactly against Horse/Ship/Airplane/Rocket (`0x22/0x24/
      0x26/0x28` = `TileId` 17-20 ×2). Horse is free; Ship vs. Frigate
      is `field_AC`; Airplane is `field_A9`; Rocket is `field_A7`
      (Ankh, already known). All 4 successes route through
      `end_of_turn2`, a label *inside* `end_of_turn` (not a separate
      proc) that skips one call to `sub_15C95` — not chased further.
      Full trace in
      [overview.md](overview.md#boards-vehicle-boarding-requirements-fully-traced).
- [x] **IDB hygiene gap fixed** — Paul fixed the Ship/Airplane
      failure-path bytes directly in IDA. Revealed real text and
      **corrected an assumption from the initial trace**: both are
      rejection messages (`"THE CREW OF THIS SHIP WILL NOT LET YOU
      BOARD!"`, `"STRANGE YOU CAN'T GET IN!"`), not a harmless default
      — there's no "board a plain Ship" path at all, all three
      non-Horse vehicles are gated the same way (required condition or
      rejection). Corrected in
      [overview.md](overview.md#boards-vehicle-boarding-requirements-fully-traced).
- [x] `field_A7`→`_ankhOwned` renamed, confirmed in the refreshed `.asm`.
- [x] `field_AC`→`_frigateAllowed`, `field_A9`→`_planeAllowed` renamed
      and confirmed in the refreshed `.asm` — double-checked the
      which-is-which pairing against the code before applying, since
      it had been stated backwards in chat at one point.
- [x] Traced `sub_15C95` — just a sound effect (corrects the earlier
      "possibly turn/food bookkeeping" guess in `docs/overview.md`).
      Same shape as `play_hit_sound`/`play_magic_sound`/`play_fail_sound`:
      checks `byte_1795D`, plays one fixed tone via the shared
      low-level primitives, but a flat single beep, not a sweep. A
      generic "turn advances" tick played at the start of every full
      `end_of_turn`, skipped by callers (like `board`'s vehicle-board
      successes) that jump straight to `end_of_turn2`. Full trace in
      [overview.md](overview.md#play_tick_sound--the-turn-tick-sound-traced-and-renamed).
      Renamed and confirmed in the refreshed `.asm`: `sub_15C95` →
      `play_tick_sound`.
- [x] Traced `sub_14B26` — a screen-invert flash effect. XORs both
      interleaved CGA framebuffer segments (`0xB800`/`0xBA00`) via a
      private helper `sub_14B35`; XOR-based so calling it twice
      (bracketing pattern seen everywhere) flashes then restores.
      Reused well beyond the 4 trap handlers — also brackets an
      armor-damage encounter in `end_of_turn` and the "killed Minax"
      branch in `fire` — confirming it's general-purpose, not
      event-specific. Full trace in
      [overview.md](overview.md#flash_screen--the-screen-flash-effect-traced-and-renamed).
      Renamed and confirmed in the refreshed `.asm`: `sub_14B26`→
      `flash_screen`, `sub_14B35`→`xor_invert_cga_bank`.
- [x] Traced the `TEXT_STRINGS` location-descriptor block (indices
      1-18) — **dead data, confirmed, not just unconfirmed**. Checked
      all 23 real call sites of `print_indexed_menu_string`; every one
      lands in the weapon/armor/spell ranges (≥19) or is an
      already-known caller, none reach 1-18. The strings themselves
      have zero xrefs beyond their own declarations. Ruled out the
      `view`/`sub_129D2` hypothesis directly — read that routine in
      full, it's a pure map-drawing/flood-fill routine (repeated calls
      to `sub_12B65`, a screen helper) with no `write_string` or
      `print_indexed_menu_string` call anywhere in its body. Likely a
      cut/unused "status line" feature (terrain-under-player text) that
      never shipped, or reachable only from one of the 79 still-unnamed
      `sub_XXXXX` functions elsewhere. Full writeup in
      [overview.md](overview.md#text_strings-location-descriptor-block-traced--dead-data).
- [x] Traced the `TEXT_STRINGS` treasure-item block (46-61) — **major
      structural discovery**: the whole `Savegame` `0xA0`-`0xAF`
      cluster (named field-by-field across several sessions) is
      actually one 16-element array, `_ringOwned[0..15]` (named
      `_hasRing` at the time, renamed since — see below), displayed by
      `zstats`' `"ITEMS:"` inventory screen. 8 of 16 slots
      cross-check perfectly against fields already named independently
      elsewhere — including a new exact confirmation: `launch`
      requires `field_AB` (item 11 = Brass Button) to launch a Plane,
      "FUNNY THIS PLANE IS MISSING A BRASS BUTTON!" Also explains *why*
      `_planeAllowed`/`_frigateAllowed` gate what they do (they're
      really Skull Key/Blue Tassle ownership). Full table in
      [overview.md](overview.md#text_strings-treasure-item-block-traced--a-16-element-inventory-array-unifying-8-prior-findings).
- [x] **`fix_inline_strings.py` re-run, `DRY_RUN = False`, applied for
      real** — fixed `view`'s inline-data gap (and the space-travel/
      planet-name cluster in `launch`, ~10 sites). Hit and fixed a real
      bug in `find_terminator()`/the resync logic along the way (it was
      finding the correct `0x00` terminator, but a stale/malformed
      auto-created string literal left over from IDA's own heuristics —
      stopping short at a non-printable `0x8D` byte, mistaking it for
      the start of an opcode — was masking the fix; the already-broken
      "lea" garbage downstream was old damage the fix repairs, not
      something the script introduced). `view`'s full text recovered:
      `"VIEW",0x8D,"WITH MAGICAL HELM!"`.
- [x] **`_gems`/`0xA5` conflict resolved** — with `view`'s gap fixed,
      confirmed `field_A5` is spent by `view` on a *Magical Helm*, not
      a gem ("VIEW WHAT?" if zero, "VIEW WITH MAGICAL HELM!" +
      decrement on success). Renamed `_gems` → `_helmsOwned` via
      `apply_structs.py`, confirmed in the refreshed `.asm` (3 sites:
      `view`'s check+decrement, plus a previously-uncatalogued 25%-
      chance drop on killing a Fighter in `attack`, asm 5341-5348 — see
      [file-formats.md](file-formats.md#monx--monsternpc-data)).
- [x] **All 16 `_ringOwned[]` treasure-array slots now named.** Traced
      the last 3 vague ones: `_triLithium` (`0xAF`, Rocket *and*
      hyperwarp fuel — richest mechanic of the batch, see
      [overview.md](overview.md#text_strings-treasure-item-block-traced--a-16-element-inventory-array-unifying-8-prior-findings)),
      `_strangeCoin` (`0xAD`, spent by `negate_time`),
      `_brassButtonOwned` (`0xAB`, wasn't even a recognized struct
      member before — raw `player+0ABh`; gates launching the Plane).
      Added the 3 pure gaps (`_gemOwned`/`_redGemOwned`/
      `_greenGemOwned`, `0xA6`/`0xA8`/`0xAA`) from array position only
      — no independent reference exists for any of the three. Bonus
      find: a barkeep "buy a rumor" hint table independently confirms
      almost every item-requirement mapping in this arc. Kept
      `_planeAllowed`/`_frigateAllowed` as functional names rather
      than renaming to item identities (Paul's call). Also renamed
      `_hasRing` (`0xA0`) → `_ringOwned` for consistency, once traced
      usage (asm ~4159-4166, ~7376-7383) confirmed it's a boolean flag
      like its siblings, not a count.
- [x] **New IDA automation**: `ida_scripts/batch_run_and_export.py` +
      `run_ida_script.ps1` run any `apply_*.py`/fix script headlessly
      via `idat.exe -A` and re-export `.asm`/`.idc`, no GUI needed
      (IDA must be closed first — the `.idb` locks). Confirmed working
      2026-08-18. Two non-obvious fixes: `ida_loader.gen_file()` needs
      a real SWIG `FILE *` from `ida_diskio.fopenWT()`/`eclose()`, not
      a plain Python file handle; console output from `idat.exe -A`
      isn't reliable, so the driver logs every step to
      `batch_run_and_export.log` instead.
      **Hardened same day**: running `idat.exe` against an `.idb` the
      GUI already has open doesn't fail cleanly — it races the GUI's
      in-memory copy and can silently drop the *last* operation in a
      batch with zero error output (hit for real: `_ringOwned`'s
      `apply_structs.py` entry vanished this way). `run_ida_script.ps1`
      now does a pre-flight exclusive-open check on the `.idb` and
      refuses to launch `idat.exe` at all if something else already
      has it open.
- [x] Single-caller helpers inside the 26 command handlers, not chased
      this pass (lower priority — no cross-context confirmation
      available): `sub_15FE0`, `sub_15FA6`, `sub_172A0`
      (attack), `sub_15EAC` (fire), `sub_1361C` (get), `sub_16999`/
      `sub_15EC7` (launch). **All since named** in the completed
      `sub_XXXXX` sweep: `play_attack_sound`, `play_bump_sound`,
      `minax_death_sequence`, `play_cannon_sound`,
      `clear_picked_up_tile`, `setup_rocket_launch_display`,
      `play_step_tick` respectively.

## Medium term

- [x] **Pin down remaining `Savegame` struct fields — done
      (2026-08-18).** `_disableSave`'s "not yet placed" note was stale
      — it's already a named member at `0x37`; tracing it properly
      revealed it's genuinely dual-purpose (save-gate boolean +
      hyperwarp orbit-target index, 0-8=planet/0xA=deep space/9=a
      distinct Castle-quest value), documented rather than renamed.
      Named 7 more fields along the way: `_inSpace` (persisted
      "away from overworld" flag, checked at game-load),
      `_launchMapX`/`_launchMapY` (saved launch position for `klimb`
      to return to), `_ringQuestFlag`, `_patrolWaypoint`,
      `_offerRewardItems` (a previously-unknown 9-byte reward array —
      individual slot contents still not cross-referenced against any
      item table, small open follow-up), and `_enilnoOwned`
      ("ENILNO IS YOURS!", "ONLINE" backwards — a third `offer` quest
      reward alongside the Ring and the reward array, not previously
      documented at all). Also fixed a badly stale struct dump in
      overview.md that still showed old pre-rename names (`_gems`,
      `field_AB`/`AD`/`AF`) for the treasure array. Full struct layout
      in [overview.md](overview.md#savegame-asm-line-22-sizeof-0x100--256-bytes).
- [x] **Sweep of the unnamed `sub_XXXXX` functions — COMPLETE.**
      Ranked by call/jmp-site reuse rather than picked arbitrarily; all
      73 named, 2026-08-18, zero unnamed functions left in the binary.
      See
      [overview.md](overview.md#sub_xxxxx-sweep--unnamed-helpers-ranked-by-call-site-reuse)
      for the full list and evidence. Named clusters: CGA
      point/line-drawing primitives (`draw_line`/`plot_point`/
      `erase_point`/`clear_screen`/`clear_cga_bank`), PC-speaker
      primitives (`speaker_on`/`hold_tone`/`speaker_off`), the
      hyperwarp starfield animation and space-flight loop
      (`animate_starfield`/`draw_ship_marker`/`erase_ship_marker`/
      `next_star_coord`/`init_starfield`/`space_travel_command_loop`/
      `setup_rocket_launch_display`/`play_star_twinkle_sound`), `view`'s
      actual map-overview mechanism (`draw_world_map_overview`/
      `plot_map_icon_point`, resolves what `sub_129D2` does, previously
      only vaguely described during the location-descriptor dead-data
      investigation), `end_of_turn`'s trap handlers (`leg_paralysis_trap`/
      `arm_paralysis_trap`/`magic_missile_trap`/`sleep_trap`/
      `minax_curse_trap`/`random_item_loss_trap`/`consume_food`) and
      monster-AI helpers (`spawn_dungeon_monster`/
      `compute_monster_direction_to_player`/`update_patrol_marker`),
      `_flag1` → `_negateTimeDuration` (Negate Time's remaining-
      duration counter, confirmed via 3 sites), `minax_death_sequence`,
      and — the biggest find — **Ultima II's entire dungeon
      first-person view rendering pipeline**, fully identified end to
      end including all 10 individual wall-segment helpers:
      `render_dungeon_view` → `precompute_dungeon_corridor` →
      `draw_dungeon_corridor` → (`draw_corridor_wall_segment`/
      `draw_dungeon_door`/`draw_ladder_down`/`draw_ladder_up`/
      `draw_chest_icon`/`draw_ladder_rail` for the right probe;
      `draw_left_wall_segment`/`draw_left_door`/`draw_left_open`;
      `draw_ahead_wall_segment`/`draw_ahead_door`/`draw_ahead_open`) →
      `draw_dungeon_monster` → `draw_dungeon_monster_sprite`/
      `draw_sprite_row`. The 10 wall-segment helpers were precisely
      decoded (not guessed) by cross-referencing the dispatch bit
      logic against the already-documented dungeon tile format — see
      the table in overview.md. This is what the old "`sub_16xxx`
      cluster, dense, likely dungeon movement or combat" note used to
      point at — it's neither, it's the classic wireframe maze-view
      renderer. Full writeup in
      [overview.md](overview.md#ultima-iis-dungeon-first-person-view-fully-identified).
      Note: the old `sub_10A30` "movement-key dispatcher" entry point
      from an earlier session no longer exists as a separate function
      — its callers now resolve inside `end_of_turn` itself (likely
      absorbed when the `command_jump_table` fix reshuffled code
      boundaries); don't go looking for it.
- [ ] Rename the auto-named segments (`sg01a2`, `sg08e3`) once their
      contents/roles are clear, and rename `_picData` (misleadingly named
      after one file type when it's the shared FCB for all file I/O).
- [ ] Build a "string catalog" script/pass: since `write_string` call
      sites now carry the decoded text as a comment, a script to walk all
      xrefs to `write_string` and dump `(address, text)` to a text file
      would give a full game-text inventory for free — useful both as
      documentation and later as a resource file for the ScummVM port.
- [ ] Write a standalone (non-IDA) Python decoder for `tlkx???` files —
      once we have actual game data files to test against, not just the
      EXE — applying the ROT-128 decrypt. Useful independent of IDA.
      Use **256 bytes**, not the wiki's 384, per the confirmed trace in
      file-formats.md.

## Longer term (toward the C++ / ScummVM goal)

- [ ] Once `canMoveToTile` and the map/draw pipeline are understood,
      start sketching the clean-room C++ structure (data model first:
      `Savegame`, map/dungeon representations, monster/NPC state —
      these map fairly directly to ScummVM engine conventions).
- [ ] Identify all DOS/BIOS interrupt dependencies (`int 21h` FCB I/O,
      CGA video I/O, keyboard) as the porting boundary — these are what
      the ScummVM engine shim will need to replace.
- [ ] **Design note (Paul, 2026-08-18)**: the `_ringOwned[]` treasure
      array's 16 individual `Savegame` member names (`_ringOwned`,
      `_wands`, `_triLithium`, etc.) are an artifact of naming each
      slot separately while its meaning was worked out one at a time
      during disassembly — don't carry that 1:1 into the C++ port. The
      clean-room struct should instead have one generic array (e.g.
      `_items[]`) indexed by an `ItemType` enum (`RING`, `WAND`,
      `STAFF`, `BOOTS`, `CLOAK`, `HELM`, `GEM`, `ANKH`, `RED_GEM`,
      `SKULL_KEY`, `GREEN_GEM`, `BRASS_BUTTON`, `BLUE_TASSLE`,
      `STRANGE_COIN`, `GREEN_IDOL`, `TRI_LITHIUM`, matching
      `TEXT_STRINGS` order 46-61) — same shape as the `TileId` enum
      already used for tile/monster-type semantics. Applies more
      broadly too: several other `Savegame` fields were named
      individually during disassembly (`_armorOwned[]`/
      `_weaponOwned[]`/`_spellCharges[]`, the readied-item indices)
      that are really parallel per-type arrays and should get the same
      enum-indexed treatment in the C++ struct rather than keeping
      their disassembly-era per-field names.
