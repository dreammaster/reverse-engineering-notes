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
approach (IDA Pro, `idat.exe`, no GUI required — the GUI is
incompatible with this flow since it locks the `.idb`). Originally set
up against IDA Pro 8.3; switched to 8.2 on 2026-09-14 after a machine
move left only 8.2 available, and the `.idb` was recreated from scratch
under 8.2 — see the `ida_scripts/backups/` note below about the
tentative-renaming work that recreation lost. This project has
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
- **`ida_scripts/backups/`** — holds `yendor2.idc.pre-8.2-rebuild.bak`,
  a snapshot of `yendor2.idc` taken 2026-09-14 right before the `.idb`
  was recreated under IDA 8.2. The recreated `.idb` has none of the
  prior tentative function/variable renaming, so this backup is the
  reference to reapply (or diff against) that work rather than
  redoing it from scratch.

Confirmed working end-to-end 2026-09-14, including a full export+save
round-trip (not just report mode) — see "2026-09-14 session update"
below for what that round-trip actually did.

**`-NoExport` is not a dry run.** It only skips re-exporting
`yendor2.asm`/`.idc` and the explicit `save_database()` call in
`batch_run_and_export.py` — `idat.exe` itself still commits database
writes (`set_name`, `add_func`, `op_offset`, etc.) to `yendor2.idb` on
exit regardless. Confirmed by writing a comment in one `-NoExport` run
and reading it back, still present, in a separate later `-NoExport` run.
Use `-NoExport` for genuinely read-only report scripts (`identify.py`);
don't rely on it to "safely try" a mutating script.

### 2026-09-14 session update: reapplied prior work + ErrorTable recovery

The `.idb` referenced above as "before any work this session" was a
*second* rebuild: partway through this session the original IDA-8.3 .idb
had to be recreated again from scratch under IDA 8.2 (8.3 wasn't
installed on this machine), which lost the 45 named functions, 497
comments, 4 structs and 18 typed prototypes described below. That work
was fully recovered:

- `ida_scripts/backups/yendor2.idc.pre-8.2-rebuild.bak` — full IDC
  export from the pre-rebuild database, kept as the durable
  reference/backup.
- `ida_scripts/backups/yendor2_annotations.pre-8.2-rebuild.json` — just
  the `set_name`/`set_cmt`/`SetType` calls pulled out of the `.bak` by
  regex. This is what actually got replayed: a naive `idat.exe -c` fresh
  reload + `compile_idc_file()`+`main()` replay of the whole `.bak` was
  tried first and turned out to silently drop almost all of the naming
  (the old `Functions_0()` IDC function is ~750 statements back to back;
  the classic IDC VM chokes partway through it with no reported error —
  a scratch test reproduced only 45 of 748 names that way). Replaying
  the individual calls one at a time via `ida_scripts/apply_prior_annotations.py`
  instead gave full visibility into failures and got all 748 names, 497
  comments, and 18 prototypes applied (a handful of names needed a
  `SN_DELTAIL` retry where the fresh 8.2 auto-analysis had drawn item
  boundaries differently than the old analysis).
- Of the 748 reapplied names, only 45 were actually function names (the
  rest are data/string labels) — consistent with the "45 named
  functions" baseline, not a regression.
