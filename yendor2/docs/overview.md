# Yendorian Tales Book I, Chapter 2 (DOS) — Disassembly Overview

Working notes on the `yendor2.idb` / `yendor2.asm` reverse-engineering
effort. Goal: fully document the DOS executable well enough to write a
clean C++ reimplementation and, eventually, a ScummVM engine module —
same overall approach as the sibling [`ultima1`](../../ultima1) and
[`ultima2`](../../ultima2) projects, adapted for this game's shape.

This file is the entry point into `docs/`. See also:
- [roadmap.md](roadmap.md) — prioritized list of what's investigated vs.
  still open.
- [file-formats.md](file-formats.md) — on-disk data formats (`WORLD.DAT`,
  savegames, `PICTURES.VGA`), once documented. Not started yet.

**Status of this pass (2026-09-14)**: initial setup only, per Paul's
request — got the headless IDA pipeline working end-to-end against the
existing `yendor2.idb`, and did a first read of the executable's strings
and structure to gauge whether there's enough to work with. No renaming
or structural changes made yet. **Existing names in the IDB are from
earlier, unfinished sessions and should not be presumed accurate** —
verify before relying on any of them.

## The game and its files

Real title per the game's own on-screen text: **"Yendorian Tales Book I:
Chapter 2"**, with an in-universe subtitle **"DARK UNION"** used on the
online clue-book screen (`"DARK UNION : THE ON-LINE CLUE BOOK"`). MobyGames
lists it at
https://www.mobygames.com/game/11051/yendorian-tales-book-i-chapter-2/
(not fetchable by automated tools — returns HTTP 403 — so cross-check
manually if needed). Genre matches Paul's description: first-person,
tile-based movement, 90°-turn rotation, Eye of the Beholder-style dungeon
crawler.

