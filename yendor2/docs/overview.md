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

### 2026-09-14 session update, continued: core string utilities

While tracing `sub_14B24`'s repeated copy/trim/concat call sequence
(originally assumed to be box-drawing purely from its caller context —
it isn't, see below), found and named a small family of unambiguous,
broadly-used C-runtime-equivalent string helpers
(`ida_scripts/name_string_utils.py`):

- `StrLen` (`0x28A5A`) — `es:di=bx`, scan for a null byte (max 255),
  `ax` = length. 6 callers.
- `StpCpy` (`0x23A64`) — copies `src=ax` into `dest=bx` including the
  terminator; returns `bx` = pointer to the copied terminator (stpcpy,
  not plain strcpy's "return start"). 8 callers, including the
  already-trusted `findSavegame`, a good sanity check.
- `StrCat` (`0x1700E`) — finds `dest=bx`'s existing terminator (scanning
  up to 1024 bytes), appends `src=ax`; also returns a pointer to the new
  terminator. 16 callers.
- `TrimTrailingSpaces` (`0x16EDE`) — `StrLen` then walks backward
  turning trailing spaces into nulls. 5 callers.

This is exactly the same shape of win as the DS-segreg fix: foundational
utilities used across dozens of call sites, so naming them pays off far
beyond the 4 functions themselves — every caller's disassembly is now
more readable. `sub_14B24` itself (which chains `StpCpy`/
`TrimTrailingSpaces`/`StrCat` repeatedly with a base pointer
`word_2E546` and fixed separator strings) is building a formatted
multi-part label, not drawing a box as first assumed from
`DrawMessageBox` calling it — not yet renamed, worth a follow-up pass to
pin down what the label actually says.

Found and named one more of the same family: `StrFillN` (`0x1D01E`) —
`memset`-and-null-terminate (`dest=bx`, `count=ah`, `fill=al`, returns
end pointer, same convention as `StpCpy`/`StrCat`). Found via a scratch
text buffer at `0xAFA8` that several list/label-drawing functions clear
with spaces (`StrFillN(bx=0xAFA8, ah=0x19, al=' ')`) before rebuilding a
label in it and displaying it with `writeString` — this buffer, and the
functions around it (`sub_1F197` redraws whatever's currently in it;
`sub_1F1B8` looks up a list item's `x`,`y` from a 10-byte-per-entry
table at `0x5CD0` indexed by `word_3291E`) are a promising lead for a
list/menu rendering subsystem, not yet named.

Named both: `GetListItemPosition` (`0x1F1B8`, the 10-byte-per-entry
table lookup) and `EraseLabelText` (`0x1F197`, blanks the scratch
buffer with `StrFillN` then `writeString`s it, which — since it's now
blank — erases whatever label was drawn there before).

Both are called from `sub_1EA6E`, a 1631-byte function called directly
from both `start` and `InitGame`, touching all four `FileEntry` ops and
`GameDialog_drawButtons` — almost certainly a major top-level screen
(main menu or HUD) but too large and multi-purpose to confidently name
as a single unit without much more work; flagged rather than guessed.

73 named of 769 functions as of this update.

### 2026-09-15 session update: keyboard polling + clue book (resolves a prior ambiguity)

Traced `sub_1305E`'s wait loop down to the core, non-blocking keyboard
poll (`ida_scripts/name_poll_keyboard.py`): `PollKeyboardInput`
(`0x1D038`) does `INT 21h/AH=6/DL=0xFF` twice in sequence — a regular
character (uppercased if a-z) or, failing that, an extended/function-key
scan code — storing the result in `byte_2E400` and using `errorCode` as
an *input-event-type* flag (0/1/2). **Important**: this is the same
`errorCode` global named earlier from its role in `ErrorCheck`/
`ErrorExit` — it's legitimately reused for an unrelated purpose here,
noted via comment rather than given a second name. `sub_1305E` itself,
which loops calling this until a key event arrives, is now
`WaitForKeypress`.

Following one specific case in `PollKeyboardInput` — scan code `0x42`
(`'B'`) triggers a call to `sub_10C40` — paid off nicely: `0x42` is the
standard BIOS scan code for **F8**, and `docs/manual.txt` line 220 says
"F8 On-line clue book". `sub_10C40` calls `ShowPagedEntryScreen` (named
two updates ago), so it's now `ShowClueBook`
(`ida_scripts/name_clue_book.py`) — and this resolves
`ShowPagedEntryScreen`'s earlier "book text vs. catalog" ambiguity in
favor of "clue book entries", since that's what triggers it.

76 named of 769 functions as of this update. `sub_10C40` was previously
flagged as "too large to name" (1311 bytes) — worth remembering that a
function's size doesn't block naming it once an external anchor (a
manual hotkey, in this case) pins down its role with confidence.

### 2026-09-15 session update: PICTURES.VGA fully decoded

At Paul's request ("try pictures.vga"), fully cracked `PICTURES.VGA`'s
format by tracing its FileEntry (found via `FileEntry_OpenFile` always
reading the filename from `bx+0xE`, right after the 14-byte struct —
`bx=0x9043` → `"WORLD.DAT"`, `bx=0x902C` → `"SAVGAMEX"`, `bx=0x9011` →
`"PICTURES.VGA"`) through to a picture directory table and validating
the guessed field layout by **extracting and rendering actual pixels**
(`ida_scripts/extract_pic.py`, a standalone stdlib-only PNG writer) —
entry 0 turned out to be the exact "SmithWare" splash-screen logo, an
unambiguous confirmation. Full writeup with the directory struct layout
and all four verified examples in
[file-formats.md](file-formats.md#pictures.vga). Named the whole
pipeline (`ida_scripts/name_picture_system.py`): `g_pictureDir`,
`LoadPictureIntoEms` (the LIM EMS 4.0 LRU page cache + file read), and
`DrawPicture` (the blit, called from `start` and 8+ other functions —
the core picture-drawing primitive used throughout the game).

Along the way, also confirmed the WORLD.DAT-offset resource-stub
cluster documented earlier (`document_resource_stubs.py`) really does
read from `WORLD.DAT` and not `PICTURES.VGA` as originally unclear —
its offsets top out at `0x1AA5DD`, matching `WORLD.DAT`'s exact size
(1,761,397 bytes), not `PICTURES.VGA`'s 12.5MB
(`ida_scripts/extract_resource_stubs.py`).

78 named of 769 functions as of this update.

### 2026-09-15 session update, continued: full picture catalog + palette I/O

At Paul's follow-up ("keep going"), enumerated `g_pictureDir`
exhaustively (`ida_scripts/enumerate_pictures.py`) and found it has
**exactly 10 entries** — a small fixed set of splash/UI graphics, not a
general asset catalog. Extracted and identified all 10 by shape
(grayscale rendering, `ida_scripts/extract_pic.py`): the SmithWare logo
and dialog panel from before, plus a combat scene, a wolf/monster
silhouette, two sky/gradient panels, a male character silhouette
(plausibly the character-creation body template), and three icons.
Full table with descriptions in
[file-formats.md](file-formats.md#pictures.vga).

Also traced the palette machinery while looking for real colors to
render these in: `SetPaletteRange`/`GetPalette` (`0x25A3B`/`0x25A5B`,
direct VGA DAC port I/O — ports `0x3C8`/`0x3C9`, bypassing the BIOS),
`FadePaletteStep` (`0x11953`, one animation-frame step of a palette
fade), and `ShowIntroPicture` (`0x1177C`, called directly from `start`
and `InitGame` — shows a picture, fades its palette, waits for a
keypress; likely the boot splash-screen display). Didn't manage to
trace a picture's actual target RGB values back to their source this
pass — the fade-target buffer is a runtime-populated scratch area, zero
at rest in the `.idb` — so extracted images stay grayscale for now; see
file-formats.md's "Palette" section for exactly where the trail goes
cold.

Sent Paul the extracted images directly (`SmithWare` logo, dialog panel,
cursor, plus the new combat/wolf/character ones) rather than just
describing them.

82 named of 769 functions as of this update.

### 2026-09-15 session update, continued: found the real palette

Picking the palette thread back up: `ShowIntroPicture`'s own code
(right before the fade-buffer copy loop) turned out to call
`LoadMasterPalette` (a rename of `sub_27CB0` — one of the WORLD.DAT
resource-block stubs that were deliberately left unnamed earlier for
being indistinguishable) to read exactly 768 bytes from `WORLD.DAT` at
offset `0x8270A`. Read those bytes directly with a script — a
well-formed 256-entry VGA DAC palette, already in 0-63 range, no
decoding needed. Re-extracted the whole `PICTURES.VGA` catalog in true
color (`extract_pic.py` now supports `--palette`) and it's a clean,
unambiguous confirmation: the dialog panel's button labels are fully
legible, the wolf and character images render with entirely natural
colors. (The splash-screen logo shows some rainbow banding at higher
palette indices — not fully explained, maybe a second palette region
for that specific image.) Full writeup in
[file-formats.md](file-formats.md#pictures.vga)'s "Palette" section.

Sent Paul the recolored set. 83 named of 769 functions as of this
update.

### 2026-09-15 session update, continued: RunGameDialog identified

Went back to `sub_1EA6E` (1631 bytes, flagged much earlier this session
as "almost certainly a major top-level screen" but too large to
confidently name) now that far more of its surroundings are understood.
It draws `DrawPicture` id 1 — the 210×105 `GameDialog_draw*` panel
decoded (and now, with the real palette, fully *read*) earlier this
session — plus the mouse cursor, calls `GameDialog_drawButtons`, then
loops on `PollKeyboardInput` dispatching **all 8 of the panel's own
button hotkeys** (A/D/F/L/M/N/R/S → Animation/Dos/SoundFx/Load/Music/
NewGame/Return/Save) to their own handler functions — an exhaustive,
unambiguous match against the decoded panel. Named it `RunGameDialog`
(`ida_scripts/name_game_dialog.py`); the 5 individual per-key handler
functions (`sub_1F163` etc.) are noted in its comment by which key they
handle but not yet renamed — their actual behavior isn't traced yet.

84 named of 769 functions as of this update.

### 2026-09-15 session update, continued: mouse cursor draw/erase (and a correction)

Traced `RunGameDialog`'s 'A' (Animation) handler one level deeper —
`sub_1F163` → `sub_237B0` → `sub_23A22` — into what turned out to be the
mouse cursor rendering pair, and finally resolved the session-opening
`Fade?` hedge in the process. First pass named `sub_23A22` "DrawMouseCursor"
(blits a 16×16 tile from a fixed source) and `sub_237B0`
"RefreshMouseCursor" (its dirty-flag gate) — but reading `Fade?`'s full
body right after (only a fragment had been read earlier this session)
showed the roles were backwards: `sub_23A22` copies **from** a saved-
background buffer (`0xE0E`) **to** the video buffer — that's *erasing*
the cursor by restoring what was under it, not drawing it. `Fade?` does
the real draw: saves the new position's background into `0xE0E` (for
next time), then blits the actual cursor sprite (`0x3FE6`, `0xFF` as a
transparent color key) onto the screen. Corrected
(`ida_scripts/fix_cursor_naming.py`, `name_mouse_cursor.py` kept as-is
with a note pointing to the fix, for the record): `RestoreCursorBackground`,
`RestoreCursorBackgroundIfDirty`, and — finally — `DrawMouseCursor` for
what was `Fade?`. It was never a screen fade; the pervasive call sites
throughout the binary are just every point the cursor moves.

86 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunGameDialog's remaining handlers

Traced the rest of `RunGameDialog`'s per-key handlers
(`ida_scripts/name_dialog_handlers.py`), all reading cleanly and
consistently: `ConfirmQuitToDos` and `ConfirmNewGame` (Dos/New Game —
both draw a confirmation panel state then call the shared
`ShowConfirmPrompt(msg_id)` yes/no prompt, proceeding only on
confirmation), `ToggleMusicSetting`/`ToggleSoundFxSetting` (Music/Sound
Fx — flip a `g_driverStateFlags` bit, start/stop the driver, redraw a
checkbox via the shared `DrawCheckboxIndicator`), and
`CycleAnimationSetting` (Animation — cycles a 3-value setting, not a
plain on/off). The Load and Save flows are large inline blocks within
`RunGameDialog` itself (list-browsing via `GetListItemPosition`, text
entry for save names) rather than separate callable functions, so
they're covered by `RunGameDialog`'s own comment instead of a new name.

This closes out `RunGameDialog` essentially completely — every hotkey
now has a named, understood handler.

93 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunTitleScreen

Followed `ConfirmNewGame`'s reset call to `sub_1D2A6`, called both from
`start` directly and from `ConfirmNewGame` — draws `g_pictureDir` entry
2 (the combat-scene picture) full-screen, then dispatches single-key
top-level commands (`C`/`A`/`E`/`R`/`I`, not individually traced) plus
direct music/sound-fx toggles bypassing `RunGameDialog`. The game's main
title screen. Named `RunTitleScreen`
(`ida_scripts/name_title_screen.py`) — moderate confidence on the
overall role, low confidence on what each individual letter does (a
good target for a future pass, especially `R`, plausibly "Register"
given this shareware build's nag string).

94 named of 769 functions as of this update.

### 2026-09-15 session update, continued: character creation wizard

Traced `RunTitleScreen`'s 5 menu options (keyboard `C`/`A`/`E`/`R`/`I` and
mouse-click numeric codes 1–5 turned out to funnel into the *same*
handler labels — selectable either way). `E` is the clearest: it sets a
flag on up to 4 party-member records then returns from the function
entirely, back to whoever called `RunTitleScreen` — i.e. it's what
actually leaves the title screen and proceeds into the game (plausibly
"Enter"). `I` calls a 3-step wizard (each step ESC-cancelable) matching
the `CHARACTER CREATION`/`PICK A CLASS`/`MALE`/`FEMALE`/`PICK A
PORTRAIT` string cluster found near `g_pictureDir` earlier this session
— named `RunCharacterCreation`. Its first step draws the male body
silhouette (`g_pictureDir` entry 6) and a sky background (entry 5) into
a freshly-allocated *offscreen* compositing buffer rather than the
screen directly — named `ComposeCharacterPortrait`
(`ida_scripts/name_char_creation.py`). The other two wizard steps and
the finalizer aren't traced yet. `C`/`A`/`R` still aren't confidently
identified (`R`, plausibly "About" or a registration-info screen, replays
`ShowIntroPicture`).

96 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunConversation, and a major lead

Went back to `rank_naming_candidates.py` for fresh targets and found the
NPC conversation system: `sub_2B656` draws the dialog panel then
dispatches to one of 4 sibling functions based on flag bits in a record
pointed to by `word_2E548` — a record that shares a status-flags field
(`+0x1C`) with the party-member records `RunTitleScreen` touches, so
this operates on a character/NPC. Matches the manual's "talk to an NPC"
feature and the "Problem retreiving conversation data" error string.
Named `RunConversation` (`ida_scripts/name_conversation.py`); the 4
topic-display siblings aren't individually distinguishable from static
analysis (same caution as the earlier resource-stub cluster), so left
unnamed.

`RunConversation`'s only caller, `sub_295A8` (400 bytes, called directly
from `start`), turned out to be a **major command dispatcher** — reads
`word_32974` (the same "current command" global the item-icon dispatcher
from much earlier this session also used, but here spanning a much
wider numeric range: single-digit codes up through the 0x2xx range seen
before) and branches across ~20 different handlers, including
`RunConversation` and the item-icon dispatcher `sub_2AE3C`. This is
very likely the core gameplay command dispatch (movement, menus,
dialogue, etc. all funneling through one command code) — a strong lead
for a future session, but mapping ~20 command codes individually is
more work than fits in one round; not named yet.

97 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandleGameCommand — the core dispatcher

Fully read `sub_295A8` (the lead flagged last round): ~20 branches on
`word_32974` (an already-decoded command code) covering movement,
`RunConversation`, the item-icon dispatcher, and more — then, for
codes that don't match anything specific, falls back to
**context-sensitive interaction**: inspects flag bits on the
currently-targeted object (`word_2E548`) to decide what fits
(conversable → `RunConversation`, container-like → `sub_2D65A`, etc.).
Matches the manual's "SPACE uses the space you are standing on" exactly.
Named `HandleGameCommand`.

Checked its callers to confirm: all 6 are either directly in `start`
(the main game loop — mouse-click handling via `sub_1D118`, keyboard via
`sub_217ED`, both funneling into `HandleGameCommand`) or in similarly
central screens. This closes the loop on the whole input pipeline traced
this session: `PollKeyboardInput`/mouse → command code → `HandleGameCommand`
→ specific handler (`RunConversation`, `RunGameDialog`, item icons, …).

98 named of 769 functions as of this update.

### 2026-09-15 session update, continued: map object lookup and mouse hit-testing

Traced the rest of `start`'s main-loop input pipeline that feeds
`HandleGameCommand`: `ProbeFacingTile` (was `sub_217ED`) offsets the
player's position by fixed deltas per a facing-direction flags word and
calls `FindObjectAtPosition` (was `sub_2186F`) — a bounds-checked,
column-indexed lookup into the current map's object table by (x, y)
coordinate, with a clean found/not-found sentinel. Separately,
`HitTestRegionTable` (was `sub_1D118`) is the generic mouse hit-test
used throughout the UI: scans a table of 10-byte clickable-region
entries (min/max x, min/max y, result) for one containing the cursor —
the same mechanism `GetListItemPosition`'s table (`0x5CD0`) and several
other screen-specific region tables use.

**Crossed 100 named functions this update: 101 of 769.**

Also named `InteractWithContainer` (was `sub_2D65A`, moderate
confidence — one of `HandleGameCommand`'s two fallback handlers for
"container-like" target flags; checks a needs-confirmation bit and
prompts before proceeding, consistent with a locked/trapped container,
but not traced past that point). The other fallback, `sub_2D60A`,
turned out to search the same `0xDFBB` table as `RunGameDialog`'s
still-unresolved `0x242`-`0x245` handler (`sub_294A3`) — ties two loose
ends together without resolving either; a good target for a future
session that wants to tackle both at once.

102 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the 0xDFBB discovery mechanic

Resolved both loose ends flagged last round together — `sub_294A3` (the
item-icon dispatcher `sub_2AE3C`'s 0x242-0x245 handler, whose original
"character panel" guess didn't hold up) and `sub_2D60A`
(`HandleGameCommand`'s other fallback) both turned out to read the same
table at `DS:0xDFBB`, now fully decoded: 22-byte entries keyed by object
type, each holding a pointer+bitmask pair (a per-object-type "known/
unlocked" flag) and a required command code.

It's a **discovery mechanic**: `UseAbilityOnTarget` (was `sub_294A3`)
tries the player's current command against whatever they're facing
(`ProbeFacingTile`) — if it's the right one, the capability gets
permanently unlocked for that object type and a success message shows;
otherwise a fail/hint message. `ExamineTarget` (was `sub_2D60A`) is the
non-destructive counterpart — just checks whether the capability is
already known and shows one message or another, without attempting to
unlock anything. Corrected `sub_2AE3C`'s comment to match (the earlier
"character panel" guess is now definitively ruled out rather than just
unconfirmed).

104 named of 769 functions as of this update.

Also named `PlayMusicTrack` (was `sub_28296`, called from
`ShowIntroPicture`, the item-icon dispatcher's `0x26D` handler, and
`sub_11A10`): gated on the music-active flag, reads a music track's data
from `WORLD.DAT` via the shared `FileEntry` and hands it to the driver.

105 named of 769 functions as of this update.

### 2026-09-15 session update, continued: a fourth FileEntry (CURGAME)

Chased the `sub_2B17F` transport-check lead partway: the table it reads
via `sub_1CDBC` at `word_36E4B` is the same 4-entry table `RunTitleScreen`
sums to gate a flag — genuine confirmation these two areas are related
— but both tables are all-zero at rest (fresh-save runtime state), so
without a populated save to inspect, couldn't pin down what the 4
entries actually represent. Left `sub_1CDBC`/`sub_2B17F` unnamed rather
than guess further.

Went back to `rank_naming_candidates.py` and found a **fourth fixed
`FileEntry`**, at `bx=0x8FFB` — filename `"CURGAME"` (the active
savegame, distinct from `SAVGAMEX`'s numbered slots). Named
`LoadCurgameRecord` (was `sub_1770C`): reads a record from it via EMS
paging, indexed in a way that plausibly matches a per-character record
(`0x1A`=26-word stride), splitting one field by 100 (currency or time,
not confirmed). Documented in
[file-formats.md](file-formats.md#curgame--savgame1-and-presumably-savgamen)'s
`CURGAME` section as a concrete lead for whoever locates the actual
savegame struct next.

106 named of 769 functions as of this update.

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