- `ida_scripts/fix_error_table.py` — separately recovered a piece of
  *structure*, not just names, that the reapplied annotations didn't
  cover: `ErrorTable` at `0x28A31` (20 words, referenced from
  `ErrorCheck+0x16`) dispatches to 19 tiny handler routines that the
  fresh auto-analysis never turned into functions at all (they're only
  reachable through the table, never `call`ed directly, so recursive
  descent doesn't find them). Rebuilt from scratch against the current
  database — function boundaries, table/operand "offset" typing so they
  render symbolically instead of raw hex, and names for the 15 handlers
  whose referenced message string is distinctive (`ShowErr_*`, e.g.
  `ShowErr_ProblemWithPalette`) — cross-checked against the committed
  pre-session `yendor2.asm` for ground truth. Net effect: 750 → 769
  functions, 44 → 59 named.
- `ida_scripts/try_flirt_sigs.py` — tried the DOS-era 16-bit C runtime
  FLIRT signature files bundled with IDA 8.2 (Borland, Digital Mars,
  Zortech, etc.) against the binary; none matched anything. IDA doesn't
  ship Watcom signatures by default, and the doc note above about a
  possible Watcom/similarly-segmented compile is still unconfirmed either
  way — this was a cheap thing to rule out, not a real result.
- `ida_scripts/analyze_functions.py` — characterized the remaining ~700
  unnamed functions to see if there was another mechanical win like
  `ErrorTable` available: no trivial (≤3-instruction) thunk chains, and
  only 2 small interrupt-dominant functions. Most unnamed functions are
  substantive (498 of 706 have 16+ instructions) — further naming needs
  genuine per-function reading, not another automated pass.

### 2026-09-14 session update, continued: DS-segreg fix (major unlock)

While manually reading `ErrorCheck`/`ErrorExit` (`ida_scripts/dump_range.py`
is the general-purpose tool for this — edit `START`/`END` and rerun), every
`ds:XXXXh`-style data operand rendered as a bare unresolved hex offset
instead of a symbol, and `idautils.DataRefsFrom`/`XrefsTo` came back empty
for them — this is also why the string-cross-reference approach earlier
in the session found 0 hits. Root cause: the DS segment register's
default value was never set for any segment in the fresh 8.2 database, so
IDA has no way to turn a `ds:offset` operand into a real linear address.
The old 8.3-era `.idc` had exactly this info in `SegRegs()`
(`SegDefReg(ea,"ds",0x2D86)` for all ~130 segments, uniformly) — this
piece of the old analysis wasn't part of the annotation reapplication
(only names/comments/types were replayed, deliberately, per the
whole-IDC-replay fragility above) but turned out to be small, mechanical,
and safe to redo directly.

`ida_scripts/fix_ds_segreg.py` sets `DS = 0x2D86` as the default segment
register value for every segment (matching the old analysis exactly) and
forces a full database reanalysis. Effect was immediate and large:
`ds:50D0h` in `ErrorCheck` turned out to already be named `errorCode` —
one of the 748 reapplied names, just invisible until this fix let it
resolve. More broadly, **391 of the 706 unnamed functions now show at
least one reference to a named data item** (was 0 before). This is
probably the single highest-leverage thing done this session for
unlocking further naming work, more so than any individual function
identification — worth running early in any future session that picks
this back up, if a database rebuild ever loses it again.

With DS resolution working, fully read and named the `ErrorCheck`/
`ErrorExit` cleanup chain (`ida_scripts/name_error_cleanup_chain.py`):
`RestoreInt1cVector` (restores the original INT 1Ch timer vector if one
was hooked), `FreeVideoBuffer` (frees `_videoBufferSeg`),
`ShutdownAudioDrivers` (sends shutdown commands through up to two
driver dispatch tables, frees their memory), plus globals
`g_driverStateFlags` and `g_soundDriverFarPtr`.

Also investigated and *deliberately declined* to mass-name a ~27-function
cluster at `0x27B42`-`0x2801A` (`ida_scripts/document_resource_stubs.py`):
each is a tiny stub that hardcodes one `FileEntry`'s block offset/size for
one specific game resource, called individually from many scattered call
sites (not a dispatch table). Which resource each one represents isn't
recoverable from static analysis alone (opaque pointer-table constants,
no distinguishing strings) — the old analyst left these as `sub_XXXXX`
too despite having the `FileEntry*` parameter type worked out, and
inventing distinguishing names now would be guessing. Documented the
pattern via a comment on the first one instead.

Named one more clean, high-confidence function found this way:
`FillVideoBuffer` (`0x14B10` — `es=_videoBufferSeg`, `cx=0x7D00` words =
64000 bytes = one full 320×200 VGA frame, `rep stosw`). 63 named of 769.

`ida_scripts/rank_naming_candidates.py` is the general tool for finding
more candidates like this: ranks unnamed functions by how many *named*
data/code references they contain (now that DS resolution works, this
is a real signal). It surfaced a large (30+), not-yet-investigated
cluster of small functions all touching `_font_bgColor`/
`_font_bgTransparent`/`_font_fgColor`/`_textPos_x`/`_textPos_y`/
`writeString`/`writeChar` — clearly text/label-drawing variants for
different UI screens, a promising lead for whoever picks this up next.
One traced this session, `sub_150E5`, draws what looks like a two-line
message box (calls `sub_14B10`/`FillVideoBuffer`-adjacent box-drawing at
`sub_14B24`, prints two strings from `word_2E3F8`/`word_2E3FA`, both
already commented `"msg"`) but wasn't named — confirming the box-drawing
helper chain (`sub_12554`, `sub_23A64`, `sub_16EDE`, `sub_1700E`) needs
its own dedicated pass first. Also spot-checked the existing hedged
`Fade?` name by reading past its entry point: the code actually reached
there (blitting a 16×8 tile from `word_2E4AA` into the video buffer at a
text-cell position, guarded by an audio call) looks much more like a
cursor/icon blit than a screen fade — the original "?" hedge looks
justified, a full trace of all its branches would be needed before
renaming it either way, left as-is rather than guess.

### 2026-09-14 session update, continued: paginated-entry screen cluster

Followed a lead from `rank_naming_candidates.py` into a small,
self-contained UI cluster and named it with real confidence
(`ida_scripts/name_paged_screen.py`): `ShowPagedEntryScreen` (0x132F2,
top-level: shows one entry + scroll-arrow state + fade transition),
`UpdateScrollArrows` (0x138C0: shows/hides two arrow glyphs depending on
whether the current page index `word_3293A` is at the first/last of up
to 31 entries, then loads that entry via `FileEntry_Read` and draws its
icon+message), `DrawMessageBox` (0x150E5: draws a box then two lines of
text). Exact content type (book/sign text vs. a spell/item catalog)
isn't confirmed — names describe the confirmed structural behavior
(paginated single-entry display) rather than asserting which game
content it shows.

Also traced (but did not name, `document_item_icon_dispatch.py`) a
command dispatcher at `0x2AE3C` handling event codes `0x242`-`0x2C8`.
`0x246`-`0x249` (4 codes, each flashing an icon then a transition)
plausibly correspond to the manual's 4 single-key inventory item icons
(`D` disk, `K` keyring, `M` map, `T` hourglass, `docs/manual.txt`
line 210-216) but which code is which item isn't determinable
statically. `0x242`-`0x245` share one handler that turned out to be an
ESC-cancelable list-selection routine, not a simple "draw character
panel N" as the manual's F1-F4 hotkeys might suggest — didn't rename it
since that hypothesis didn't hold up.

New reusable tools: `ida_scripts/find_callers.py` (who calls a given
address) and `ida_scripts/inspect_func.py` (one function's size/refs/
callers/callees at a glance) — both hardcode their target(s) at the top,
same pattern as `dump_range.py`.

66 named of 769 functions as of this update.

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
