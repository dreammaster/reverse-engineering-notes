# Roadmap

Prioritized list of what's investigated vs. still open for `yendor2.idb`
(`SW.EXE`). See [overview.md](overview.md) for narrative detail behind
each item.

## Not started (all of it — this was a setup-only session, 2026-09-14)

Roughly in likely investigation order, not a hard sequence:

1. **Locate and verify the per-character/item savegame struct**
   (`CURGAME`), now that `docs/Hex Hacking Item Guide.txt` (added by Paul
   2026-09-14) gives a strong external hypothesis to check against: 8
   item slots/character × 4 characters, each slot 4 bytes
   (`item_id`/`modifier`/`uses`/`unknown`), plus a near-complete 3-page
   item ID table. This is now the **lowest-risk, highest-value first
   target** — promoted above item #1 below because, unlike everything
   else in this list, we already have a candidate answer to verify
   rather than needing to derive one from scratch (`ultima1`'s
   `Savegame`-struct work is the closest parallel, but that started with
   no external reference at all). Candidate struct: the already-present
   but unidentified `Struc1` in the IDB — check whether its size/shape
   is consistent with this layout before assuming it's a match. See
   file-formats.md for full detail.
2. **Spot-check the 45 pre-existing function names** against their
   actual disassembly before trusting any of them further — they're
   from earlier, unfinished sessions and Paul has explicitly flagged
   they may not be accurate. Start with the highest-confidence-looking
   cluster (`FileEntry_*` + `loadWorldDat*`/`WorldDat_setBlock*`, cross-
   checked against the `WORLD.DAT`/`CURGAME` error strings) since that's
   the fastest way to build or lose confidence in the rest.
3. **Confirm single-executable vs. overlay/chain architecture.** No
   second executable filename or `INT 21h`/`4Bh` EXEC pattern spotted in
   a first string pass, but not specifically traced in code yet. Matters
   for how the eventual C++/ScummVM structure should be shaped —
   `ultima1` was 5 chained EXEs, `ultima2` was 1; this looks like
   `ultima2`'s shape but isn't confirmed.
4. **EMS/expanded-memory subsystem.** Already-named `MapUnmapPages`/
   `InitMemory` plus the mandatory-EMS error strings (`EMM Ver 4.0`,
   `1MB Expanded RAM`, `EMMXXXX0` device signature) suggest this game
   pages large assets (especially the 12MB `PICTURES.VGA`) through EMS
   rather than loading flat — understanding this early will likely
   clarify a lot of the `WORLD.DAT`/graphics loading code once traced.
   Also now has a discrepancy to resolve: the in-EXE string says "1MB"
   minimum, `manual.txt` says "2MB" minimum/"8MB" used if available —
   see overview.md.
5. **`WORLD.DAT` format.** 1.76MB, referenced by `loadWorldDat1`-`5` and
   `WorldDat_setBlock1`-`6` (names unverified, see #2). Likely holds
   maps, NPC data, conversation/text data, and item tables per the
   error strings (`"Problem retreiving text/NPC/conversation data."`).
   No structure investigated yet.
6. **`PICTURES.VGA` format.** 12.5MB — clearly a large sprite/tile/image
   atlas rather than a single image. Not investigated. Likely tied
   closely to #4 (EMS paging) given its size.
7. **130-segment structure.** No `CODE`/`DATA` renaming attempted (see
   overview.md's note on why this is lower priority than in
   `ultima1`/`ultima2` — 130 segments is a lot to rename individually).
   Revisit once enough functions are named to see if a smaller number of
   "real" logical regions emerges (e.g. most segments turning out to
   belong to a handful of purpose clusters).
8. **Command/dispatch structure for the first-person dungeon-crawling
   loop** (movement, 90° turn/rotation, wall rendering) — the core
   gameplay loop Paul is most interested in eventually, per the
   Eye-of-the-Beholder-style description. Not located yet; expect this
   to be reachable from `InitGraphics`/`InitGame`'s callees once traced.
   `manual.txt`'s control scheme (arrow keys move/turn, `Ctrl`+arrow
   strafe) is now a concrete spec to match against once found.
9. **Character-creation and class-advancement system.** Now has a
   precise spec from `manual.txt`, not just string evidence: each of the
   9 base classes is a percentage blend of two of three archetypes
   (Fighter/Thief, Cleric, Wizard) — e.g. `Alchemist = 75% Cleric, 25%
   Wizard`. No functions identified yet, but this gives an exact,
   checkable target the way `ultima1`'s point-buy mechanic did once
   found — see overview.md for the full class table.
10. **Shop/economy system** (buy/sell/enhance/repair, 3-currency economy
    of gold/ore/`NUORE`) — rich string evidence, no functions traced.
    `manual.txt` confirms `BARTERING` skill affects shop profit margins
    and `CHEMISTRY` converts ore via a "brown potion" — a concrete
    mechanic to look for once this cluster is found.

## Open questions

- Is `NUORE` a currency, a resource, or both? Appears alongside "MAGIC
  ORE" in several UI strings (e.g. `"GOLD COINS:"` / `"MAGIC ORE:"` /
  `"NUORE:"` together) — looks like a 3-resource economy, not just
  gold + one crafting material, but not confirmed against actual game
  logic yet.
- Relationship between the "region" passwords (`YENDORIAN`,
  `BARIAGIAN`, `OBVERSIAN`, `MONTESERIAN`, `SLATORIAN`, `HEARDONIAN`)
  and the "town" passwords (`PORT HOPE`, `THIEF'S DEN`, etc.) — two
  separate systems, or one list split across two string clusters by the
  crude scan? Needs checking against the actual code once located.
- Whether this executable really has zero overlay/chain behavior, or
  whether that's just because the crude string scan wouldn't have
  caught a chained filename buried mid-binary without an obvious
  `.exe`/`.com` suffix nearby — worth a targeted string search for
  `.EXE`/`.COM` substrings specifically, not done this session.
- What `sg0977`/`sg0ffc`/`sg1486`/`sg195C`/`sg1ABC` (the 5 segments with
  IDA hex-address names instead of sequential `segNNN`) actually are —
  gap in the sequential numbering, or something IDA treated differently
  at analysis time. Not investigated.
- Whether `Hex Hacking Item Guide.txt`'s item table (3 pages of ~256,
  ~768 total) is complete/current for *this* executable specifically —
  it's undated against a specific game version, and the shareware
  Chapter 2 release may not expose every item the full retail game (or a
  later chapter) does. Treat gaps/mismatches against the IDB as
  possible version drift, not necessarily guide error, given how well it
  cross-confirms on the parts checked so far (see file-formats.md).