`game/` (gitignored — copyrighted game data, kept locally only for
reference, per the parent repo's `.gitignore`):

| File | Size | Role |
|---|---|---|
| `SW.EXE` | 199,292 bytes | The main executable — the one `yendor2.idb` is built from. Launched via `CH2.BAT` (`sw %1 %2 %3 %4 %5 %6 /b`, wrapped with `sbfmdrv`/`sbfmdrv /u` for Sound Blaster FM driver load/unload). |
| `SBFMDRV.COM` | 7,211 bytes | Sound Blaster FM driver, loaded/unloaded around `SW.EXE` by `CH2.BAT`. Not part of `yendor2.idb`. |
| `WORLD.DAT` | 1,761,397 bytes | Bulk game-world data — maps, presumably NPC/item/text tables per the in-EXE error strings (`"Problem retreiving text/NPC/conversation data"`). Not decoded yet. |
| `PICTURES.VGA` | 12,550,618 bytes | Graphics data (referenced by the in-EXE error string `"Problem with PICTURE.VGA."`, singular — the on-disk file is plural `PICTURES.VGA`). Not decoded yet. |
| `CURGAME` | 77,509 bytes | Active savegame. Starts `"SMITHWARE PARTY\0WARE\0"` at offset 0 — **`SmithWare` is the developer**, consistent with `SW.EXE`'s name. Same size as `SAVGAME1`. |
| `SAVGAME1` | 77,509 bytes | A saved-game slot (`"SAVGAMEX"` appears in `SW.EXE`'s strings as the templated filename — X is presumably the slot digit). |
| `CH2.BAT` | — | Launch script. |
| `dosbox.conf`, `mapper-0.74-3.map` | — | Paul's own DOSBox 0.74-3 setup for running the game, not part of the original release. |

`SW.EXE` is a 16-bit real-mode MS-DOS `MZ` executable (header confirmed
via IDA and a raw hex dump — `MZ` signature, standard relocation table).
Entry point per the IDB is `0x10000:0x0` (`start`, already named from
earlier work).

**Single executable, not an overlay chain** — unlike `ultima1` (5
separate chaining `.EXE`s), this game appears to be one binary. No
`chainToExecutable`-style `INT 21h`/`4Bh` EXEC pattern or second `.EXE`
filename has been confirmed in the strings yet (nothing found resembling
another executable name), but this hasn't been specifically verified by
tracing code — worth confirming once analysis starts in earnest, since
`WORLD.DAT` is large enough that some kind of dynamic loading/overlay
scheme for its contents is likely even without multiple EXEs.

**Many small segments**: the IDB has **130 segments**, `seg000`
(`0x10000`) through `seg130` (ending `0x3ce80`), overwhelmingly small
(tens of bytes to a few KB) with one large outlier, `seg129`
(`0x2d86d`-`0x3cc80`, 62,483 bytes — almost certainly the main data
segment: globals, string literals, etc., given its size and position
near the end). A handful have IDA-generated hex names instead of the
`segNNN` pattern (`sg0977`, `sg0ffc`, `sg1486`, `sg195C`, `sg1ABC`) —
just segments IDA didn't sequentially number, no special meaning
confirmed yet. This segmentation shape (many small code segments) is
consistent with a Watcom or similarly-segmented C/C++ compile (each
translation unit or function group getting its own segment) rather than
`ultima1`/`ultima2`'s handful of large, purpose-built segments — **not
yet renamed to a `CODE`/`DATA` convention**, and with 130 of them,
probably not worth doing per-segment; likely worth a role summary
instead once more functions are named.

## Headless IDA pipeline

Set up 2026-09-14, mirroring `ultima1`/`ultima2`'s `ida_scripts/`
approach (IDA Pro 8.3, `idat.exe`, no GUI required — the GUI is
incompatible with this flow since it locks the `.idb`). This project has
exactly one `.idb` (`ultima2`'s situation, not `ultima1`'s five), so the
driver hardcodes the single IDB/ASM/IDC path the way `ultima2`'s does,
but keeps `ultima1`'s later `-NoExport` refinement for read-only report
scripts.

- **`ida_scripts/run_ida_script.ps1`** — entry point.
  ```powershell
  .\run_ida_script.ps1 identify.py -NoExport
  .\run_ida_script.ps1 apply_renames.py
  ```
  Refuses to run if `yendor2.idb` is already open elsewhere (e.g. the IDA
  GUI) rather than racing it. `-NoExport` skips the `.asm`/`.idc` export
  and `save_database` step, for read-only discovery/report scripts.
- **`ida_scripts/batch_run_and_export.py`** — the actual driver, invoked
  via `idat.exe -A -S"batch_run_and_export.py <target.py> [noexport]"`.
  Execs the target script's code (so it runs exactly as it would under
  Alt+F7 in the GUI), then exports `yendor2.asm`/`yendor2.idc` and saves.
  Every step — including the target script's own captured stdout and any
  exception traceback — goes to `ida_scripts/batch_run_and_export.log`,
  since `idat.exe`'s own console output in `-A` mode is not reliably
  flushed before exit.
- **`ida_scripts/identify.py`** — read-only report script (root
  filename, input path/hash, segments, function-naming progress, struct
  list, sample of already-named functions). Safe to re-run any time as a
  sanity check with `-NoExport`.

Confirmed working end-to-end 2026-09-14 (report mode against
`yendor2.idb`, `identify.py -NoExport` — see current state below). Full
export+save round-trip not yet exercised here (no changes made yet to
export), but uses the exact same `ida_loader.gen_file`/`save_database`
code already proven in `ultima1`/`ultima2`.

## Current state (2026-09-14, before any work this session)

Via `identify.py`:

- **Root file**: `SW.EXE`, MD5 `d464f6847b9ea4296e9ce1b251f92788`.
- **771 functions total, 45 named (5.8%), 726 still `sub_XXXXX`.**
  Substantially less prior work than any of the `ultima1` executables
  had going in — this is close to a blank slate.
- **4 structs already defined**: `FontChar`, `FileEntry`, `Struc1`,
  `StringXY`. `FileEntry` pairs with 6 already-named `FileEntry_*`
  functions (`Close`/`CreateFile`/`OpenFile`/`Read`/`Write`/`Seek`) — a
  file-handle wrapper layer, plausibly CRT-like but not yet confirmed
  against a known runtime the way `ultima1`/`ultima2`'s CRT clusters
  were. `Struc1` is an obvious placeholder name — whatever it represents
  hasn't been identified.
- **Named functions given so far** (from earlier, unverified sessions)
  include plausible engine-infrastructure names — `InitGame`,
  `InitGlobals`, `InitMemory`, `InitGraphics`, `InitMouse`,
  `MapUnmapPages`, `allocMem`, `calcOffset`, `loadWorldDat1`-`5`,
  `WorldDat_setBlock1`-`6`, `writeString`, `writeChar`, `ErrorCheck`,
  `ErrorExit`, `wait`, `findSavegame`, `clear_kbd_buffer`,
  `GameDialog_draw*` (8 dialog-button-icon drawers: `Animation`, `Dos`,
  `Return`, `Load`, `Music`, `NewGame`, `Save`, `SoundFx`), and one
  hedged guess `Fade?` (question mark left in by whoever named it —
  treat as unconfirmed). These read as reasonable given the string
  evidence below (see `loadWorldDat*`/`WorldDat_setBlock*` against the
  `WORLD.DAT`-related error strings, `InitMouse` against expected DOS
  mouse-driver `INT 33h` use in an EGA/VGA-era game), but **none have
  been independently re-verified this session** — first real pass should
  spot-check a few before trusting the pattern.

## String survey — is there enough to work with?

Short answer: **yes, comfortably** — this is the main finding of this
session. A quick raw scan of `SW.EXE` for printable ASCII runs (≥5 chars,
crude regex, not IDA's own string detector) found roughly 850 candidate
strings, and unlike a lot of that being compiler noise, a large fraction
is genuine, readable game text. Highlights, by category:

- **File/error strings** (near the front of the file, likely one
  contiguous error-message table): `"Problem with WORLD.DAT."`,
  `"Problem with CURGAME."`, `"Problem with a SAVED GAME file."`,
  `"Problem with PICTURE.VGA."`, `"Problem retreiving text/NPC/
  conversation data."`, `"Required Expanded Memory Manager (EMM Ver 4.0
  or later)..."`, `"An EMM mapping error has occurred."` — confirms EMS
  (expanded memory) dependency and gives a direct map from error
  condition to the file/subsystem involved, exactly the kind of anchor
  that made `ultima1`/`ultima2`'s CRT/file-I/O layers easy to name.
- **Environment/hardware probing**: `EMMXXXX0` (the standard EMS driver
  device-name signature, confirms `INT 67h` EMS detection),
  `BLASTER=`, `SOUND=` (Sound Blaster env-var probing, consistent with
  `CH2.BAT`'s `sbfmdrv` wrapping).
- **Character system**: 6 core attributes (`STRENGTH`, `DEXTERITY`,
  `STAMINA`, `INTELLIGENCE`, `WISDOM`, `CHARISMA`) plus a large skill
  list (`PROJECTILE ACCURACY/DAMAGE`, `BATTLE ACCURACY/DAMAGE`,
  `ABSORPTION`, `REPAIR`, `SURVIVAL`, `LINGUISTICS`, `THIEVRY` [sic,
  in-game typo], `CHEMISTRY`, `NAVIGATION`, `CASTING`, `MAPPING`,
  `BARTERING`); 9 base classes (`FIGHTER`, `MERCHANT`, `ROGUE`, `MONK`,
  `ALCHEMIST`, `PALADIN`, `MAGE`, `DRUID`, `MARKSMAN`) each with two
  class-change tiers reachable later (e.g. `MONK / CLERIC / PRIEST`,
  `MAGE / WIZARD / SORCERER` — 6 lines listing 3 classes each, an
  18-class advancement tree); afflictions (`DISEASED`, `POISONED`,
  `STONED`, `FROZEN`, `PARALYZED`, `CURSED`, `HEXED`, `JINXED`).
- **Transports**: `PEGASUS`, `GIANT EAGLE`, `FLYING RUG`, `MAGIC DRAGON`
  — each with day/night flight-window restrictions in nearby strings
  (`"CAN ONLY FLY BETWEEN 12AM AND 6PM"` etc.).
- **World geography**: town/dungeon names `PORT HOPE`, `THIEF'S DEN`,
  `ANCIENT RUIN`, `REGISTRATION`, `TORCHLIGHT`, `NUMAGIK`, `STONY PEAK`,
  each paired with a `"<NAME> PASSWORD?"` prompt, plus 6 differently-named
  regional passwords (`YENDORIAN`, `BARIAGIAN`, `OBVERSIAN`,
  `MONTESERIAN`, `SLATORIAN`, `HEARDONIAN`) — suggests a
  region/territory system distinct from the town list.
- **Intro narrative text**, verbatim opening cutscene lines referencing
  "THE LAND OF YENDOR", the governor of Port Hope, and a villain
  ("`ZAMORA HAS FALLEN!`") — useful for confirming this really is
  "Book I Chapter 2" (a direct sequel hook referencing a prior game's
  events, i.e. "Book I Chapter 1").
- **In-game help system**: a full `TAB`-key help screen listing F1-F6
  sub-screens (`MAPS`, `MONSTER STATISTICS`, `SPELLS`, `MAGIC USERS`,
  `INVENTORY ITEMS`, `WALK THROUGH`) — likely a rich, self-documenting
  in-game reference worth cross-checking against `WORLD.DAT`/
  `PICTURES.VGA` content once those formats are decoded.
- **Item/economy UI**: key-tier system (`BRASS`/`BRONZE`/`COPPER`/`IRON`/
  `STEEL`/`SILVER`/`GOLD` `KEY`), lock states (`LOCKED`, `LOCKED AND
  TRAPPED`, `MAGICALLY LOCKED`, `NOT LOCKED BUT TRAPPED`), a
  gold/ore/`NUORE` (a game-specific currency or resource) triple-economy,
  shop flows (buy/sell/enhance/repair, with distinct failure/success
  message sets for each).
- **Registration nag screen**: `"Thank You for playing Yendorian Tales
  Book I Chapter 2 / Please register your copy today."` — shareware
  registration reminder, confirming this release is shareware (matches
  `SW.EXE`'s name: **S**hare**w**are).

This density and readability of strings is at least as good a starting
position as `ultima1`/`ultima2` had, and the direct file/error-message
correlations (`WORLD.DAT`, `CURGAME`, `PICTURE(S).VGA`) give an immediate,
low-risk way to start confirming the already-present `loadWorldDat*`/
`WorldDat_setBlock*`/`FileEntry_*` names — likely the natural first real
target once deeper work begins, same pattern as `ultima1`'s CRT
file-I/O-layer pass.

## Reference documents added by Paul (2026-09-14, same session)

Two files landed in `docs/` partway through this session, both useful
enough to fold into the working notes rather than just sit as raw
attachments:

- **`manual.txt`** — the official game manual. Confirms/refines several
  things the string survey above only guessed at:
  - **EMS requirement, more precisely**: manual says **minimum 2MB**,
    **up to 8MB** used if available — the in-EXE error string found this
    session only said `"Minimum of 1MB Expanded RAM must be available"`,
    so either the error string is a looser/older check than the real
    requirement, or "1MB" refers to something more specific (e.g. a
    single allocation, not the whole budget). Worth resolving once
    `MapUnmapPages`/`InitMemory` are actually traced.
  - **Class system is a percentage blend of two base archetypes**, not
    flat classes — e.g. `Merchant = 50-75% Fighter, 25-50% Thief`,
    `Alchemist = 75% Cleric, 25% Wizard`, `Marksman = 50% Wizard, 50%
    Fighter`. Every one of the 9 base classes found in strings maps onto
    one of 3 rows (Fighter/Thief-leaning, Cleric-leaning,
    Wizard-leaning), each row having a "pure", "mostly", and "half-and-
    half" member. This is a precise, checkable spec for wherever
    character-stat generation lives in the code — strong parallel to
    `ultima1`'s point-buy mechanic being an easy, high-confidence early
    win.
  - **Attribute/skill definitions**, matching the string-survey list
    exactly but now with actual game-mechanical meaning attached (e.g.
    `DEXTERITY` = turn order in battle, `CHARISMA` = bonus training
    points up to 15, `MAPPING` = party average gates overhead-map detail,
    `NAVIGATION` = party average gates transport range). Useful as
    acceptance criteria once the stat-calculation functions are found.
  - **Controls**: arrow keys move/turn, `Ctrl`+arrow strafes, single-key
    hotkeys (`A` attack, `C` cast, `D` disk panel, `K` keyring, `M` party
    map, `P` toggle panels, `R` rest, `S` shoot, `T` hourglass, `1`-`4`
    select character, `F1`-`F4` character sheets, `F5` map toggle, `F8`
    clue book, `SPACE` context-use/confirm) plus a fairly rich
    click/double-click/right-click mouse vocabulary. Good target list for
    locating the main input-dispatch loop later — the four on-screen
    action icons (`SHOOT`/`CAST`/`REST`/`DISK`) plus the `D`-for-disk
    hotkey plausibly connect to the already-named `GameDialog_draw*`
    cluster (`Dos`/`Load`/`Save`/`Return`/... — worth checking once
    that cluster is traced).
  - **"Book I" backstory recap**: this exe is a direct narrative sequel
    to an earlier game/chapter, not a fresh story — the manual's
    "Recent History" section names the antagonist banished at the end of
    Book I as **Paltivar**, and the returned-orb bearer as **Zamora,
    "Dean of the Athaneum"** (a person, not just an exclamation) — which
    directly explains two strings found in this session's own survey:
    the intro cutscene's `"ONCE ENCHANTED, THIS ORB WILL VANQUISH ALL
    EVIL CREATURES..."` and the ominous closer `"ZAMORA HAS FALLEN!"`.
    Useful narrative/text-asset context for later `WORLD.DAT` text-table
    work, and confirms "Book I Chapter 1" is a distinct, separate game
    from this one (not just a marketing chapter split).
- **`Hex Hacking Item Guide.txt`** — community-written (2004, Josh
  Hines), documents the savegame's per-item 4-byte encoding and a
  near-complete 3-page item ID table by direct hex-editing observation.
  Folded into [file-formats.md](file-formats.md#item-slot-encoding-from-hex-hacking-item-guidetxt-not-yet-cross-checked-against-the-idb)
  in full detail — this is likely the single highest-leverage document
  added this session, since it turns the `CURGAME`/savegame format from
  "undecoded" into "has a strong, partially externally-cross-confirmed
  hypothesis, just needs verifying against the actual struct in the
  IDB." Cross-confirmed independently against this session's own string
  survey (door-key tier names, town/key names) — see file-formats.md for
  specifics.

## Next steps (not started this session)

See [roadmap.md](roadmap.md) for the fuller prioritized list. Immediate
candidates once deeper work begins:
1. Verify/correct the 45 existing names by direct reading, same
   skepticism applied throughout `ultima1` (e.g. its `writeString2_mb` →
   `printStartupMessage` correction) — don't inherit possibly-wrong
   names uncritically.
2. Confirm or rule out an overlay/chaining mechanism (single EXE vs.
   `ultima1`-style multi-EXE) by tracing for `INT 21h`/`4Bh` EXEC or a
   custom loader like `ultima1`'s `execProgram`.
3. Trace the EMS-mapping code (`MapUnmapPages`, already named) given how
   central expanded memory clearly is (mandatory per the error strings,
   `PICTURES.VGA` alone is 12MB — must be paged in via EMS, not loaded
   flat).
4. Use the file/error-string correlations to nail down the
   `WORLD.DAT`/`CURGAME`/savegame loading path precisely, likely
   unlocking `file-formats.md` content in the same pass (`ultima1`'s CRT
   file-I/O layer and `ultima2`'s dungeon/map struct work both started
   this way).
