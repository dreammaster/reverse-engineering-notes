# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current across
sessions, unlike a one-off todo list.

## Immediate (IDB hygiene)

- [ ] Run `ida_scripts/fix_inline_strings.py` with `DRY_RUN = False`
      (already validated in dry-run — strings like `ORIGIN`,
      `PROUDLY PRESENTS`, `BY LORD BRITISH` decoded correctly). **Done in
      a prior session per the .asm already reflecting it** — confirm
      still current after any further analysis passes.
- [ ] Run `ida_scripts/fix_access_file_calls.py`, `DRY_RUN = True` first.
      Several `access_file` call sites are still raw code garbage (asm
      lines ~419, 615, 643, 787, 6971 as of this pass) while others
      already happen to show a clean 8-byte name — the script handles
      both, always repairing the CALL→data graph edge either way.
- [ ] Grep the refreshed asm for any *other* `pop bx` / stack-adjust
      pattern near a `proc` start — `access_file` was found this way
      (noticed while reading its body after the `write_string` fix). If
      one turns up, add a third `ida_scripts/fix_*.py`.

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
- [ ] Run `ida_scripts/apply_renames.py` (dry run first) — has two new
      pending entries from the transact/talk-buffer trace below:
      `sub_1154E` → proposed `print_indexed_shop_string`, `sub_153F4` →
      proposed `print_char`. Confirm/adjust the names before applying.
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
- [x] **Traced the talk-buffer consumer** — `sub_1154E` (asm 3383,
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
- [ ] Now that the 26 command-handler procs are exposed, most of their
      internals are still unnamed `sub_XXXXX` helpers (proc/sub counts
      jumped from 93/58 to 145/79 after resolving the table) — sweep
      these for names once a few are read through.
- [ ] Translate EXE file offset `0x7C43` (embedded overworld tile
      graphics, 0x40 tiles) to an IDB address via
      `idc.get_fileregion_ea(0x7C43)` in the IDA console, then label/type
      the tile array. Cross-check tile byte size against the "divide by
      4" tile-ID encoding note in file-formats.md.
- [ ] Fetch the ModdingWiki "Ultima II Monster Format" page (linked from
      the summary page, not yet pulled) to fill in `monx??` layout in
      file-formats.md.
- [ ] Look for a wiki page on the `player` save format specifically (the
      summary page names it "Ultima II Save Game Format" but we haven't
      located/fetched that page's URL yet) to cross-check the `Savegame`
      struct fields already identified.

## Medium term

- [ ] Pin down remaining `Savegame` struct fields — current layout stops
      being well-understood partway through the 256-byte struct.
      `_disableSave` is used but not yet placed.
- [ ] Name the 79 remaining `sub_XXXXX` functions. Best entry points:
      `canMoveToTile` and `sub_10A30` (movement/collision and command
      dispatch respectively — see the correction in overview.md, these
      are two separate functions, not one) and the `sub_16xxx` cluster
      (asm ~19113-20276, dense self-contained call graph, likely one
      subsystem — combat? dungeon movement?).
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
