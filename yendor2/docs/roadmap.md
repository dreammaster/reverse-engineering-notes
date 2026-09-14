# Roadmap

Prioritized list of what's investigated vs. still open for `yendor2.idb`
(`SW.EXE`). See [overview.md](overview.md) for narrative detail behind
each item.

## Not started (all of it — this was a setup-only session, 2026-09-14)

Roughly in likely investigation order, not a hard sequence:

1. **Spot-check the 45 pre-existing function names** against their
   actual disassembly before trusting any of them further — they're
   from earlier, unfinished sessions and Paul has explicitly flagged
   they may not be accurate. Start with the highest-confidence-looking
   cluster (`FileEntry_*` + `loadWorldDat*`/`WorldDat_setBlock*`, cross-
   checked against the `WORLD.DAT`/`CURGAME` error strings) since that's
   the fastest way to build or lose confidence in the rest.
2. **Confirm single-executable vs. overlay/chain architecture.** No
   second executable filename or `INT 21h`/`4Bh` EXEC pattern spotted in
   a first string pass, but not specifically traced in code yet. Matters
   for how the eventual C++/ScummVM structure should be shaped —
   `ultima1` was 5 chained EXEs, `ultima2` was 1; this looks like
   `ultima2`'s shape but isn't confirmed.
3. **EMS/expanded-memory subsystem.** Already-named `MapUnmapPages`/
   `InitMemory` plus the mandatory-EMS error strings (`EMM Ver 4.0`,
   `1MB Expanded RAM`, `EMMXXXX0` device signature) suggest this game
   pages large assets (especially the 12MB `PICTURES.VGA`) through EMS
   rather than loading flat — understanding this early will likely
   clarify a lot of the `WORLD.DAT`/graphics loading code once traced.
4. **`WORLD.DAT` format.** 1.76MB, referenced by `loadWorldDat1`-`5` and
   `WorldDat_setBlock1`-`6` (names unverified, see #1). Likely holds
   maps, NPC data, conversation/text data, and item tables per the
   error strings (`"Problem retreiving text/NPC/conversation data."`).
   No structure investigated yet — first target for `file-formats.md`.
5. **`CURGAME`/`SAVGAMEn` savegame format.** 77,509 bytes, starts with a
   `"SMITHWARE PARTY\0WARE\0"` header. Likely the easiest format to crack
   first (fixed-size, single party, probably a flat struct dump) —
   `ultima1`'s `Savegame` struct work is a good template. `Struc1`
   (already in the IDB, placeholder name) may already be this, or may
   be something else entirely — needs checking.
6. **`PICTURES.VGA` format.** 12.5MB — clearly a large sprite/tile/image
   atlas rather than a single image. Not investigated. Likely tied
   closely to #3 (EMS paging) given its size.
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
9. **Character-creation and class-advancement system.** Strong string
   evidence already (6 attributes, skill list, 9 base classes each with
   a 2-tier advancement path — see overview.md) but no functions
   identified yet. Good candidate for an early "pure game logic, no CRT
   digging required" win, same role `GEN.EXE`'s point-buy mechanic
   played early in the `ultima1` effort.
10. **Shop/economy system** (buy/sell/enhance/repair, 3-currency economy
    of gold/ore/`NUORE`) — rich string evidence, no functions traced.

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
