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

### 2026-09-15 session update, continued: ShowPartyMembers resolves 'C'

Found `DrawListEntryLabel` (was `sub_24FFC`): loads a paged record
(`sub_12554`) and draws an icon+text-label entry from it — the same
"icon + two-field label" pattern `sub_14B24` (the `DrawMessageBox`
helper) uses, confirming `sub_12554`'s record layout has an icon id at
`+8` and text fields at `+0x13`/`+0x20`. Its one caller, `sub_245AE`
(989 bytes, too large to trace this round), is itself only called by
`sub_23BAE` — which turned out to be `RunTitleScreen`'s **'C' option**:
iterates the party-member linked list, running a pipeline of per-member
display steps. Named `ShowPartyMembers`, resolving 'C' as "view your
party's characters" and updating `RunTitleScreen`'s comment accordingly
— only `A` and `R` remain unidentified among its 5 options.

108 named of 769 functions as of this update.

Also named `ShowWorldMap` (was `sub_2BD1A`, moderate confidence —
`RunTitleScreen`'s 'A' option, its only caller): draws a full-screen
background then places up to 9 small markers at per-location positions
from a table, skipping any not flagged discovered — fits the docs'
string-survey finding of ~7 named towns. This resolves 4 of
`RunTitleScreen`'s 5 options; only `R` (plausibly "About" or a
registration-info screen) remains unidentified.

109 named of 769 functions as of this update.

### 2026-09-15 session update, continued: character stats and inventory screens

Checked `RunTitleScreen`'s last unresolved option, `R`: it doesn't set a
new picture id before calling `ShowIntroPicture`, so it just replays
whatever's already showing (the combat scene) with the standard
fade+wait treatment — not distinct enough content to name confidently.
Updated the comment to record this rather than leave the earlier
under-qualified "About"/"Register" guess standing.

Dug into two of `ShowPartyMembers`' pipeline steps instead:
`ShowCharacterStats` (was `sub_24BF2`) draws exactly 6 lines of text —
matching the manual's 6 core attributes (Strength/Dexterity/Stamina/
Intelligence/Wisdom/Charisma). `ShowCharacterInventory` (was
`sub_245AE`, 989 bytes) draws up to 8 `DrawListEntryLabel` entries
(icon+text), each individually skippable — matching the 8-item-slot-
per-character savegame layout from `file-formats.md`'s item-guide
cross-reference — then a selection loop sharing the same N(ext)/Q(uit)/
E(xit) keys `ShowPartyMembers`' outer loop uses.

111 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowPartyMembers fully mapped

Named the remaining four `ShowPartyMembers` pipeline steps, completing
the character review screen end to end: `ShowCharacterSkills` (3
category headers + 15 skill lines, matching the manual's skill list),
`ShowCharacterEquipment` (a 3x3 grid of equipment-slot icons, fitting
the manual's equip diagram), `EditCharacterName` (a 13-character text
field that writes into the party-member record's first field — pins
down that record's name field at offset 0), and `ShowCharacterSummary`
(moderate confidence — reuses `ShowCharacterStats`' message pointers,
reads as a condensed recap rather than fresh content).

`ShowPartyMembers`' full pipeline is now: `ShowCharacterSkills` →
`ShowCharacterEquipment` → `ShowCharacterStats` →
`ShowCharacterInventory` → `EditCharacterName` → `ShowCharacterSummary`,
iterated per party member with N(ext)/Q(uit)/E(xit) navigation
throughout.

115 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CURGAME autosave and TryInteractAtPosition

Followed the savegame-struct lead from `EditCharacterName`: traced
`start`'s autosave-on-movement behavior (writes a fixed-offset record
to `CURGAME` after certain interaction outcomes) and the clean-shutdown
zero+close sequence. Named `TryInteractAtPosition` (was `sub_216F0`):
validates an interaction at a map position via `FindObjectAtPosition`,
branching on the target's type flags into a weight/capacity check, a
`CURGAME` record load, or a specific failure code — the function whose
outcome gates the autosave. Documented the party-member record's
confirmed fields (`+0x0` name, `+0x10` gender/type, `+0x1C` status
flags) in [file-formats.md](file-formats.md#curgame--savgame1-and-presumably-savgamen)
as a concrete starting point for locating the rest of the struct.

116 named of 769 functions as of this update.

### 2026-09-15 session update, continued: first combat/spell functions

Picked a fresh anchor: the two `HandleGameCommand` branches covering
wide numeric ranges neither `RunGameDialog` nor the item-icon
dispatcher claimed. `RestCharacter` (was `sub_2A9AD`, codes
`0x36`-`0x46`) matches the manual's "R rest": a time-of-day gate, then
percentage-of-max regeneration on one of two stat pairs. `CastSpell`
(was `sub_2AA58`, codes `0x12`-`0x1D`) matches "C cast spell": a
target-validity gate, then per-code effects — two confirmed as minor/
major heal (25%/50% of missing HP, capped at max). Both read/write the
same party-member record fields, letting the struct map in
[file-formats.md](file-formats.md#curgame--savgame1-and-presumably-savgamen)
grow concretely: `+0x52`/`+0x92` confirmed HP current/max, `+0x54`/
`+0x94` plausibly MP, `+0xE` a time-of-day value. Several of
`CastSpell`'s other codes (`0x14`/`0x17`/`0x18`/`0x1D`) aren't traced
yet — likely more spells.

118 named of 769 functions as of this update.

Also named `TickStatusEffects` (was `sub_208CA`, moderate-high
confidence): manages 3 timed-effect duration counters, decrementing one
per call and clearing its active flag on expiry — plausibly the subset
of the manual's 8 afflictions (Diseased/Poisoned/Stoned/Frozen/
Paralyzed/Cursed/Hexed/Jinxed) that are timed rather than
permanent-until-cured.

119 named of 769 functions as of this update.

Immediately confirmed it with its exact mirror: `ApplyStatusEffect`
(was `sub_2095A`, codes `8`/`0xE`/`0xB`) sets the same flag bits
`TickStatusEffects` clears and increments the same duration counters it
decrements — as clean a confirmation pair as this session has found.

120 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the rest of CastSpell's effect table

Traced `CastSpell`'s remaining codes: `0x14` (full HP heal), `0x17`
(full MP restore), `0x1D` (MP heal, 50% of missing — the MP-side
counterpart to `0x13`), and `0x18` (dispel/cure — clears a *different*
3-bit group in `+0x1C` than the one `TickStatusEffects`/
`ApplyStatusEffect` manage). `0x1C` turned out to be handled specially,
before the main self-target dispatch: a separate target-picking loop
with its own confirmation prompt — more complex than the others,
plausibly an attack or ally-targeted spell; not fully traced. Rather
than name each internal branch as a separate function (they're all
inline within `CastSpell`, not callable on their own), consolidated the
full effect table into `CastSpell`'s comment and
[file-formats.md](file-formats.md#curgame--savgame1-and-presumably-savgamen)'s
field map, which now has both HP and MP confirmed.

No new named function that round — a documentation/comment
consolidation of what's already been found.

Named one more: `FlashStatusWarning` (was `sub_1A5CC`, moderate
confidence) — `HandleGameCommand`'s handler for when a status effect is
already active (the flag `ApplyStatusEffect` sets). Plays a sound and
briefly flashes a picture before restoring, reading as a periodic
"you're still affected" warning.

121 named of 769 functions as of this update.

### 2026-09-15 session update, continued: FormatNumber, and CastSpell's 0x1C

Chased `CastSpell`'s last unresolved code, `0x1C`. Found and named
`FormatNumber` (was `sub_17FB8`, high confidence): a general decimal
integer-to-string formatter with leading-zero suppression, used here to
display the spell's effect amount. `0x1C` itself reads as an
offensive/damage spell — target selection, a resource-availability
check, then an icon + `FormatNumber`'d amount + sound — distinct from
the self-heal codes, but not fully traced (the exact resource check and
damage source remain open). Updated `CastSpell`'s comment with the full
picture.

122 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the dungeon automap/fog-of-war system

Picked a fresh anchor away from `CastSpell`: followed the player-
position update in `sub_112AE` (called directly from `start` —
presumably the movement command handler, not yet named itself) into
`sub_21D30`, which turned out to be a clean, well-confirmed cluster —
the **automap reveal system**. Named all 5 functions
(`ida_scripts/name_map_reveal.py`): `GetMapCellPtr` computes a map
cell's address in a confirmed 8-bytes-per-cell, 78-cells-wide grid;
`MarkCellExplored` checks/sets a per-cell "explored" flag (visit-once
fog-of-war marking, skipping cells already seen); `ScanAdjacentCellsAlongX`/
`ScanAdjacentCellsAlongY` each mark 3 neighboring cells; and
`RevealCellsAroundPlayer` (called right after the player's position
updates) uses the facing-direction flags to reveal the cells to both
sides of the player — matches the manual's "M uses the party map"
exactly (cells become known as you walk near them). Full grid layout
documented in
[file-formats.md](file-formats.md#in-memory-dungeon-map-grid-source-file-not-yet-identified--plausibly-loaded-from-worlddat).

This is a genuinely fresh system (no overlap with anything mapped
earlier this session) and a good entry point for whoever continues —
`MarkCellExplored`'s own reveal action (`sub_21CC2`) is the natural next
function to trace, and would likely lead into the actual dungeon-view
rendering code.

127 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PersistExploredCell, HandleMovementInput

`MarkCellExplored`'s reveal action, `PersistExploredCell` (was
`sub_21CC2`), turned out to be about *persistence* rather than
rendering: it writes the explored-cell bit directly into `CURGAME`
(bit-packed, byte = x/8 + `word_3685F`, bit = x%8) — the automap survives
save/load because it's stored in the savegame itself, not kept in
memory only.

Stepping back up, named the function that started this whole thread:
`HandleMovementInput` (was `sub_112AE`, called directly from `start`) —
dispatches on the key `H`/`P`/`K`/`M`, the classic BIOS extended scan
codes for the cursor arrow keys, matching the manual's "move forward/
backward, turn left/right" exactly. Updates position and facing, then
triggers the automap reveal chain.

The actual first-person dungeon-view *rendering* still hasn't
surfaced — `HandleMovementInput`'s own picture/marker draws
(`sub_116CF`) turned out to be a small on-screen indicator, not the
main view. A good next lead for whoever continues.

129 named of 769 functions as of this update.

### 2026-09-15 session update, continued: found the "renderer" — it's a minimap

Resolved the flagged lead by following the `sub_209D2`/`sub_20C1E`/
`sub_21612`/`sub_21588` sequence that recurs after every state-changing
action in `start`'s main loop. `BuildMinimapTileData` (was `sub_21612`)
gathers a 7×9 grid of tile-picture-ids centered on the player from the
map cells (two new confirmed fields, `+0`/`+2`, index into small lookup
tables). `DrawMinimap` (was `sub_21588`) then draws that grid as 63
small `DrawPicture` calls (base tile + optional overlay each) at a
fixed on-screen position. **This is the dungeon "view"** — a small
tile-grid minimap widget, not a full-screen first-person 3D render;
nothing resembling that has turned up anywhere this session. Full
write-up, including the two new map-cell fields, in
[file-formats.md](file-formats.md#in-memory-dungeon-map-grid-source-file-not-yet-identified--plausibly-loaded-from-worlddat).

131 named of 769 functions as of this update.

### 2026-09-15 session update, continued: minimap tile tables were a dead end, but led to a legend screen

Tried to pin down what `word_32926` (a parameter `BuildMinimapTileData`
sets per-cell before drawing) actually controls. `DrawMinimap` keeps its
own picture index fixed at `g_pictureDir` entry 9 throughout its loop,
so the varying table value isn't a picture selector there; checked
whether it's a color/remap parameter by reading `sub_2A53C` and ruled
that out too — it never reads `word_32926` at all. Documented as
genuinely open in file-formats.md rather than leaving the disproven
theory standing (commit `518cf62`, no new names that round).

Followed the same two tables (0xE551/0xE175, confirmed 12/10-byte
strides) into a second, previously-unexplored consumer: a full
interactive screen (`sub_20070`, reached from an ordinary keyboard
command slot in `start`'s main dispatch, not a debug hook) that draws
two scrollable 17-icon horizontal strips — one per table — as a legend,
then uses `GetMapCellPtr` on a stored screen position to preview the
current cell's own floor+overlay icon pair at full size. Named the
clearly-understood pieces: `DrawCellIconPair` (floor+overlay icon for
one cell, also used elsewhere), `DrawWallTypeLegendRow`/
`DrawFloorTypeLegendRow` (the two strips), `IsPairedValueMatch` (a
small fuzzy-equality helper used by the strip-highlighting logic:
`ax==bx`, or `ax`'s even/odd pair partner `==bx`).

**Correction (same session, before this got pushed further): this
screen is a map editor, not a passive legend.** Initially concluded
"nothing in this cluster writes back to map data" and named the screen
`ShowTileLegend` on that basis — checked the highest-ref-count
neighboring functions next (via `rank_naming_candidates.py`) and found
that was wrong. Its `A` key floods the *entire* visible 40×24 cell area
with the currently-selected legend tile and calls `FileEntry_Write` per
cell (via a new `PaintCellAndPersist`, called in a loop by a new
`FillVisibleAreaWithSelectedTile`); `B`/`F` browse a per-level tile
palette read from `WORLD.DAT`; a `9` key shows "H"/"V" coordinate-axis
readouts. Renamed `sub_20070` → `RunMapEditorScreen` and corrected its
comment rather than leaving the wrong "legend, not an editor" claim in
place. The `WORLD.DAT` palette-lookup side (`sub_205C0`/`sub_27FE0`,
plus `sub_205FB`/`sub_20626`/`sub_20817` which call it) isn't traced
precisely enough to name yet — left open.

A sibling cluster on the same two tables (`sub_20C8E`/`sub_20CEC`/
`sub_20D2F`/`sub_20E12`/`sub_29FF6`, called from the same `sub_20C1E`
master-redraw dispatch as the minimap) draws two small "current cell
class" preview boxes and scans candidate lists to highlight the
matching legend icon — the overall shape is understood but the exact
per-field semantics (`word_328E6`..`word_328F2`, which are read in
several places but never written anywhere findable by static
address — they're almost certainly filled by an indirect/computed
pointer write, not a literal `mov word_328E6, ax`) aren't confirmed
enough to name. Left open rather than guessed at.

136 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the masked-blit prep helper

One more small, self-contained find while in this area: `sub_2A53C`
(`DrawPicture`'s only callee) turned out to be a factored-out copy of a
mask-expand loop that also appears inlined twice more — directly in
`DrawPicture` and in `sub_29B0F` (the tile-legend cluster's other blit
routine). All three are byte-identical: if `word_328C6` bit 0 is set,
walk a byte mask (`word_2E48E`/`word_2E490`, set from ~12 call sites
throughout the game) and expand each byte's low nibble into a
zero-extended word — a standard VGA masked-blit prep step for sprites
with an explicit transparency mask rather than a simple color key.
Named `ExpandBlitMaskNibbles`, `g_blitMaskPtr`, `g_blitMaskLen`.
`word_328C6` itself is left alone — it's a broad flags word reused by
many unrelated subsystems, confirmed by the wide variety of other bit
patterns tested against it elsewhere.

137 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the "legend" screen is a map editor

139 named of 769 functions as of this update. See the correction in
the "minimap tile tables ... led to a legend screen" section above —
`RunMapEditorScreen` (was `ShowTileLegend`), `FillVisibleAreaWithSelectedTile`,
and `PaintCellAndPersist` are the 3 new names from this correction.

### 2026-09-15 session update, continued: CastSpell's 0x1C is alchemy, and a BCD bignum library

Went back to close out the one piece of `CastSpell` flagged as "not
fully traced": the `0x1C` effect, previously guessed to be "an
offensive/damage spell". Dumped its message strings directly from the
data segment (they weren't recognized as string literals by IDA) and
got `YOUR SKILL IS NOT HIGH ENOUGH!`, `YOU MUST HAVE AT LEAST 10
UNITS.`, and two creation messages (`NUORE CREATED` / `MAGIC ORE
CREATED.`) gated on which of two confirm-prompt answers (`5`/`7`) the
player picks. **This is a materials-transmutation/alchemy ability, not
a damage spell**: it gates on a target's skill byte, checks a fixed
resource counter has at least 10 units, then converts 10 units of one
material into the other. Corrected `CastSpell`'s comment rather than
leaving the old "damage spell, not traced" guess in place.

That resource check and the following add/subtract led to a genuinely
new, general-purpose subsystem, not specific to this one ability:
**4-byte (8-digit) packed-BCD bignum arithmetic**, called from all over
the executable (combat/inventory code, `HandleMovementInput`, etc.) —
almost certainly the engine behind gold/currency and other large
counters. Confirmed by the actual opcodes (`DAA`/`DAS`, which only make
sense for packed BCD). Named the whole family: `ConvertWordToBCD4`
(16-bit binary → 4-byte packed BCD), `CompareBCD4`/`IsBCDCounterAtLeast`
(most-significant-digit-first comparison, `CF=1` if smaller — the
"at least N" check), `AddBCD4`/`AddToBCDCounter`, `SubBCD4`/
`SubtractFromBCDCounter`. The scratch buffer these share (`word_38808`/
`word_3880A`, aka `ds:0xAFA8`) is left unnamed — it's reused for
unrelated things elsewhere, same pattern as `errorCode`/`word_32974`.

146 named of 769 functions as of this update.

### 2026-09-15 session update, continued: clamped resource-deduction helpers

Followed the two alchemy BCD counters (0x94B7/0x94BB) to their other
~14 call sites and found a third sibling counter (0x94B3) plus a
shared "pay this cost, whatever form it takes" dispatcher (`sub_18257`,
called from an icon-bar loop over 4 fixed slots that isn't fully
understood yet — possibly per-tick upkeep for an active effect rather
than a one-time item cost, so left unnamed). Three of its callees are
clean and independently confirmed, so named on their own:
`SpendMaterialCounterClamped` (BCD counter spend that zeroes rather
than goes negative when insufficient, notifying a not-yet-traced
"depleted" hook), `DeductHPClamped`/`DeductMPClamped` (party-member
HP/MP deduction, same `+0x52`/`+0x54` fields `RestCharacter`/
`CastSpell` already confirmed, clamped at 0 — `DeductHPClamped` also
sets a status bit and calls two untraced functions when HP hits 0,
plausibly death/incapacitation handling).

149 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the trap/status-effect system, and the real party-record array

Followed `sub_18257`'s caller chain (left unnamed last round) up to
`HandleMovementInput` and found a complete, coherent map-trigger/trap
system: `ApplyMapTriggerEffect` (was `sub_19E56`) is called whenever a
move lands on a flagged object, and branches on its type flags —
`0x4000` teleports the party, `0x2000` is a separate untraced effect,
and `0x1000`/`0x800`/`0x400`/a `0x300` pair are trap/status effects
applied to every valid (non-dead/paralyzed) party member. Each
afflicted member gets an entry in a 4-slot icon-bar array
(`g_partyEffectIconSlots`, was a bare `0xC50` literal), which
`ApplyEffectAndDrawIconBar` (was `sub_180BA`) then walks to draw the
effect's icon and apply its cost via `ApplyEffectCost` (was
`sub_18257`, now confidently named — its flag bits select one of the
three resource-deduction helpers named last round). Effect definitions
themselves live in a new 12-byte-stride table, `g_trapEffectDefs`
(icon, cost-type flags, display-mode flags), reset and looked up per
trigger by `PrepareTrapEffectSlots` (was `sub_18068`).

The most consequential single fact from this trace: `ApplyMapTriggerEffect`
computes a party-member record's address as
`g_partyRecords + (slot-1)*0x1F4` (500 bytes/record) via a small
4-entry slot-assignment table (`g_partySlotAssignment`, was
`word_36E4B`) — i.e. **the party is a fixed 4-record array with a
confirmed base and stride**, not a dynamically-linked structure. The
`word_328D4` "linked list" documented earlier this session is almost
certainly just iterating pointers into this same fixed array, not
separate heap allocations — worth keeping in mind for any future work
on party-record layout.

153 named of 769 functions as of this update.

### 2026-09-15 session update, continued: another correction — "+0x10 next link" was wrong

While documenting `g_partyRecords`, went looking for the `+0x10` "next"
link this same doc had claimed `word_328D4` was traversed through
since early in the session, to reconcile it with the new fixed-array
finding — and couldn't find it anywhere. Traced it to a comment that
had been misattached to `ShowPartyMembers`'s first instruction: the
text (describing its 6-step per-member display pipeline) was correct
about the pipeline but wrong about the traversal mechanism it was
written next to. The real mechanism, confirmed by reading
`sub_250E5`: a linear scan of `g_partyRecords` (cx=9, so up to 9
slots — more than the 4 UI/effect slots from the trap-effect system)
for the first record with `+0xE`==0, named `SelectDefaultPartyRecord`.
Corrected the comment in place and fixed `file-formats.md`'s party
record section rather than leaving the wrong claim standing.

154 named of 769 functions as of this update.

### 2026-09-15 session update, continued: a saving-throw/resistance pipeline

Kept following `ApplyEffectAndDrawIconBar`'s default branch one step
earlier, to the two calls it makes right before `ApplyEffectCost` —
and found a complete saving-throw/resistance-check pipeline for
trap/status effects. `RollEffectMagnitude` (was `sub_183D5`) computes
an effect's amount (fixed, or a random roll scaled by a party stat).
`RollEffectResistance` (was `sub_18333`) sums a **newly-found 9-field
block on the party record** (`+0x20` through `+0x30`, one per matching
high bit of the effect's cost flags — equipment/bonus resistance
values, individual fields not yet matched to specific types) and calls
`FailsSavingThrow` (was `sub_2781C`: `chance = max(5, 5*(level -
threshold) + resistance bonus)`, rolled against `RandomInRange(100)`)
to decide whether the effect actually lands. `RandomInRange` (was
`sub_2746C`) turned out to be the game's general-purpose PRNG — a
DOS-time-seeded linear-congruential generator, widely called from
elsewhere too.

158 named of 769 functions as of this update.

### 2026-09-15 session update, continued: a party-average-stat tiering system, three open leads

Followed the death-handling callee shared by `DeductHPClamped` and the
`0x600`-bit branch of `ApplyEffectAndDrawIconBar` to `sub_1AB26`: it
averages 3 party-record fields (`+0x64`, `+0x66`, `+0x58`) across valid
members and feeds each into a separate tiered gameplay system. The
first is the most concrete lead: its tiers write bits into
`word_36C7F`, which `DrawMinimap`/`BuildMinimapTileData` read directly
— bit `0x1000` skips drawing the minimap entirely and shows a
"depleted" indicator instead, strongly suggestive of a light-source/
torch-fuel mechanic, though not confirmed. The other two feed a
4-tier overlay effect (plausibly weather) and a progressively-revealed
per-object detail display (plausibly a bestiary/identify mechanic) —
both found but not traced. Named the well-understood averaging/tiering
function itself (`UpdatePartyAverageStatTiers`) and documented all
three leads in `file-formats.md` without forcing field-identity
guesses.

159 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the shared "resource depleted" overlay

Traced `sub_2704C` — the hook called both when a material BCD counter
can't cover a cost (`SpendMaterialCounterClamped`) and when the
dungeon view itself is blanked (`DrawMinimap`'s `word_36C7F` bit
`0x1000`). It's a single, unified "you're out of something" overlay:
blits a fixed image over the minimap's screen area, checks **all 3**
material BCD counters (confirming `0x94B3`/`0x94B7`/`0x94BB` are
exactly consecutive, stride 4 — nice consolidation of an earlier
finding), and draws status icons for whichever are empty via the new
`DrawResourceStatusIcons`. Named `ShowResourceDepletedOverlay`.

161 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — it's not weather, it's a map-reveal ability

Went back to the "plausibly weather" lead flagged two rounds ago and
read past the part I'd stopped at. It's wrong: `sub_28CFF` is a
special class ability (one of 4 slots, gated on a newly-found
per-character "abilities learned" bitmask at `+0xB4` and per-slot
charge/level values at `+0xB6`–`+0xBC`) that reads `WORLD.DAT` and
`CURGAME` directly, walks the same bit-packed explored-cell bitmap
`PersistExploredCell` writes, and reveals a tier-sized box of the map
centered on the player — classic `Locate`/`Scout`/`Magic-Mapping`
mechanics, not a visual effect. Corrected the hedge in `file-formats.md`
rather than leaving the wrong guess in place. Named `RevealMapRegion`
and its per-row worker `RevealMapRegionRow`; the exact ability/spell
name and a few internal gates (`sub_28C94`/`sub_28CB1`/`sub_29259`,
possibly tied to the still-unconfirmed "transport-check" table) remain
open.

163 named of 769 functions as of this update.

### 2026-09-15 session update, continued: found the main game loop; another hedge corrected

Followed `sub_234D3`'s caller (`sub_232A8`, which draws it exactly 3
times at 3 fixed addresses/screen positions — not a dynamic list, so
the earlier "plausibly bestiary/identify" hedge doesn't fit either;
corrected to "3 fixed status widgets, content unclear") one level
further up, into `sub_162F6` — called directly from `start` right
after setup, looping on itself and driving movement input, redraws,
and resource checks every iteration. This is the main dungeon game
loop. Named `RunDungeonGameLoop`.

164 named of 769 functions as of this update.

### 2026-09-15 session update, continued: found the combat system (turn order, monster slots)

Followed `RunDungeonGameLoop`'s per-iteration call to `sub_16A39` and
found the combat turn-order system, which also finally nails down what
the "3 fixed records" at `0x51C0`/`0x525C`/`0x52F8` are — **monster
slots**, not generic status widgets (a further correction on top of
last round's "content unclear" hedge, this time with solid evidence).
`BuildCombatTurnOrder` (was `sub_16A39`) builds a combined party+
monster list (`g_combatTurnOrder`, was bare `0x539E`) from
`g_partySlotAssignment` and the new `g_monsterSlots` (3 × 0x9C-byte
records, was `0x51C0`), giving monsters a random living party target
and sorting the whole list by a speed/initiative field — classic
initiative ordering. `SelectActiveMonster` (was `sub_16D7F`) then picks
the first non-defeated monster from that order. `FindPartySlotForRecord`
(was `sub_16D4D`) tells party members from monsters by whether a
record matches a `g_partySlotAssignment` slot.

With monster slots identified, `DrawMonsterInfoPanels`/
`DrawMonsterInfoPanel` (was `sub_232A8`/`sub_234D3`) now make sense as
a monster info display gated by the party's average "identify" stat
(`word_36CA9`) from two rounds ago — restoring that original hypothesis
on firmer ground, just wrong before about *why* there were exactly 3
slots.

169 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the per-level monster spawn/wander pool

Followed `RunDungeonGameLoop`'s other monster callee (`sub_22D4C`,
also called directly from `start`) and found the level-wide monster
pool feeding last round's 3-slot combat system: `g_levelMonsters` (was
bare `0xF26`), 80 records at the same `0x9C`-byte stride and `[+0xC]`
flag conventions as `g_monsterSlots`. `ProcessLevelMonsters` (was
`sub_22D4C`) ticks each occupied slot via the new `TickMonsterTimer`
(was `sub_22CED`, a per-monster countdown/state-machine: movement/
attack-readiness timers, plus a slower reset timer that restores a
monster from its template data — plausibly a death/respawn cycle), and
on "ready" promotes it toward active combat via two further,
not-yet-traced calls. This is the per-level spawn/wander pool that
`g_monsterSlots` draws its up-to-3 active combatants from.

171 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — "ready" means reward+removal, not combat promotion

Traced the two functions `ProcessLevelMonsters` calls on
`TickMonsterTimer`'s "ready" signal, guessed last round to "plausibly
promote this monster into an active combat slot." Wrong: they're
`GrantMonsterRewards` (adds fixed BCD values into the monster's own
tally fields and a global running counter, `0x51B6`) and
`RemoveMonsterFromMap` (clears the monster's map presence and wipes
its record entirely). So a `g_levelMonsters` slot's "ready" countdown
ends its presence with a reward grant and cleanup, not a hand-off into
`g_monsterSlots` combat — corrected the guess in `file-formats.md`
rather than leaving it standing. The real spawn-into-combat trigger, if
distinct from this, is still unfound.

173 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the loot/XP award screen — and another self-correction

Traced `sub_23151`, fired when the global staging counter `0x51B6`
crosses a threshold: it's the "you have found treasure" screen. This
also revealed that last round's `GrantMonsterRewards` comment had the
`AddBCD4` direction backwards — corrected it: the function *stages* a
dead monster's own loot fields into 4 global counters, it doesn't add
fixed values into the monster. `ShowLootAndAwardExperience` (was
`sub_23151`) then drains those staging counters into the permanent
material counters (`0x94B3`/`0x94B7`/`0x94BB`) and into every party
member's own `+0x18` field — a new find, plausibly experience points.
Named `FormatAndDrawBCD4` (was `sub_19B80`) along the way, the BCD
counterpart to `FormatNumber`. A call to `sub_22445` inside this
function turned out, on inspection, to be a party-portrait icon
redraw rather than the "level up" handler its calling context
suggested — left unnamed rather than force a guess that didn't pan
out.

175 named of 769 functions as of this update.

### 2026-09-15 session update, continued: a global quest/world-state flag system

Traced `GrantMonsterRewards`' two remaining untraced callees
(`[+0x14]`/`[+0x16]` signed stat deltas) and found a small, clean
global boolean flag-bit family backed by a bitfield array,
`g_globalFlags` (was `0x94D1`): `GetGlobalFlagBitAndWord` (shared
"index → word+mask" split), `SetGlobalFlag`, `ClearGlobalFlag`, and
`TestGlobalFlag` (confirmed fundamental — called directly from `start`
at several points, not just from monster deaths). So a monster's death
can set or clear an arbitrary quest/world-state flag via its own
`[+0x14]`/`[+0x16]` fields (negative = clear, positive = set). A
related but distinct family (`sub_27A6E`/`sub_27A3E`, `sub_27AC1`)
manipulates *per-object* flag banks at different fixed offsets from a
caller-supplied record instead of this global array — not traced this
round, a good next lead (the differing offset constants suggest more
than one record type carries its own flag bank).

179 named of 769 functions as of this update.

### 2026-09-15 session update, continued: per-object flag banks (party-member event flags)

Followed up on last round's flagged lead (the per-object flag-bit
family distinct from `g_globalFlags`). Traced two of the three
accessors: `GetRecordFlagBitAndWord_10C`/`SetRecordFlag_10C` (were
`sub_27A6E`/`sub_27A3E`) operate on a flag bank at a record's `+0x10C`
— the one traced real caller passes the current party member
(`word_328D4`) inside an item-use dispatcher branch, suggesting these
are per-character one-time-event flags (quest steps, items read, NPCs
met — not confirmed). `GetRecordFlagBitAndWord_CA` (was `sub_27AC1`)
is the same mechanism at a different offset (`+0xCA`) on an
unconfirmed record type. Named conservatively on their offsets rather
than guessing record identities beyond what's evidenced.

182 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the top-level UseItem system

Followed up on the item-use dispatcher partly explored a few rounds
ago (`sub_1BBED`, which turned out to be one branch of a bigger
system) to its actual top level: `UseItem` (was `sub_17B92`, called
directly from `start`) first calls the new `LoadItemData` (was
`sub_1C890`) — which reveals a `WORLD.DAT` item-data catalog at a
fixed record (`0xBCE`), structurally similar to `g_pictureDir`'s role
for `PICTURES.VGA` — then dispatches on the loaded item's own type-
flags word to one of several type-specific effect handlers. None of
those handlers are named yet (one, `sub_1BBED`'s material-gated
branch, was already traced two rounds ago) — a good next target now
that the dispatcher and its data source are understood.

184 named of 769 functions as of this update.

### 2026-09-15 session update, continued: one of UseItem's type handlers

Traced `UseItemType_400` (was `sub_1BEA1`, one of `UseItem`'s several
type-specific handlers), plus two small shared helpers:
`SelectItemUseRecord` (was `sub_1B702` — reveals `LoadItemData`'s
buffer is a list of 58-byte use-records, not a single blob) and
`FinishItemUse` (was `sub_1B6DE`, common post-use cleanup called by
every branch of this handler and its `0x800`-selected sibling). Its
material-cost branch repeats the exact "spend a BCD counter, then mark
a personal one-time-event flag" pattern already found in that sibling
two rounds ago; another branch builds message text instead, with no
cost — plausibly a non-consuming "read/examine" path. Named on the
dispatch-selector bit rather than a guessed item category, since the
true game-design identity still isn't confirmed.

187 named of 769 functions as of this update.

### 2026-09-15 session update, continued: items can flip up to 6 global flags each

Traced `sub_1BB48` — called from `UseItem`'s own fallback path and
from inside several of its type handlers — a shared "apply this item's
bit-level effects" step. It extends last round's global-flag finding:
an item can set/clear up to **6** global quest/world-state flags (not
just the 1-2 seen from `GrantMonsterRewards`), via 6 signed flag-index
fields in its data record. It also directly manipulates two pairs of
16-bit flag words (`word_328F6`/`word_328F8` and `word_2E40C`/
`word_2E40E`) with save/restore and set/clear-mask semantics —
plausibly current player/party status or equipment-bonus flags, not
confirmed. Named `ApplyItemEffectFlags`.

188 named of 769 functions as of this update.

### 2026-09-15 session update, continued: item-target status classification

Followed `word_2E40C`'s other writers (besides `ApplyItemEffectFlags`)
to a small cluster building an "item target selection" status display:
`ClassifyPartyMemberCondition` scores the targeted party member on 3
conditions (a "dead"-looking status bit, a wider status mask, HP below
max) into a tier; `CheckPartyMemberItemFlag`/
`CheckPartyMemberItemFlagAndClearPanel` check whether that member has
already triggered the current item's personal one-time flag (via the
now-complete `TestRecordFlag_10C`, finishing the Get/Set/Test trio for
that flag bank). `ClearStatusPanelIfDirty` (was `sub_16E18`, called
extremely widely including directly from `start`) is the shared
"erase the panel before redrawing it" step gated on the general
redraw-dirty flag.

193 named of 769 functions as of this update.

### 2026-09-15 session update, continued: found a real item type — UseHealingItem

One of `UseItem`'s type handlers (`word_2E410` bit `0x8000`) finally
had concrete, unambiguous behavior instead of the usual offset-
suffixed ambiguity: pays a BCD material cost, then restores HP and
clears specific status-ailment bits on the current party member per a
separate cure-flags word — classic healing-potion/cure-spell mechanics.
Named `UseHealingItem` outright rather than hedging. It also reuses
the trap/status-effect icon-bar system from several rounds ago
(`PrepareTrapEffectSlots`/`ApplyEffectAndDrawIconBar`) to show a
"healed" icon, and a separate branch confirmed `ClassifyPartyMemberCondition`'s
tier bits really do drive the user-facing status message shown for a
target, not just internal bookkeeping.

194 named of 769 functions as of this update.

### 2026-09-15 session update, continued: UseTrainingItem — a level-up item, and a flagged reinterpretation

Another of `UseItem`'s type handlers (`word_2E410` bit `0x4000`) turned
out concrete: pays a BCD material cost, then increments the current
party member's `+0x16` (level/skill stat, capped at 90) and
recalculates max HP/MP from it — a level-up/training item. Named
`UseTrainingItem`. Along the way, noticed it reduces `+0xE` (documented
since early in the session as "a time-of-day-like value") the exact
same way `RestCharacter` does, but here to select a class/race-specific
stat-growth path — a class/race id would fit at least as well as a
clock value. Flagged as an open alternate reading in `file-formats.md`
rather than overriding the existing description outright, since
nothing yet distinguishes between the two.

195 named of 769 functions as of this update.

### 2026-09-15 session update, continued: UseAbilityScroll closes the loop on special abilities

The last of `UseItem`'s four type handlers turned out to directly
confirm the special-ability system found many rounds ago via
`RevealMapRegion`: pays a BCD material cost, then teaches the current
party member a new special ability by setting the matching bit in
their own `+0xB4` bitmask and zeroing the matching per-ability charge
field (`+0xB6`/`+0xB8`/`+0xBA`) — a scroll/tome item. Named
`UseAbilityScroll`. This cross-confirms both fields' roles cleanly:
the charge resets to 0 the instant the ability is learned. With this,
all four of `UseItem`'s top-level type branches now have concrete
names (`UseHealingItem`, `UseTrainingItem`, `UseAbilityScroll`, and
the earlier `UseItemType_400`/`sub_1BBED` pair, which remain hedged
since their evidence was less conclusive).

196 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — party field +0xE is a class id, not time-of-day

Followed up on the lead flagged last round. Read `UseTrainingItem`'s
MP-growth branch in full — it blends two class-specific stat tables in
different proportions depending on the same reduced `+0xE` value — then
rechecked `RestCharacter`'s two branches side by side: HP-regen never
touches `+0xE`, but MP-regen reduces it the identical way and skips
regen entirely below 4. Put together, a class id (ids 0-3 plausibly
non-casters with no MP pool) fits both sites far better than "time of
day", which had no reason to determine class-specific growth formulas
or gate consistently on a fixed per-character value. Corrected
`file-formats.md` and `RestCharacter`'s own stale comment rather than
leaving the earlier guess standing next to contradicting evidence.

### 2026-09-15 session update, continued: two small stat-math helpers

Named the two clean, widely-reused arithmetic helpers behind
`UseTrainingItem`'s class-growth formulas: `ScaleByPercentRounded`
(round-to-nearest percentage scale) and `AddToStatCapped` (adds a
delta to any party-member stat field, clamped at 9999 for HP/MP or 999
for everything else, refusing to grow an uninitialized field).

198 named of 769 functions as of this update.

### 2026-09-15 session update, continued: mapped the 6 core attributes

Traced `sub_252EF`, character creation's stat roller, and finally
mapped the party record's 6 core attributes — flagged "not yet mapped"
since the very first pass over the executable's strings. Each rolls
`RandomInRange(15)+45` into a base field, copied to a derived field
0x40 higher; the derived fields at `+0x82`/`+0x84` (already known from
`UseTrainingItem`'s MP-growth formula) and `+0x80` (scaled 25% to set
HP) are now confirmed to be 3 of the 6 attributes, plausibly
Intelligence/Wisdom and Stamina respectively, alongside a weight-
deriving Strength-like pair and two more without a matched name yet.
Named `RollCharacterAttributes`.

199 named of 769 functions as of this update.

### 2026-09-15 session update, continued: found the skill-value array

Traced `ShowCharacterSkills`' reset step and found the character's
skill values live in a 16-word array at `+0xCA`–`+0xE9`, matching its
15 skill-name lines (3+4+8 across 3 categories). Individual skill
identities within that array aren't mapped yet. This also sharpens an
earlier hedge: `GetRecordFlagBitAndWord_CA`'s per-object flag bank sits
at the same relative offset but holds a bitmask, not plain word values
— since `+0xCA` on the party record is now confirmed to hold skill
values, that flag bank must belong to a different record type. No new
function named this round — a documentation-only pass, refining
`ShowCharacterSkills`' own comment to match.

### 2026-09-15 session update, continued: found the 8 item slots

Followed `ShowCharacterInventory`'s item-selection path down into a
new function, `GetInventorySlotPtr`, and found the last remaining
party-record TODO: each character has up to 4 separate 8-slot
inventories (1 main + 3 "alternate bags," selected by 3 marker
fields), each slot 4 bytes, addressed via a small group-base-plus-
offset formula. Slot content encoding itself (item id vs. quantity)
isn't decoded yet. With this, all three of the original "not yet
mapped" party-record TODOs (attributes, skills, item slots) have at
least a structural answer.

200 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the melee attack-roll formula

Followed `word_32A1E`'s ("active monster") other usages into a mouse-
click attack handler inside `RunDungeonGameLoop`'s input code, and
found the actual combat attack-roll math: `ResolveAttack` (was
`sub_25A73`) — hit if `(accuracy - defense) >= random(0-54)`, damage =
`weaponPower * (accuracy - defense) / 100`, minimum 1, or a flat miss
otherwise. `UpdateMonsterWoundTier` (was `sub_286B2`) then classifies
the hit into an escalating visual wound-severity tier (light/moderate/
severe, by percentage of the target's apparent max HP) — notably
without subtracting from any HP counter directly, so actual monster
death/HP tracking is still an open question for a future round.

202 named of 769 functions as of this update.

### 2026-09-15 session update, continued: monster HP found; named HandleDungeonInput

Right after `UpdateMonsterWoundTier` (which only sets a visual wound-
severity flag), its caller does `[word_32A1E+0x10] -= word_2E49C` — the
actual HP subtraction. Confirms `g_monsterSlots`' `+0x10` field doubles
as current HP in active combat, the same field `TickMonsterTimer` uses
as a lifespan countdown in the level-wide spawn-pool context — another
instance of this codebase's polymorphic-field pattern. `+0x50` is
therefore plausibly max HP; what happens at 0 HP (death handling)
remains untraced. Named the containing function, one of
`RunDungeonGameLoop`'s 3 per-iteration input handlers,
`HandleDungeonInput`.

203 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ProcessCombatRound closes the combat loop

Traced `sub_16B63` (called directly from `RunDungeonGameLoop`) and
found the piece connecting last round's HP subtraction to actual
death handling: `ProcessCombatRound` checks every `g_monsterSlots`
entry's HP each iteration, and on death flags its `g_combatTurnOrder`
entry `0x4000` — **confirming the "plausibly defeated" guess from
several rounds ago** — clears `word_32A1E` if needed, and calls
`GrantMonsterRewards`. If nothing died that pass, it instead advances
the turn to the next living combatant. This closes the loop: attack
roll → HP subtraction → death detection → loot, all traced end to end
across this session.

204 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowItemUsagePreview

Named one more `UseItem` fallback branch: `ShowItemUsagePreview` (was
`sub_1CA64`) re-dispatches on the item's own type flags but only to
call one of the target-status classifiers (`ClassifyPartyMemberCondition`
and friends) with no cost or stat change applied — reads as a preview
shown before actually using an item, not a real application of its
effect.

205 named of 769 functions as of this update.

### 2026-09-15 session update, continued: found the level-up system

Followed up on `+0x1E` (known to gate a portrait-redraw call, from a
few rounds back) and `+0x18` (suspected XP, from `ShowLootAndAwardExperience`)
into `CheckForLevelUp` and `ShowLevelUpMessage` — a clean, well-
confirmed pair: XP (`+0x18`, packed BCD) is compared against a
65-level threshold table (`0x9277`) starting from the current level
(`+0x16`); a higher resulting level is staged into `+0x1E` rather than
applied immediately, and `ShowLevelUpMessage` displays both the
current and pending level, confirming `+0x1E`'s role.

207 named of 769 functions as of this update.

### 2026-09-15 session update, continued: containers are separately-saved items

Confirmed the "3 alternate bags" from last round's `GetInventorySlotPtr`
finding are literal container items with their own persisted contents:
opening one (`LoadContainerContents`) reads its saved inventory
straight from `CURGAME`; closing it (`SaveAndCloseContainer`) writes
any contents back and unloads it. Each bag's contents live
independently in the savegame, only swapped into the character's
inventory groups while open.

209 named of 769 functions as of this update.

### 2026-09-15 session update, continued: a small material-counter HUD element

Named `ShowMaterialCounterHud` (was `sub_175F4`): a small, gated HUD
element showing material counter `0x94B3`'s current value — the same
counter consumed by several already-traced item types. Its calling
context (which screen(s) actually show it) isn't nailed down yet.

210 named of 769 functions as of this update.

### 2026-09-15 session update, continued: UseAbilityCommand, and ShowMaterialCounterHud's context resolved

Traced `sub_178A6` (called directly from `start`) — the top-level "use
an ability on a target" command shell: picks/validates a target,
confirms, writes the result to `CURGAME`, and shows a message box.
Named `UseAbilityCommand`. This also resolves last round's open
question — it's the caller that reaches `ShowMaterialCounterHud`, via
`sub_1732B`, when the action's own flags call for it.

211 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowLockStatus, closing a very early lead

Traced `sub_17795` and dumped its message strings directly — they're
the exact 7-tier key hierarchy (`BRASS`/`BRONZE`/`COPPER`/`IRON`/
`STEEL`/`SILVER`/`GOLD KEY`) the very first string survey of this
session cross-confirmed against the Hex Hacking Item Guide's door-key
table, but whose actual usage in code was never traced until now.
Named `ShowLockStatus`: examines a targeted lock and reports its state
(`NOT LOCKED`/`LOCKED`/`MAGICALLY LOCKED`/`LOCKED AND TRAPPED`/
`REQUIRES SPECIAL KEY: <tier>`), gated by a party member's `+0x6C`
field against skill-looking thresholds — plausibly lockpicking or
perception, not yet cross-checked against the skill array.

212 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — the "weight/capacity check" is a lock-state loader

`TryInteractAtPosition`'s very first comment (written before this area
of code was understood) called `sub_1766F` "a weight/capacity check".
Traced its body: it reads a bit-packed "previously unlocked?" array
from `CURGAME`, a second `CURGAME` block into an EMS buffer, and
splits a value via `/100` — nothing to do with weight. Confirmed by
its call site: right afterward, `TryInteractAtPosition` tests the
exact globals (`byte_32DCD`, `word_32DCE` bits `0x20`/`0x40`)
`ShowLockStatus` reads to pick its message. Named `LoadLockState` and
corrected `TryInteractAtPosition`'s stale comment (the old wording is
left standing in this log's earlier dated entries, per this session's
practice of appending corrections rather than editing history).

213 named of 769 functions as of this update.

### 2026-09-15 session update, continued: UseKeyItem

Found another `UseItem` dispatch branch: `UseKeyItem` passes the
item's own type-flags field directly as the lock id to `LoadLockState`
— confirms key items encode which numbered door they open via their
own catalog "type" value, tying `UseItem` and the lock system together
directly.

214 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CheckKeyItem

Found a near-duplicate of `UseKeyItem` at a different `UseItem`
dispatch offset — same `LoadLockState` call, but missing the "action
in progress" bracketing and final panel clear, reading as a lighter
check/preview variant. Named `CheckKeyItem`.

215 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunConversation's 4 topic-display branches

Traced `RunConversation`'s 4 sub-functions (previously left
undistinguished, "plausibly different response categories, not
confirmed which") — they share an identical mechanism (paginated
conversation text: portrait icon + 2-column word-wrapped pages), all
reading the *same* text field but with different prep functions and
screen position/color. Named by dispatch bit
(`ShowConversationText_4000`/`_2000`/`_1000`/`_800`) rather than
guessing which topic category each represents, since nothing
distinguishes their data source. Refreshed `RunConversation`'s own
comment to match.

219 named of 769 functions as of this update.

### 2026-09-15 session update, continued: conversation topics confirmed distinct

Checked the 4 "prep" functions the conversation-text display branches
call, and they settle last round's open question: each reads from
`WORLD.DAT` via a different resource-stub helper (part of the
~27-function cluster documented much earlier this session), so the 4
`ShowConversationText_*` branches genuinely do read 4 distinct topic
texts — confirming the original "plausibly different response
categories" guess, even though which specific category each is
(Name/Job/Bye/Rumor-style) still isn't identified. Named the 4 loaders
`LoadConversationText_4000`/`_2000`/`_1000`/`_800` to match.

223 named of 769 functions as of this update.

### 2026-09-15 session update, continued: a skill-gated conversation system

Traced `sub_2B9D4`, called before every `RunConversation` topic
display: compares the party member's `+0x6E` field against tiered
thresholds (selected by the topic's own difficulty bits) to gate how
much an NPC reveals — a 4-tier "response quality" system, plausibly
driven by a charisma/persuasion-like stat. Named
`ClassifyConversationSkillTier`. `+0x6E` sits right next to `+0x6C`
(the lockpicking/perception-like field from `ShowLockStatus`), outside
the confirmed skill array — a small cluster of derived/social stats,
or something else, not confirmed either way.

224 named of 769 functions as of this update.

### 2026-09-15 session update, continued: found the item-repair minigame

Traced `sub_2C010` (called directly from `HandleGameCommand`) and
dumped its message strings — an unambiguous item-repair minigame with
3 outcomes: critical fail (item destroyed), soft fail (item survives),
or success, chosen by a percentile roll against thresholds gated on
the character's own `+0x6A` field (a third member of the `+0x6A`/
`+0x6C`/`+0x6E` skill-like cluster found this session, plausibly
repair/crafting). Named `RepairItemCommand`.

225 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SyncAllContainers extends the container-persistence system

Traced `RepairItemCommand`'s opening call into a small cluster that
extends the container-persistence finding from a few rounds ago:
`SyncAllContainers` commits every open bag's contents to `CURGAME`
across the whole party (via `SyncPartyMemberContainers` →
`SyncContainerContents`, a write-back-without-closing variant of
`SaveAndCloseContainer`) — called before the repair minigame's risky
roll, presumably to keep the savegame current.

228 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the transport-check lead, resolved

Revisited `sub_1CDBC`, flagged as an unconfirmable "transport-check"
lead very early in the session (its table was all-zero at rest, with
no populated save to inspect). With `GetInventorySlotPtr`'s inventory
layout now known, its callee `sub_1CE6B` resolved cleanly:
`FindItemInInventoryRange` is a generic "does this character carry an
item whose id falls in a given range, anywhere including open
containers" search over the confirmed 8-slot main inventory (plus one
extra "equipped" slot). `CheckTransportAvailability` (was `sub_1CDBC`)
builds on it: checks a fixed 6-entry table for a direct route match,
then falls back to asking whether any party member carries a
qualifying item — "can the party use this mode of transport," gated
by a scripted route or an actual boat/horse-type item, not by
unreadable save state. (This doesn't resolve the separate, still-open
`sub_28C94`/`sub_28CB1`/`sub_29259` speculation from the
`RevealMapRegion` correction a few rounds back — different functions,
tentatively linked only by name.)

230 named of 769 functions as of this update.

### 2026-09-15 session update, continued: FindItemInsideContainer

Named `FindItemInInventoryRange`'s remaining callee,
`FindItemInsideContainer`: reads a container's contents from `CURGAME`
and searches its 8 slots for a matching item, recursing into nested
containers — full bag-within-a-bag support for the inventory-range
search.

231 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyMultiStatEffect, and RepairItemCommand cross-confirmed

Traced the very first check `HandleGameCommand` makes:
`ApplyMultiStatEffect` walks up to 4 (offset, amount) pairs from a
per-target table (populated from the target's own catalog lookup) and
bumps the matching party-member fields — reads as an equip-bonus or
multi-effect consumable mechanic. While reading further into
`HandleGameCommand`'s fallback dispatch, also found a second, direct
confirmation of `RepairItemCommand`'s role: it fires when the current
target has a "needs repair" catalog flag, passing the specific broken
item's catalog data as the roll parameter.

232 named of 769 functions as of this update.

### 2026-09-15 session update, continued: UnlockDoorCommand ties the lock system together

Named `sub_29738` — `HandleGameCommand`'s unlock-door handler — which
ties together `ProbeFacingTile`, `LoadLockState`, and `ShowLockStatus`'s
exact strings: finds the lock ahead, loads its state, shows `NOT
LOCKED` directly via the same bit test `ShowLockStatus` uses if already
open, otherwise compares required-key flags against the player's held
key. Completes the lock/key system traced across several rounds this
session.

233 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ToggleMapViewMode

Named `HandleGameCommand`'s `word_32974==0x1F` handler: branches on a
view-mode bit to either do a normal small-view redraw or draw a
full-screen picture instead — a zoomed/expanded map view, though the
exact trigger for the mode bit itself wasn't traced.

234 named of 769 functions as of this update.

### 2026-09-15 session update, continued: found an in-game clock/calendar

Traced `HandleGameCommand`'s `word_32974==7` handler and dumped its
template strings directly — `12:12 AM` and `12/12/1212`, filled in
with the current time and date. A genuine, standalone finding: the
game tracks a real in-game calendar, not just a coarse day/night
state. Named `ShowGameClockCommand`. The clock-advancement logic
itself (who ticks these globals, and how fast) isn't traced yet.

235 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the full game-clock mechanism

Followed `ShowGameClockCommand`'s globals to their source — a genuinely
major, foundational find. `AdvanceGameClock` is the master per-minute
tick: advances a minutes-since-midnight counter, rolls a full 30-day-
month/12-month-year calendar on day overflow, fires dawn/dusk events
at exactly 6:00 AM/6:00 PM, and runs a separate 5-minute periodic
timer. `ComputeGameClockTime` converts the counter to a 12-hour
display. Resting advances the clock by a fixed 8 hours, matching the
classic convention. The dawn/dusk and 5-minute event handlers
themselves (`sub_1FFE4`, `sub_1FD24`, `sub_28FF9` for new-day) aren't
traced yet — good next leads now that the clock's skeleton is known.

237 named of 769 functions as of this update.

### 2026-09-15 session update, continued: a real day/night ambient lighting system

Traced `AdvanceGameClock`'s dawn/dusk handler and found a genuine
ambient-lighting system, not a flag flip: `AdvanceDayNightPaletteFade`
gradually fades the VGA palette's last 32 entries over 113 steps
(forward from dawn, backward from dusk) through a snapshot table,
guarded so it only initializes once per transition. Confirms this
DOS-era game has real, gradual day/night lighting.

238 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the 5-minute timer is an ailment sweep

Traced `AdvanceGameClock`'s remaining piece, the 5-minute periodic
timer: `TickWorldAilments`/`TickAilmentDuration` — not an item/torch
timer as first guessed, but a status-ailment duration sweep. Ailments
occupy a shared slot format (`[+0]`=code, `[+2]`=remaining duration)
across a 6-entry world table (the same one `CheckTransportAvailability`
reads) and every party member's main inventory slots, decrementing
global per-ailment counters and clearing `TickStatusEffects`' flags
once an ailment type has no instances left anywhere — and disables
itself once nothing needs ticking. With this, `AdvanceGameClock`'s
whole tick chain (calendar, dawn/dusk lighting, ailment sweep) is now
traced end to end.

240 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ResetDailyAbilityCharges closes the clock chain

Named the last piece of `AdvanceGameClock`'s tick chain: on the daily
rollover, `ResetDailyAbilityCharges` zeroes every party member's 4
special-ability charge fields — special abilities recharge once per
in-game day. Ties the clock system directly to the special-ability
system from many rounds ago. `AdvanceGameClock`'s full chain (calendar,
dawn/dusk lighting, ailment sweep, daily ability recharge) is now
traced end to end.

241 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the clock is a real timer interrupt

Traced `AdvanceGameClock`'s own trigger and found it's called from a
genuine `INT 1Ch` timer interrupt service routine (18.2 Hz hardware
tick, ends in `iret`) — confirming the clock/lighting/ailment systems
run on real wall-clock time, independent of player movement or turns.
The ISR multiplexes 5 periodic sub-tasks off one hardware tick, each
with its own gate bit and reload countdown; named the two
cleanly-bounded remaining ones, `TickRedrawTimer` (periodic dirty-
screen flag) and `AnimatePaletteCycle` (a palette-cycling animation
effect, torch/water-shimmer style, not fully decoded). Left the raw
ISR entry point itself undisturbed — it's embedded in bytes mixed
with a preceding data declaration, and forcing a rename risked
corrupting the disassembly boundary; documented its dispatch
structure in `file-formats.md` instead.

243 named of 769 functions as of this update.

### 2026-09-15 session update, continued: UpdateAmbientMusic — the clock drives day/night music too

Named the timer-ISR's last sub-task, `UpdateAmbientMusic`: switches
between day and night background music tracks based on the game
clock's current time, via the already-named `PlayMusicTrack`. All 5 of
the timer ISR's sub-tasks are now named. A satisfying capstone to the
whole clock-system investigation this round.

244 named of 769 functions as of this update.

### 2026-09-15 session update, continued: confirmed the music-override handoff

Checked `word_3297E` (the "forced track" override `UpdateAmbientMusic`
respects) at its other write sites: `RunTitleScreen` forces title
music on entry and clears the override right at its `E`
("Enter"/leave-title-screen) exit — handing control to the ambient
day/night system for the rest of gameplay — and briefly silences music
during character creation before restoring it. Documentation-only
round, no new function named.

### 2026-09-15 session update, continued: ShowLocalAreaMap

Named `HandleGameCommand`'s `word_32974==0x1E` handler (also called
from `RunMapEditorScreen`): a full-screen, 24-row map view centered on
the player, reading both `WORLD.DAT` and `CURGAME` per row — distinct
from the already-named overworld `ShowWorldMap`.

245 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawLocalMapRow/Cell

Named `ShowLocalAreaMap`'s per-row/per-cell drawing helpers:
`DrawLocalMapRow` respects the same fog-of-war bitmap as the automap
(blank tile if unexplored), and `DrawLocalMapCell` calls the
already-named `TryInteractAtPosition` to show special interactive
objects (doors, etc.) with distinct icons rather than plain terrain.

247 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — "transport check" was too specific, and a quest-completion sequence found

Traced a new caller of `CheckTransportAvailability` (item-icon-dispatch
handler `word_32974==0x2C8`) and found it checking 4 *specific* item
ids for being **absent** — not a transport gate at all. The function's
own mechanism (fixed table, else party-wide inventory search over an
id range) is genuinely generic, so "transport" was too specific a
guess from its first-seen use. Renamed `IsItemRangeAvailable` and
corrected its comment. The new caller itself, `CheckQuestItemsCompleted`,
reads as a quest-item-completion reward sequence: if 4 specific items
are all gone from the party's inventory, plays a success sound and
runs an animated re-check of those 4 plus a 5th item in reverse order —
exact narrative not identified, but a genuine new lead into the game's
main-quest structure.

248 named of 769 functions as of this update.

### 2026-09-15 session update, continued: named the central item-lookup primitive

Named `sub_12554`, by far the most pervasively-called function
encountered this session (dozens of callers spanning nearly every
subsystem explored): `LoadItemCatalogRecord` maps in an item's 58-byte
catalog data and, as a side effect, sets up both `word_2E54A` (the
multi-stat-effect table `ApplyMultiStatEffect` walks) and `word_2E548`
(the "current target" pointer read throughout the codebase) — looking
up an item's data is also how the game establishes "the current
target" context.

249 named of 769 functions as of this update.

### 2026-09-15 session update, continued: UseLocationBoundPotion — the quest-item thread deepens

Followed up on last round's quest-completion lead: item `0x258`
(immediately adjacent to the `0x254`-`0x257` completion range) is
`UseLocationBoundPotion` — a potion that only works at one specific map
cell, confirmed by its own strings ("THE POTION WORKED SUCCESSFULLY" /
"YOU CAN NOT USE THAT HERE!"), setting a quest-milestone global flag
when used there. The adjacency (`0x254`-`0x258` consecutive, plus
`0x2C8`) strongly suggests one themed quest-item set — a good next
thread for a future round.

250 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowVisionAtLocation — another piece of the quest cluster

Item `0x253`, immediately before the `0x254`-`0x258` quest-item run, is
`ShowVisionAtLocation`: a scrying effect that briefly jumps the view
(not the player) to a fixed map coordinate using the same redraw
sequence teleports use, then restores it — presumably revealing a
story-significant location. Deepens the themed item cluster found over
the last two rounds; the exact narrative still isn't identified.

251 named of 769 functions as of this update.

### 2026-09-15 session update, continued: StopMusicAndResetTimer

Named `sub_2849C` (called from `start` and before forced music-track
changes): stops the current music and re-arms `UpdateAmbientMusic`'s
timer/latch so the ambient system or a following forced track can take
over cleanly.

252 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the full relic-item cluster confirmed

Dumped message strings for the remaining 4 items in the cluster
(`0x246`-`0x249`, all gated by a "patience is a virtue" recharge
flag) — powerful, unambiguous relic effects: `CollectNuoreCache`/
`CollectMagicOreCache` (+5,000 of a material each), `PartyMassHealAndOverheal`
(sets the whole party's HP/MP to 2× max), and `InstantKillActiveMonster`
(zeroes a monster's HP directly, no roll). Together with the vision/
potion/completion cluster from the last two rounds, this reads as a
themed set of quest/relic items central to the main story — exact
narrative still unidentified, but the whole cluster's mechanics are
now fully mapped, a strong foundation for whoever picks up the
narrative thread next.

256 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CheckWorldDatCompatibility

Named `sub_21DE2` (called from two unexplored save/load-adjacent
functions): dumped its strings (" LEVEL X" / " MAP X") and confirmed
it's a version/compatibility check comparing the loaded `WORLD.DAT`/
save data against an expected level/map number, not a text-record
parse as first appeared.

257 named of 769 functions as of this update.

### 2026-09-15 session update, continued: BuildLoadValidationMessage

Named `sub_12ECD`, a ~16-way dispatcher on a validation-failure-type
selector: composes a detailed error message for whichever specific
save/load validation failure occurred (state 1 uses the newly-named
`CheckWorldDatCompatibility` for level/map mismatches; other states
not individually traced).

258 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawSaveSlotList closes out the save/load menu thread

Named `sub_12C96`, `BuildLoadValidationMessage`'s caller: a scrollable
save-slot list display, highlighting the selected entry and drawing
each slot's status text via `BuildLoadValidationMessage`, with scroll
indicators when the list extends beyond the visible window. Completes
this round's save/load-menu investigation: `DrawSaveSlotList` →
`BuildLoadValidationMessage` → `CheckWorldDatCompatibility`.

259 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowSaveSlotMenu

Named `sub_12E59`, `DrawSaveSlotList`'s caller: the save-slot menu's
init+draw step — initializes scroll/selection state from a per-
category slot count table on first call, then draws the message-box
frame and slot list every call. Full chain now traced: `ShowSaveSlotMenu`
→ `DrawSaveSlotList` → `BuildLoadValidationMessage` →
`CheckWorldDatCompatibility`.

260 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — that whole chain was the clue book, not a save-load menu

Checked every caller of the "save slot menu" chain named across the
last four rounds (`sub_12B84`, above `ShowSaveSlotMenu`) and found all
13 of them are inside `ShowClueBook` — the game's actual F8 on-line
clue book (confirmed by its own pre-existing comment quoting the
manual) — with no other caller anywhere. The "level/map validation"
framing was wrong: `CheckWorldDatCompatibility` → `BuildClueLocationSuffix`
actually tags individual clue-book entries with which level/map they
apply to (appending " LEVEL X"/" MAP X" when relevant), and the rest of
the chain renamed to match: `BuildLoadValidationMessage` →
`BuildClueEntryText`, `DrawSaveSlotList` → `DrawClueEntryList`,
`ShowSaveSlotMenu` → `ShowClueCategoryEntries`. `word_2E3F6` is a clue
*category* selector, not a validation-failure type. Corrected rather
than leaving the wrong "save menu" framing in the historical log above
— see this entry for the real identification.

260 named of 769 functions (renames only, no new count).

### 2026-09-15 session update, continued: RunClueEntryMenu completes the clue-book chain

Named `sub_12B84` itself: the interactive per-category clue-book entry
menu (mouse-click hit-testing against the entry list, Enter/Space to
view an unread entry's detail). Completes the corrected chain:
`RunClueEntryMenu` → `ShowClueCategoryEntries` → `DrawClueEntryList` →
`BuildClueEntryText` → `BuildClueLocationSuffix`.

261 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowClueBookRegistrationNag — a shareware limitation

Traced `RunClueEntryMenu`'s "unavailable entry" callee and dumped its
message: "REGISTER YOUR COPY OF THE CLUE BOOK TODAY!" — some clue-book
entries are locked behind shareware registration. Named
`ShowClueBookRegistrationNag`.

262 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PlayClueBookOpenAnimation

Named the clue book's one-time intro animation: a palette swap, a
picture, two timed sound cues, then a restore — shown once per session
when the clue book first opens (skipped if in combat or already
shown).

263 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawClueBookNavBar

Named `sub_14C37`, called twice from `RunClueEntryMenu`: draws the clue
book's top navigation bar — two conditional hotkey hints ("d) LIST" /
"c) MAP", confirmed via raw message bytes) plus a row of 7 category-tab
icons, each swapped to a highlighted variant (base id +1) when its bit
in `word_328CC` (0x8000 down to 0x200) is set. Only the drawing
mechanism is confirmed; which tab corresponds to which clue-book
category is not traced.

264 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TryConvertItemToMaterial

Named `sub_19264`, a Space-bar action in the main input loop
(`sub_1869D`, `word_328C6` bit `0x10`) while carrying an item: checks
whether the held item's type mask overlaps the standing location's
accepted-type mask; on mismatch shows "I HAVE NO NEED FOR THAT TYPE OF
ITEM." (dumped from msg `0x7FF7`, which turned out to share a table
with the alchemy ore-conversion prompts — "YOU MUST HAVE AT LEAST 10
UNITS" / "IT WILL COST _ GOLD" / "IS THAT PRICE AGREEABLE?"); on match,
consumes the item and adds its value (`AddBCD4`) into the global
material counter `0x94B3`, then shows the material HUD. This gives
`0x94B3` a source beyond `ApplyEffectCost`'s cost dispatch, though its
exact identity (and what kind of station this is) is still open.

265 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — 0x94B3 is g_partyGold, not a generic material counter

While tracing `TryEnhanceItemForGold` (previously unnamed `sub_18FDA`,
the sibling Space-bar action to last round's item-sell function), its
failure message turned out to be "YOU DON'T HAVE ENOUGH GOLD!" —
direct textual proof that the counter at `0x94B3`, used by
`CompareBCD4`/`SubBCD4` there, is the party's gold, not a generic
"material" as framed last round. Confirmed further: `ShowMaterialCounterHud`'s
HUD label for this same counter (msg `0x7FC4`) is a literal `"$"`, and
the label is immediately followed in the same message bank by
"SPACEBAR TO / SELL ITEM OR / ESC TO UNDO" — the exact prompt for last
round's `TryConvertItemToMaterial`, which is therefore a sell-item
action, not a material conversion. Renamed: `0x36D13` (data) →
`g_partyGold`; `TryConvertItemToMaterial` → `TrySellItemForGold`;
`sub_18FDA` → `TryEnhanceItemForGold` (eligibility check, gold cost
from a per-tier table at `0xCB2`, then advances the held item to the
next catalog entry — an item-enhancement upgrade). Left last round's
entry standing above rather than editing it; `docs/file-formats.md`'s
material-counter section is corrected in place.

266 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TryRepairItemForGold

Named `sub_19140`, the third Space-bar sibling in `sub_1869D`'s main
loop (`word_328C6` bit 4, alongside `TrySellItemForGold`'s bit `0x10`
and `TryEnhanceItemForGold`'s bit 8). Structurally identical to
`TryEnhanceItemForGold`: rejects with "I CAN NOT REPAIR THAT" if
ineligible, else spends `g_partyGold` against a cost table at `0x5082`
and restores the held item from `word_3194C` (fixing the same item,
unlike `TryEnhanceItemForGold`'s advance to the next catalog entry).
Distinct from the skill-based `RepairItemCommand` minigame, which can
critically fail and destroy the item.

267 named of 769 functions as of this update.

### 2026-09-15 session update, continued: IsItemEligibleForEnhance / IsItemEligibleForRepair

Named the two eligibility-check helpers gating `TryEnhanceItemForGold`
and `TryRepairItemForGold`. `IsItemEligibleForEnhance` (`sub_1B147`)
selects a held-item field based on the standing location's flags and
checks it against a range table at `0xBCE`. `IsItemEligibleForRepair`
(`sub_1B20C`) matches a pair of location/held-item flag bits directly.
Both have other, untraced callers beyond this cluster (e.g. `sub_1CCBC`),
so only their confirmed primary behavior is documented.

269 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowInsufficientGoldMessage and RunSellItemScreen

Named `sub_190AF` (the shared "YOU DON'T HAVE ENOUGH GOLD!" rejection
used by both `TryEnhanceItemForGold` and `TryRepairItemForGold`) and
`sub_1B245` (reached from `UseItem` when the used item's `[+0xE]`
flags have bit `0x4000` set): sets `word_328C6` bit `0x10` — the exact
bit gating `TrySellItemForGold` — then runs the main input loop
(`sub_1869D`) itself so the player can interactively sell items,
cleaning up and redrawing the minimap on exit. This is the entry point
for the whole sell-item screen/station.

271 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowClueBookHelpScreen identifies the clue book's categories

Named `sub_14BD5`, called from `ShowClueBook`: the TAB help/index
screen, titled "** PRESS TAB AT ANY TIME TO SEE THIS SCREEN **",
listing F1 Maps, F2 Monster Statistics, F3 Spells, F4 Magic Users
(spells by class), F5 Inventory Items, F6 Complete Walk Through, ESC
Return to Game. This very likely names (at least 6 of)
`DrawClueBookNavBar`'s 7 category tabs, though the bit-to-category
mapping isn't confirmed yet.

272 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowClueBookItemDetail

Named `sub_13678`: draws an entry's icon plus "BASE VALUE:" and
"WEIGHT:" fields after DrawMessageBox/DrawClueBookNavBar — the F5
"INVENTORY ITEMS" category's per-entry detail screen.

273 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunClueBookItemCategory

Named `sub_13090`, the F5 Inventory Items category's own interactive
loop (called once from `ShowClueBook`): draws entries via
`ShowClueBookItemDetail`, polls input, and hit-tests a region table so
the player can click a sub-icon to jump directly to an entry. A more
complex sibling, `sub_13119` (two other `ShowClueBook` call sites),
also calls `ShowClueBookItemDetail` but adds extra dispatches for
item-id ranges overlapping `CastSpell`/`RestCharacter`'s selector
ranges — left unnamed, not traced.

274 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RedrawPartyGoldDisplay

Named `sub_1CC2E`: blanks the display area then redraws `g_partyGold`
via `FormatAndDrawBCD4`, plus a conditional resource-depleted overlay.
Called from `sub_1BBED`, a large UseItem shop/vendor "buy" handler
that spends gold against a price table (`0x512A`) — left unnamed this
round since its multiple purchase-path branches (single item vs. a
quantity loop that adds the unit price to both `g_partyGold` and a
second counter `0xB30`) aren't disentangled with enough confidence
yet.

275 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowHealingCostPrompt

Named `sub_1B96F`, called from `UseHealingItem` (5 sites) and
`UseItemType_400`: computes and displays a temple/healer paid-service
cost. Message dump confirms the prompt text: "IT WILL COST `<total>`
GOLD TO REPLENISH YOUR HEALTH POINTS." / "...TO REMOVE YOUR
CONDITIONS." / "...TO RETURN YOU TO LIFE." / "...TO COMPLETELY RESTORE
YOU.", followed by "IS THAT PRICE AGREEABLE?". Only computes and draws
the total; the Y/N poll and gold deduction happen in the caller
(not traced).

276 named of 769 functions as of this update.

### 2026-09-15 session update, continued: precision fix — ShowHealingCostPrompt's confirm/pay step isn't in any traced caller

Checked all 6 call sites of `ShowHealingCostPrompt` (found a 6th: it's
also called from `UseTrainingItem`, not just `UseHealingItem`/
`UseItemType_400`). Every one `retf`s immediately after the call —
none poll Y/N or deduct gold there. Corrected last entry's overstated
claim that "the caller" handles the confirm+pay step; in fact no
traced call site does, so that step (if it exists at all) happens on
some separate, later re-entry not yet found. Renames only, no new
count.

### 2026-09-15 session update, continued: SelectAndDrawPartyStatusRow

Named `sub_19957`: given a party record, maps it to a slot number via
`g_partySlotAssignment`, fakes that digit as a keypress to reuse the
main loop's panel-select routine (`sub_25B34`), then draws that
member's status-bar row (portrait icon, name, level, packed-BCD XP).
Called from a `UseItemType_400` path and from the F1-F4/click-portrait
party-member selection handler (`sub_19553`).

277 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawAlchemyStatusPanel

Named `sub_1E546`, the alchemy screen's character/resource panel:
name, a "MAGIC:" current/max bar, and "MAGIC ORE: " (`0x94B7`) /
"NUORE: " (`0x94BB`) readouts. The "MAGIC:" label independently
confirms `+0x54`/`+0x94` as MP current/max (already established via
`CastSpell`'s heal codes) — two independent confirmations for the same
field pair now.

278 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunAlchemyScreen

Named `sub_1DCE0`, called directly from `start`: the alchemy screen's
own 534-line driver loop, built around repeated `DrawAlchemyStatusPanel`
redraws, input polling, clickable-region hit-testing, the shared
party-member panel-select routine, and a confirm prompt (plausibly for
an ore conversion) before exiting back to the dungeon via
`ApplyMapTriggerEffect`. Confidently the alchemy screen's driver given
its direct reach from `start` and total reliance on
`DrawAlchemyStatusPanel`, even though its many internal helpers aren't
individually traced.

279 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PayGoldAndAcquireItem

Named `sub_17A8D`, a 4th shop-mode action (`word_328C6` bit `0x200`,
alongside sell/enhance/repair's `0x10`/`8`/`4`) reached via mouse
click (region table `0x5AC0`) rather than Space: pays `g_partyGold`
against a price at `0xB30`, bailing if unaffordable and special-casing
an exact-drain-to-zero purchase with `ShowResourceDepletedOverlay`,
then stages the acquired item the same way the Space-bar actions stage
theirs. Its caller `sub_17032` (a catalog-click handler with several
other, untraced item-type branches) is left unnamed for now.

280 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunShopScreen ties the whole vendor cluster together

Named `sub_1732B`, reached from `UseAbilityCommand`: the umbrella shop
screen hosting all four vendor actions found this session. Calls the
main input loop `sub_1869D` directly, twice (enabling the
sell/enhance/repair Space-bar cluster), and `sub_17032` (enabling
`PayGoldAndAcquireItem`'s click-to-buy path); redraws
`ShowMaterialCounterHud` repeatedly; writes state via `FileEntry_Write`
near an exit. Many internal helpers not individually traced.

281 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowMapSkillTooLowMessage

Named `sub_222BD`, called from `ShowLocalAreaMap` and
`ToggleMapViewMode`: "YOUR SKILL IS NOT HIGH ENOUGH!" — a
cartography/mapping skill gate, not yet traced to a specific
party-record field.

282 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowCompassDirection

Named `sub_21530`, called from `RunAlchemyScreen`: a "NORTH"/"SOUTH"/
"EAST"/"WEST" HUD readout, drawn at the same position as the material/
gold HUD, gated on an unidentified "compass active" mode.

283 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunClueBookMonsterCategory and LoadClueBookMonsterEntry

Named the F2 "MONSTER STATISTICS" category's loop and entry loader,
mirroring the F5 Items cluster. `LoadClueBookMonsterEntry` reads
`WORLD.DAT` block `0x32` for the current entry into a fresh buffer;
`RunClueBookMonsterCategory` (from `ShowClueBook`) calls it once then
loops redrawing a detail panel (`sub_141D9`, 362 lines with dozens of
labeled stat fields, left unnamed) until ESC.

285 named of 769 functions as of this update.

### 2026-09-15 session update, continued: mapped ShowClueBook's full F-key dispatch; two precision corrections

Traced `ShowClueBook`'s own `word_2E40A` switch directly, mapping all
6 F-key categories plus ESC: F1 Maps, F2 Monster Statistics (→
`RunClueBookMonsterCategory`), F3 Spells, F4 Magic Users (class picker
then that class's spell list), F5 Inventory Items (→ an 8-subtype
selector, subtype 1 = `RunClueBookItemCategory`, subtypes 3–6 =
`sub_13119`, others unnamed loops `sub_13278`/`sub_13216`/`sub_1334E`/
`sub_1318D`), F6 → the already-named `ShowPagedEntryScreen`. Two
corrections along the way: `RunClueBookItemCategory` is only item
subtype 1's loop within F5, not "the F5 category's own loop" as
described when it was named; and `sub_13119` has 4 confirmed call
sites, not "two other sites" as first counted. Documentation-only,
renames only, no new count.

### 2026-09-15 session update, continued: ShowClueBookMonsterDetail

Named `sub_141D9`, the F2 Monster Statistics detail panel. Message
dump gives a full monster stat sheet: `EXPERIENCE:`, `GOLD:`,
`MAGIC ORE:`, `NUORE:` (loot, matching `GrantMonsterRewards`'s 4
staged loot fields), `HEALTH-`/`ACCURACY-`/`DEXTERITY-`/`ABSORPTION-`/
`DAMAGE-`/`RANGED ACC.-`/`RANGED DAM.-` (combat), and 10
resistance/vulnerability labels (poison, disease, paralysis, freezing,
hexing, cursing, fire, cold, electric, power). Individual field
offsets not traced yet.

286 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunClueBookSpellCategory and ShowClueBookSpellDetail

Named the F3 Spells / F4 Magic Users clue-book category loop and its
detail panel. `ShowClueBookSpellDetail` draws "CLASS:"/"LEVEL:" plus
"MP:"/"NUORE:"/"ORE:" cost fields (spells cost MP and the two alchemy
ore counters) and "AFFECTS:"/"WHEN:"/"EFFECT:" description sections
with a 6-class eligibility marker row.

288 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunClueBookMapCategory, LoadClueBookMapEntry, DrawClueBookMapGrid

Named the F1 "MAPS" clue-book category cluster. `LoadClueBookMapEntry`
reads the current map id from `WORLD.DAT`; `DrawClueBookMapGrid`
computes a row/col grid from the id and draws per-cell location labels
via the already-named `BuildClueLocationSuffix`; `RunClueBookMapCategory`
ties them together with input polling and cell-click hit-testing
(dispatching to an untraced `sub_14122`), until ESC.

291 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunClueBookTransportCategory / ShowClueBookTransportDetail

Named F5 item-subtype-7's cluster: a single "TRANSPORTATIONS" screen
(dumped title) listing PEGASUS, GIANT EAGLE, MAGIC DRAGON — ties back
to `IsItemRangeAvailable`'s documented "boat/horse-style transport
gate" use case from earlier this session.

293 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunClueBookWeaponCategory completes the F5 item-subtype map

Named `sub_1318D` (title dumped as "WEAPONS"), F5 item-subtype 8's
category loop, structurally identical to `RunClueBookItemCategory`
(subtype 1). This completes identification of all 8 F5 subtypes: 1
ARMOR/RINGS, 2 (empty placeholder), 3 JEWELS/ARTIFACTS/UNIQUE ITEMS, 4
MAGIC SCROLLS/QUARTZ, 5 POTIONS, 6 SUPPLIES/FOOD, 7 TRANSPORTATIONS, 8
WEAPONS — closing out this session's full trace of `ShowClueBook`'s
category dispatch.

294 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunEnhanceItemScreen and RunRepairItemScreen

Named the two remaining entry points completing the shop cluster,
siblings of `RunSellItemScreen`: `RunEnhanceItemScreen`
(`UseItem+0x1C1`, sets `word_328C6` bit 8) and `RunRepairItemScreen`
(`UseItem+0x1D0`, sets bit 4) — both otherwise identical to
`RunSellItemScreen`. All three of `UseItem`'s shop entry points are
now named.

296 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RefreshPartyPortraits

Named `sub_185A2`, called from `start`, `HandleDungeonInput`, and all
three shop screens: refreshes the 4 party portrait slots, and when a
shop action bit is active also draws a context hint ("SPACEBAR TO
ENHANCE/REPAIR ITEM", or the default sell hint).

297 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowTransportUsagePreview

Named `sub_2909C`, called from `ShowItemUsagePreview`: draws the
transport/mount item-use preview (name, cost, flight-time restriction)
using the same 4-slot PEGASUS/GIANT EAGLE/MAGIC DRAGON table as
`ShowClueBookTransportDetail`.

298 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawLabeledBCDIfNonzero and DrawRecordFieldBCDIfNonzero

Named two small reused display helpers ("draw a label, then the BCD4
value only if nonzero"), shared between `ShowClueBookItemDetail` and
`ShowClueBookMonsterDetail`'s stat fields. One takes a value pointer
directly, the other a record pointer.

300 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SpawnMonsterInFacingDirection

Named `sub_22A68`, the per-level monster spawn function: finds an
empty `g_levelMonsters` slot, loads the monster's catalog record from
`WORLD.DAT`, computes a spawn position offset from the party's current
facing direction (same tier bits as `ShowCompassDirection`), sets a
countdown timer and full HP. Called from an untraced movement/trigger
handler (`sub_212B8`) — a first foothold into the combat-encounter
trigger system, not chased further this round.

301 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RenderDungeonViewport reveals the first-person corridor renderer and encounter gating

Traced `SpawnMonsterInFacingDirection`'s caller chain one level
further and found the **first-person dungeon corridor viewport
renderer**: `RenderDungeonViewport` (was `sub_20FB7`) calls
`RenderDungeonViewRow` (was `sub_21015`) six times with decreasing
cell counts, one per depth row of the visible corridor.
`RenderDungeonViewRow` draws each cell's picture and calls
`TryTriggerMonsterEncounterAtCell` (was `sub_212B8`) once per cell.
Key finding: the encounter check only fires for the farthest two rows
(`word_3292C >= 0x11`) — **monsters can only spawn at the edge of
visibility, never right next to the party** — a deliberate fairness
design, not an incidental detail. This is a genuine architectural
discovery about the game's core rendering/encounter loop, found by
following one function's caller chain rather than starting from this
subsystem directly.

304 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RedrawDungeonScreen and RefreshDungeonScreen

Named `RenderDungeonViewport`'s two direct, `start`-reachable callers:
`RedrawDungeonScreen` (fuller setup) and `RefreshDungeonScreen`
(lighter, plus a conditional minimap redraw). Exact trigger
distinguishing the two not traced.

306 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawDungeonCellWallTexture

Named `sub_21128`, called from `RenderDungeonViewRow` per visible
cell: draws the cell's base wall texture (0xE551 lookup table) plus a
conditional overlay picture (door/torch/decoration marker, not
confirmed).

307 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawDungeonCellSideFeature and TryDrawDungeonCellSideFeature

Named the door/side-feature sprite drawer for the dungeon viewport:
`TryDrawDungeonCellSideFeature` (null-check wrapper) calls
`DrawDungeonCellSideFeature`, which indexes a facing-direction table
by the cell's side-feature id, with a conditional open-door/lit-torch
overlay.

309 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RenderDungeonVanishingPoint

Named `sub_21217`, `RenderDungeonViewport`'s 7th/final call: draws the
far-wall/vanishing-point cells at the end of the visible corridor,
then runs the same per-cell side-feature and encounter checks as
`RenderDungeonViewRow` for the final cell.

310 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RenderActiveMonsterSprites

Named `sub_212EB`, called from `RenderDungeonVanishingPoint` when
`word_328CA` bit `0x1000` is set: iterates the 3 `g_monsterSlots`
records and draws each occupied one's sprite into the dungeon
viewport.

311 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — FindMonsterTypeInLevelPool is a dedup check, not a probability roll

Named `sub_22B78`. Re-tracing `TryTriggerMonsterEncounterAtCell`'s
call to it shows it's a duplicate-prevention lookup (scans
`g_levelMonsters` for an existing entry of the same monster type,
skipping the spawn if found) — not "a probability check" as
`TryTriggerMonsterEncounterAtCell`'s own naming comment claimed two
rounds ago. Corrected both that comment and file-formats.md.

312 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TryActivateMonsterByDistance

Named `sub_233F5`, called from `SpawnMonsterInFacingDirection` and
`FindMonsterTypeInLevelPool`: a distance-based "the party notices this
monster" activation check, comparing the render-depth counter against
a per-monster detection-range threshold before setting an aware flag.

313 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawMonsterAndUpdateAttackState connects dungeon rendering to the existing combat/wound documentation

Named `sub_20E54`, called from `TryTriggerMonsterEncounterAtCell` and
`RenderActiveMonsterSprites`: draws a monster's sprite (base picture,
hit-flash/recovery animation via `+0xC` bits 2/4 — the same wound
flags `UpdateMonsterWoundTier` sets, documented earlier this session)
plus a weapon/attack-effect sprite, then checks the same `+0xC 0x3010`
bits `BuildCombatTurnOrder`/`TickMonsterTimer` use for countdown
gating. Nicely ties this round's dungeon-rendering work back to
earlier-session combat findings.

314 named of 769 functions as of this update.

### 2026-09-15 session update, continued: AdvanceMonsterAnimationFrame

Named `sub_25656`, called from `DrawMonsterAndUpdateAttackState` (the
non-attacking case) and reused by `ShowClueBookMonsterDetail`:
advances a monster's idle/walk animation frame within a small cycle
relative to a base frame.

315 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawDungeonFloorAndCeiling and ExtendDungeonFloorTexture

Named a separate rendering pass that runs before `RenderDungeonViewport`:
`DrawDungeonFloorAndCeiling` draws the sky/floor backdrop, then
`ExtendDungeonFloorTexture` extends the floor texture across cells
sharing the same floor type — a simpler "seamless floor" pass,
distinct from `RenderDungeonViewRow`'s full wall/object rendering.

317 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ExtendDungeonCeilingPass completes the dungeon render sequence

Named the ceiling counterpart to `DrawDungeonFloorAndCeiling`/
`ExtendDungeonFloorTexture`. The full dungeon-screen render sequence
is now clear: 1) backdrop + floor-extension, 2) ceiling-extension, 3)
`RenderDungeonViewport`'s full wall/door/monster/encounter rendering.

319 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawViewportSprite — the shared primitive behind every dungeon-render pass

Named `sub_29B0F` (632 lines, internals not traced): the core
sprite/picture blitter every viewport-rendering function this session
calls with a consistent (picture id, scale class, z-layer, transparency)
signature — the depth-aware counterpart to `DrawPicture`. Closes out
this session's dungeon-rendering arc: spawn → activate → floor/
ceiling/wall passes → monster sprites/animation → the shared blitter
underneath all of it.

320 named of 769 functions as of this update.

### 2026-09-15 session update, continued: BuildDungeonViewportCells and CopyDungeonRowCells

Named the setup step that runs before all three dungeon render
passes: builds the local scratch cell buffer they all read from,
copying the visible cells out of the level's map data using a
facing-dependent row stride. Traces the full path from "current
position + facing direction" to "cells the render passes see."

322 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ComputeDungeonCellVisibility identifies the occlusion flag's origin

Named `sub_213FC`, called right after `BuildDungeonViewportCells` in
`RedrawDungeonScreen`: computes dungeon line-of-sight occlusion,
marking cells behind wall corners with the `[+6]` bit-0 "hidden" flag
that every render-pass function checks. This is that flag's origin —
closes the loop on why some cells in the copied buffer don't get
drawn.

323 named of 769 functions as of this update.

### 2026-09-15 session update, continued: IsDungeonRowFullyBlocked

Named `sub_214F4`, called from `ComputeDungeonCellVisibility`: the
per-row "is every cell here a solid wall" dead-end test used to find
the occlusion boundary. Completes this session's full trace of the
dungeon-rendering pipeline, from facing direction through cell
copying, visibility computation, three render passes, sprite
blitting, and monster AI.

324 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyDamageToMapMonster

Named `sub_2D370`: applies damage to a `g_levelMonsters`-pool monster
in the corridor (not yet in turn-based combat), then resolves death
(reusing `GrantMonsterRewards`/`RemoveMonsterFromMap`/
`RedrawDungeonScreen`) or survival (`RefreshDungeonScreen`). Ties this
round's dungeon-rendering names back into earlier-session combat/loot
findings.

325 named of 769 functions as of this update.

### 2026-09-15 session update, continued: InitializeDungeonLevel

Named `sub_1F0CD`, called from `start`: initializes/enters a dungeon
level — copies a per-level metadata template, reveals cells around
the player, redraws the screen/minimap, and resets combat state
(zeroes `g_monsterSlots`, clears the active-combat-monster global).

326 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SelectGameDialogOption

Named `sub_1F29D`, `RunGameDialog`'s input handler: polls keyboard/
mouse to pick one of its 8 icon options (Animation/Dos/Return/Load/
Music/NewGame/Save/SoundFx).

327 named of 769 functions as of this update.

### 2026-09-15 session update, continued: EditWallLegendTypeNumber and EditFloorLegendTypeNumber

Named a symmetric pair of numeric-entry fields in `RunMapEditorScreen`
(a pre-existing but previously undocumented name): each reads a
wall/floor type number and redraws the corresponding legend row. One
error path suggests Tab-style navigation between the two fields.

329 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — the "legend highlight" cluster is actually the ordinary dungeon floor/ceiling render

An earlier round this session, while documenting the hidden map-editor
screen (`RunMapEditorScreen`), flagged a "sibling cluster"
(`sub_20C8E`/`sub_20CEC`/`sub_20D2F`/`sub_20E12`/`sub_29FF6`) as
drawing "two small 'current cell class' preview boxes... to highlight
the matching legend icon" — structure understood but not confirmed.
This round's dungeon-rendering trace independently named all of these
(`ExtendDungeonCeilingPass`/`ExtendDungeonCeilingTexture`/
`DrawDungeonFloorAndCeiling`/`ExtendDungeonFloorTexture`) and found
that reading doesn't hold up: they draw the actual floor/ceiling
pictures for the ordinary first-person corridor view, called from
`RedrawDungeonScreen`/`RefreshDungeonScreen`, not a map-editor
legend-highlight box. Also narrows the open question about
`word_328E6`..`word_328F2`'s write site: now semantically understood
(per-row source-cell pointers computed via `BuildDungeonViewportCells`)
even though the literal write instruction still isn't found.
`file-formats.md` corrected in place; this entry left standing as the
historical record of the original (wrong) framing.

### 2026-09-15 session update, continued: ClearVideoMemoryRegion

Named `sub_203F7`, called from `RunMapEditorScreen`: a partial VGA
video-memory clear (2560 bytes at `0xA000:0000`, not the full frame).

330 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PrepareWorldDatRead and LoadWorldDatTilePalette

Named a generic `WORLD.DAT` FileEntry setup helper (a sibling of
`WorldDat_setBlock1`-`6`) and its tile-palette-loading caller, feeding
`RunMapEditorScreen`'s B/F palette-browsing keys and
`DrawClueBookMapGrid`.

332 named of 769 functions as of this update.

### 2026-09-15 session update, continued: BrowseWallTilePalette and BrowseFloorTilePalette

Named `RunMapEditorScreen`'s `B`/`F` palette-browsing key handlers:
pick a wall/floor type from the per-level tile palette at the clicked
position, storing it into the same fields
`EditWallLegendTypeNumber`/`EditFloorLegendTypeNumber` write. Closes
out this session's coverage of the hidden map-editor screen.

334 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RedrawMapEditorGrid

Named `sub_206A0`: redraws the map editor's full visible 40×24 cell
grid, the same area `FillVisibleAreaWithSelectedTile` floods.

335 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ResolveAttackOrAbilityAction ties dungeon rendering into ranged/spell combat

Named a 5-function cluster resolving ranged attacks and area-effect
spells against a corridor monster: `ResolveAttackOrAbilityAction`
(ranged weapon vs. spell/ability, selected by a caller-set flag) →
`ResolveAbilityEffect` (85% success roll, dispatches on the ability id
to set damage + status-effect flags) → for the 2 area-effect ability
ids, `ApplyDamageAlongCorridorLine` (hits 3 consecutive depth rows) →
`GetMonsterAtViewportRow` (looks up the *same* `0x6D60` scratch buffer
`RenderDungeonViewRow` reads for a monster at that row) →
`ApplyResolvedDamageWithResistance` (the shared damage step, reducing
via a resistance bit-scan). Directly ties this session's earlier
dungeon-rendering work into the combat/targeting system, and matches
the "IN A STRAIGHT LINE"/"IN A 3X3 AREA" spell-targeting text already
dumped from `ShowClueBookSpellDetail`.

340 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandleRangedOrCombatAction and the projectile-travel animation

Named `sub_1D4B8` (the combat-action entry point called from `start`)
along with `AnimateProjectileStep` and `ClassifyObstacleAtViewportRow`
— the projectile animates down the corridor one depth row at a time,
checking each row for a wall/door/monster via the same dungeon-
viewport scratch buffer `RenderDungeonViewRow` reads, until it hits
something and (for a monster) hands off to
`ResolveAttackOrAbilityAction`. `HandleRangedOrCombatAction` is a
3-way dispatcher; its formal-combat and spell-cast-opening branches
aren't traced yet, only the fully-worked-out ranged-weapon-shot path.

343 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowCombatMessageOrWait

Named `sub_1DA42`, `HandleRangedOrCombatAction`'s message helper:
shows a combat message unless speech/sound is currently busy, in
which case it just waits 6 ticks instead.

344 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HighlightSelectedAbilityIcon; traced the melee and area-spell-finish branches

Named `sub_1D937`, and while tracing it also worked out
`HandleRangedOrCombatAction`'s other two branches in full: the
in-combat melee path (much simpler — attacks `word_32A1E` directly,
no row search) and the area-effect spell finish (a 10-frame explosion
animation, then a level-wide sweep of all 80 `g_levelMonsters` slots
for kills — not just the 3 rows the attack itself touched). No new
renames from this trace beyond `HighlightSelectedAbilityIcon` — the
value was in fully understanding the combat-action dispatcher's shape.

345 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandleRangedOrCombatAction's spell-cast branch fully traced

Finished tracing the last untraced branch: the spell/ability-cast
opening (not-ranged, not-in-combat) turns out to converge into the
*exact same* row-by-row projectile scan the ranged-weapon branch
uses — both paths share the code after `HighlightSelectedAbilityIcon`.
Also found a multi-shot continuation (characters with more than one
attack per turn loop back to the next depth row) and the common
epilogue every path funnels through (loot-staging check →
`ShowLootAndAwardExperience` → `ProcessLevelMonsters` → redraw).
`HandleRangedOrCombatAction` is now fully mapped end to end.
Documentation only, no new renames.

### 2026-09-15 session update, continued: TravelToDestination and IsDestinationUnlocked

Named the party teleport/fast-travel system: `TravelToDestination`
(called from `start` and `ExamineTarget`) looks up a destination table
for the new position/facing and an optional message, gated by
`IsDestinationUnlocked`'s separate eligibility table when required.

347 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RestorePortraitPanelFromEMS

Named `sub_1922C`, called from `RefreshPartyPortraits`: when no
portrait-dirty bits are set, blits a cached background region from
EMS-paged memory straight into the video buffer instead of a full
redraw.

348 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandlePortraitClick

Named `sub_18504`, the mouse-click counterpart to the keyboard `1`-`4`
party-panel selector: hit-tests the 4 portrait zones and highlights
the clicked member.

349 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowPartyPortraitForSlot and DrawPartyMemberPortrait

Named the party-member portrait renderer used by
RefreshPartyPortraits/HandlePortraitClick: character icon, status bar,
and a condition icon. Full internal icon-selection logic not
individually traced.

351 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SellClickedCatalogItem

Named `sub_17B09`, a sibling branch of `sub_17032` (the shop-catalog
click handler) alongside `PayGoldAndAcquireItem`: credits gold back
for the clicked item instead of spending it — the click counterpart
to `TrySellItemForGold`.

352 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HitTestCatalogSlot

Named `sub_17B67`, the click gate for `sub_17032`'s buy/sell branches:
hit-tests region table `0x63C8` plus an 8-entry exclusion check.

353 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawTransportDetailRow

Named `sub_13EDF`, called 3x from `ShowClueBookTransportDetail`: draws
one mount's row — name, "VALUE:" (price), "USES:" (a count), "TIME:"
(flight window, "BETWEEN...AND..." or "ANYTIME").

354 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunClueBookItemDetailWithAbilityInfo

Named `sub_13119`, the F5 item-subtype 3-6 loop left unnamed as an
open lead earlier this session. Draws via `ShowClueBookItemDetail`,
then adds an extra ability-info overlay when the item's id falls in
`CastSpell`'s or `RestCharacter`'s dispatch range — some clue-book
items (plausibly Magic Scrolls) grant a spell when used, and the clue
book shows what it does. Closes out an old open thread.

355 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowItemEffectDuration and ShowItemAbilityEffectInfo

Named the clue-book "ability info" overlay cluster: `ShowItemEffectDuration`
("DURATION- `<n>` MINUTES") and `ShowItemAbilityEffectInfo`, plus the
shared `DrawLabeledNumberIfNonzero` helper. Notable cross-reference:
`ShowItemAbilityEffectInfo`'s effect-amount display uses the *exact
same* damage constants as `ResolveAbilityEffect`'s combat dispatch —
direct proof the clue book shows real in-engine numbers, not flavor
text.

358 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowArmorDetailRow and ShowWeaponDetailRow

Named the per-subtype extra stat rows for F5 item subtypes 1 (ARMOR/
RINGS: "ABSORPTION-") and 8 (WEAPONS: "DAMAGE:" and "2-HANDED: YES/NO"),
shown after the generic BASE VALUE/WEIGHT fields.

360 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawSubIconSelectorRow

Named `sub_14994`, shared by `ShowArmorDetailRow`/`ShowWeaponDetailRow`:
draws the clickable sub-icon selector strip using the same region
table `RunClueBookItemCategory` hit-tests for its own sub-icon clicks.

361 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowHealingItemPercentInfo

Named `sub_137C3`, the last piece of the item-ability-overlay cluster:
shows "HEALTH-"/"MAGIC-" plus a percentage for healing/restore items,
matching `UseHealingItem`'s own HP/MP flag convention.

362 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowArmorProtectionsList and ShowArmorAttributeBonusList — a major reference find

Named `ShowArmorDetailRow`'s two bonus-list drawers.
`ShowArmorAttributeBonusList`'s lookup table (`0x7DC7`) turns out to
be the **canonical index order of the game's entire attribute/skill
system** — Strength/Dexterity/Stamina/Intelligence/Wisdom/Charisma,
Hit Points/Magic Points, then Survival/Projectile/Slashing/Bashing/
Polearm/Casting/Mapping/Navigation/Bartering/Repair/Thievery/
Linguistics/Chemistry and more. Previously this skill list was only
known piecemeal from a raw string scan; this gives its actual
in-engine index order — a strong candidate for cross-referencing
against the party-record skill array and `ShowCharacterSkills`.
`ShowArmorProtectionsList` similarly names the 9 affliction-protection
types armor can grant.

364 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawEquippedItemIcons

Named `sub_267A7`, called from `DrawPartyMemberPortrait`: draws the
equipped-item icons next to a party member's portrait, using an
"active" icon variant when an item is currently in use.

365 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ClassifyObstacleAtWorldPosition

Named `sub_111C1`, called from `ProcessLevelMonsters`: the `WORLD.DAT`-
backed counterpart to `ClassifyObstacleAtViewportRow`, checking a
monster's target cell anywhere on the level (not just the rendered
viewport) for obstacles before it moves.

366 named of 769 functions as of this update.

### 2026-09-15 session update, continued: BuildItemUseMessage

Named `sub_1BA96`, called from `FinishItemUse` and
`ShowItemUsagePreview`: builds the confirmation/preview message text
for using an item — either a generic message by tier, or item-specific
entries copied from an EMS-backed segment.

367 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CheckAndPaySpecialItemCost

Named `sub_1C809`, called from `BuildItemUseMessage`: checks and pays
a special item's usage cost, selected by a tag on the item — gold,
NUORE, MAGIC ORE, or a specific consumed inventory item.

368 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RestoreCorridorBackgroundFromEMS; correction — not a sound effect

Named `sub_2BC16` and its parameterized sibling `sub_2BC72`: EMS-backed
background-restore/scroll blitters for the dungeon viewport, used by
`AnimateProjectileStep` to redraw the corridor behind the projectile
sprite each step. **Correction**: `AnimateProjectileStep`'s earlier
description guessed this call "plays a sound" — wrong, it's graphics,
not audio. Corrected the comment and file-formats.md.

370 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowAlchemyIconActive and ShowAlchemyIconIdle

Named two small alchemy-screen icon drawers, called from
`RunAlchemyScreen`. Exact narrative (what the two icon states
represent) not confirmed.

372 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawWeaponSelectIcon

Named `sub_2BAD5`, called 4x from `HandleRangedOrCombatAction`: draws
one weapon-select slot icon, with a highlighted variant for the
currently selected weapon.

373 named of 769 functions as of this update.

### 2026-09-15 session update, continued: AnimateEffectFrame

Named `sub_2D3DC`, called from the large unnamed dispatcher
`sub_2C0FE`: one animation frame, same shape as
`AnimateProjectileStep`, for some other in-viewport effect sequence.

374 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SaveCorridorBackgroundToEMS and SaveActionIconPanelToEMS

Named the EMS-backed background *save* functions (mirror image of
`RestoreCorridorBackgroundFromEMS`), caching the corridor viewport and
action-icon panel areas before animated overlays draw over them.

376 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TryTravelToClickedMapCell

Named `sub_29297`, called from `RevealMapRegion`: click a cell within
the revealed area (gated on its "explored" bit) to instantly travel
the party there — a scry-then-teleport interaction, consistent with a
Locate/Scout/Magic-Mapping ability rather than a correction to
`RevealMapRegion`'s existing documentation.

377 named of 769 functions as of this update.

### 2026-09-15 session update, continued: IsCellTypeImpassable and ClassifyFloorType

Named the two cell-type classification checks shared by
`HandleMovementInput` (ordinary movement) and
`TryTravelToClickedMapCell` (click-to-travel): one a simple blocked/
passable check, the other a finer classification into normal/special-
terrain/narrow-low-range codes.

379 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ConfirmAndSelectPartyTarget

Named `sub_2AD94`, called from `ApplyMultiStatEffect` and
`RestCharacter`: shows a confirm prompt, then resolves the selected
party record on confirmation.

380 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ReadMapCellAttributeByte

Named `sub_28CB1`, an open lead flagged much earlier this session
("don't force a name"): a single-byte `WORLD.DAT` block-3 read for a
map cell, called from `RevealMapRegion`/`RevealMapRegionRow`. Exact
meaning of the byte still not identified, but the mechanism is now
confirmed and documented.

381 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ComputeMapCellIndex and DrawRevealedCellIcon

Named the other two open leads from the same early-session flag as
`ReadMapCellAttributeByte`: a coordinate-to-index conversion and the
minimap-style icon drawer for a newly-revealed cell. Both called from
`RevealMapRegion`/`RevealMapRegionRow`. Closes out that entire old
open-lead group.

383 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawPortraitAccessoryIcon

Named `sub_26778`, called from `DrawPartyMemberPortrait`: draws one
item icon with a two-variant toggle, the same pattern
`DrawEquippedItemIcons` uses.

384 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawPlayerPositionMarker — halfway milestone

Named `sub_22255`, called from `ToggleMapViewMode`: draws a "you are
here" marker on the local area map at the party's current position.

**385 named of 769 functions as of this update — exactly half the
executable is now named**, up from 45 (5.8%) at the very start of this
project's work on `SW.EXE`.

### 2026-09-15 session update, continued: TryCureAilmentFromIconClick

Named `sub_2738B`, initially misread as a generic 6-slot equipment
icon bar — caught before committing: the table it reads (`0x9519`) is
already documented elsewhere as the 6-entry *ailment* table
`TickWorldAilments` walks, not a generic item table. Corrected to
`TryCureAilmentFromIconClick`: click handler for the active-ailment
icon bar, staging the clicked ailment (loaded via the item-catalog
path — ailment codes and item ids appear to share a numbering space)
into the "carrying" state, plausibly to apply a held cure item to it.

386 named of 769 functions as of this update.

### 2026-09-15 session update, continued: IsItemTypeAcceptedByLocation

Named `sub_2D5E0`, structurally identical to the already-named
`IsItemEligibleForRepair` (a separate function instance, same logic):
a generic "does this context accept this item type" gate, reused by
`TryCureAilmentFromIconClick` and a branch of the main input loop.

387 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawStatBar

Named `sub_226FC`, called from an unnamed function (`sub_22445`)
reached directly from the main input loop: draws a 5-row proportional
health/mana-gauge-style stat bar.

388 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawPartyMemberStatusPanel

Named `sub_22445`, called from the main input loop: a fuller
combat-style status panel per party slot -- portrait, unconscious/dead
overlay, three `DrawStatBar` gauges (HP, MP, an unidentified third
stat), an ability-readiness icon, and level-up/training text.

389 named of 769 functions as of this update.

### 2026-09-15 session update, continued: UpdateAmbientMusicForRegion

Named `sub_25608`: computes a coarse map-region index from the
party's position and, when it changes, plays that region's music
track — the ambient-music-by-zone trigger.

390 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RestPartyAndAdvanceClock — the "R rest" command

Named `sub_1E64A`: the party rest/camp action. Advances the game
clock (8 hours flat for a full rest, or up to 8 hourly ticks with
`ProcessLevelMonsters` and early combat interruption), inlining the
exact same day-rollover math `AdvanceGameClock` uses (wraps at 1440
minutes) and calling `ResetDailyAbilityCharges` on rollover — a clean
cross-confirmation of the whole clock/calendar system documented
earlier this session, from a completely different entry point.

391 named of 769 functions as of this update.

### 2026-09-15 session update, continued: IsRestingAllowedHere and IsPositionInTriggerList

Named `RestPartyAndAdvanceClock`'s eligibility check (rejects on a
global flag, forbidden map/level id, or a special-cell match) and a
shared position-list lookup also used by `ApplyMapTriggerEffect`.
Confirmed by the dumped rejection message "YOU CAN NOT REST HERE".

393 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RestoreDialogAreaFromEMS

Named `sub_28246`, shared by `RestPartyAndAdvanceClock` and
`RunGameDialog`: restores a cached status-area screen region from EMS
without a full redraw.

394 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ClearMessageBoxArea

Named `sub_16EFA`, shared by `sub_17032`, `UseAbilityCommand`, and
`RestPartyAndAdvanceClock`: a generic message/status-box background
clear, sized by combat state.

395 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ConfirmAndValidatePartyTarget

Named `sub_1B2BD`, a `UseItem` branch for the "BUY "-named item: a
confirm prompt to pick a party member, re-prompting with a warning if
the pick is incapacitated.

396 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TryDropHeldItem, IsItemDroppable, PlaceItemOnGround

Named the "drop held item" cluster: a confirm-gated drop action whose
placement step recurses into a dropped container's contents (the
same container flag already documented for the 3 alternate bags), so
a dropped bag's full contents are placed too.

399 named of 769 functions as of this update.

### 2026-09-15 session update, continued: UpdateCursorForHeldItem

Named `sub_23874`, called throughout this session's item-manipulation
functions: updates the mouse cursor to show the currently-held item's
icon. A widely-reused helper, now named across dozens of call sites.

400 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SelectPartyRecordById

Named `sub_25B14`, an extremely widely-called foundational function:
resolves a 1-based party-record id into `word_328D4`/`word_328D6`.
Fills in a gap flagged earlier this session — "exactly how 'the
current member' gets chosen isn't fully mapped yet" — this is the
main mechanism.

401 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RestorePortraitAreaAtPosition

Named `sub_190E9`, called from the main input loop and `sub_18F6C`: a
per-slot-position variant of the portrait EMS-restore pattern, using
the same EMS page as `RestorePortraitPanelFromEMS`/
`RestoreDialogAreaFromEMS`.

402 named of 769 functions as of this update.

### 2026-09-15 session update, continued: WaitForTargetClick

Named `sub_2940E`, called from `UseAbilityOnTarget` and
`UnlockDoorCommand`: a generic targeting-mode wait loop with a
crosshair cursor, waiting for ESC or a valid dungeon-viewport click.

403 named of 769 functions as of this update.

### 2026-09-15 session update, continued: IsContainerTypeCompatible

Named `sub_26C22`: checks whether the currently-open alternate bag
matches an allowed-type bitmask before letting an item be placed into
it.

404 named of 769 functions as of this update.

### 2026-09-15 session update, continued: LoadNextContainerInChain

Named `sub_26022`: walks a linked chain of container/world-object
records via `CURGAME` (each record's own `[+8]` field points to the
next id), loading each via `LoadContainerContents` in turn.

405 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawPortraitOverlayIconA and DrawPortraitOverlayIconB

Named two small overlay icon drawers called from
`DrawPartyMemberPortrait`. Exact narrative not confirmed.

407 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HasDroppableItemInInventory and HasDroppableItemInContainer

Named `IsItemDroppable`'s recursive helpers: a held container is only
droppable if it holds at least one directly-droppable item somewhere
inside it.

409 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DetectSoundDriver

Named `sub_284CB`: scans DOS interrupt vectors for an installed sound/
music driver's signature, allocating its buffer and setting driver
state flags on success.

410 named of 769 functions as of this update.

### 2026-09-15 session update, continued: InitSoundSystem

Named `sub_283EA`, called from `InitGame`: the top-level sound/music
driver initialization, gating `DetectSoundDriver` and a sibling
function on possible sound/music disable flags.

411 named of 769 functions as of this update.

### 2026-09-15 session update, continued: InitMusicDriver

Named `sub_28564`, called from `InitSoundSystem`: initializes the
sound driver's hardware config and calls into it via
`g_soundDriverFarPtr`'s function-selector interface, then allocates
its music-data buffer.

412 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PrepareMusicDataRead

Named `sub_27C5A`, called from `InitMusicDriver`: sets up a WORLD.DAT-
style read context for the sound driver's music/instrument data
block.

413 named of 769 functions as of this update.

### 2026-09-15 session update, continued: WaitForClickOrEscape

Named `sub_255C7`: a generic "wait for a click or ESC" loop, ticking
the already-named `UpdateAmbientMusic` each iteration.

414 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RestoreWorldMapAreaFromEMS

Named `sub_2438B`, called from `ShowWorldMap`'s interaction handler:
restores the world map display area from an EMS cache.

415 named of 769 functions as of this update.

### 2026-09-15 session update, continued: WriteTwoToneString

Named `sub_23A7C`: draws a string with the first character in one
color and the rest in another -- a highlighted-hotkey-letter label
style, used in several unrelated screens.

416 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawThreeThresholdStats and DrawValueWithThresholdColor

Named a 3-value threshold-highlighted stat drawer called from
`ShowCharacterSkills`, reading party-record fields `+0x4C`/`+0x4E`/
`+0x50` immediately before the confirmed HP/MP pairs — plausibly 3
primary attributes, not confirmed which.

418 named of 769 functions as of this update.

### 2026-09-15 session update, continued: correction — DrawThreeThresholdStats is not the 6 primary attributes

Checked `RollCharacterAttributes`' own pre-existing comment (called
right before `DrawThreeThresholdStats` in `ShowCharacterSkills`): the
6 core attributes are already confirmed at `+0x3C`–`+0x86`, a
completely different offset range from `DrawThreeThresholdStats`'
`+0x4C`/`+0x4E`/`+0x50`. Corrected last entry's guess; those 3 fields'
actual identity is still open, though they're drawn on the same
post-roll screen so presumably related. Renames only, no new count.

### 2026-09-15 session update, continued: ComputeDerivedCharacterStats

Named `sub_23F58`: computes a family of derived character stats from
the 6 base attributes (weighted percentage blends plus class-dependent
bonuses), mirrored into `+0x58`/`+0x98`, `+0x5A`/`+0x9A`,
`+0x5C`/`+0x9C`, `+0x5E`/`+0x9E`, `+0x60`/`+0xA0`, and more. Confirms
`+0x58` (already documented as gating `DrawMonsterInfoPanel`'s
detail-reveal) is derived, not raw-rolled.

419 named of 769 functions as of this update.

### 2026-09-15 session update, continued: GetClassNameString, DrawPartyRosterEntry

Named `sub_19768` -> `GetClassNameString` and `sub_2BFBC` ->
`DrawPartyRosterEntry`. `GetClassNameString` takes the party record's
`+0xE` field (already documented as "plausibly a class id") through the
same `cmp 9 / -0xA / cmp 9 / -0xA` dispatch seen in `RestCharacter` and
`UseTrainingItem`, and returns a pointer into a real 27-entry
class-name string table — dumping it gave the game's actual class list
(FIGHTER, MERCHANT, ROGUE, MONK, ALCHEMIST, PALADIN, MAGE, DRUID,
MARKSMAN, WARRIOR, TINKERER, THIEF, CLERIC, TRANSMUTER, CAVALIER,
WIZARD, ENCHANTER, RANGER, CHAMPION, BLACKSMITH, ASSASSIN, PRIEST,
HEALER, HERO, SORCERER, SAGE, KNIGHT), upgrading `+0xE` from "plausibly"
to confirmed.

This also let me correct a stale comment on `ShowWorldMap`: its loop
over the 9-slot `g_partyRecords` array was previously guessed to be
placing "location markers" on the map. `DrawPartyRosterEntry` calls
`GetClassNameString` and draws a name+class label per row, which only
makes sense for player characters — so this is a party roster listing
(with digit-key slot selection and what looks like a recruit/dismiss
toggle), not a set of town markers. See
[file-formats.md](file-formats.md#the-party-roster-screen-showworldmap)
for the full writeup; `sub_23C18` (the detail screen opened on
selection) remains an open, untraced lead.

421 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawCharacterClassAndLevel, ShowCreateCharacterPrompt cluster

Named `sub_2504F` -> `DrawCharacterClassAndLevel` (draws the current
character's class name plus `+0x16`, shared by `ShowCharacterSkills`
and the still-untraced `sub_23C18`) — upgrades `+0x16` from "plausibly
a level/skill stat" to confirmed character level.

Also named a small cluster found while investigating `sub_25544`
(called from `ShowPartyMembers`): `ShowCreateCharacterPrompt` (was
`sub_25544`) uses `SelectDefaultPartyRecord`'s empty-slot scan to find
a free roster slot, then calls `ClearPartyRecord` (was `sub_243C3` —
zeroes exactly one 500-byte `g_partyRecords` stride, an independent
confirmation of that stride) and `DrawFullScreenPictureAndCacheToEMS`
(was `sub_22387` — a generic full-screen draw-and-cache-to-EMS utility
used by ~11 different screens, including `InitGame` and
`RunDungeonGameLoop`) before writing "CHARACTER CREATION" — the party
roster's entry point into character creation.

425 named of 769 functions as of this update.

### 2026-09-15 session update, continued: FinalizeCharacterCreation, BuildMonsterDisplayName

Named `sub_15267` -> `FinalizeCharacterCreation`: the character-creation
wizard's common finalize/cleanup step, called from
`RunCharacterCreation` regardless of which of the 3 ESC-cancelable
steps was reached (that function's own pre-existing comment already
called it "a finalize step" — this just names it). Loads a transition
palette, reads file entry #3, frees a temp memory block if allocated,
clears the screen, and stops the character-creation music.

Named `sub_14B85` -> `BuildMonsterDisplayName`: called only from
`BuildClueEntryText` and `ShowClueBookMonsterDetail` (the F8 clue
book's monster-stats cluster). Uses the same
`WorldDat_setBlock5`/`FileEntry_Read(errorCode=9)` pattern as the
already-named `LoadClueBookMonsterEntry` (which reads `WORLD.DAT` block
`0x32`, "MONSTER STATISTICS") to build a space-joined two-field display
string — plausibly a monster's name and type/category label, though the
exact field semantics aren't independently confirmed.

427 named of 769 functions as of this update.

### 2026-09-15 session update, continued: IsMonsterStepBlocked

Named `sub_2B384` -> `IsMonsterStepBlocked`, moderate confidence:
called once from `ProcessLevelMonsters` right after it computes a
one-cell step toward the player's position, to validate that step
before moving a monster. Cell flag bits `0xC00` always block; bits
`0x6000` need a monster trait flag (`[+0x94]` bit `0x10`); a few more
branches gate on other `[+0x94]` bits and value ranges; otherwise falls
through to `ClassifyFloorType`/`IsCellTypeImpassable`, the same pair
used for plain terrain checks elsewhere. The `[+0x94]` trait bits'
exact meaning (flying/incorporeal/door-opening monsters?) isn't
confirmed — left as an open lead rather than guessed.

Also checked `sub_28412` (the sound-driver dispatch function flagged
in an earlier round's `document_sound_dispatch.py` as deliberately
*not* renamed pending confirmation of its command-6 meaning) —
confirmed its sibling `sub_2849C` is already named
`StopMusicAndResetTimer` from earlier work, but that doesn't resolve
command 6 itself, so leaving `sub_28412` unnamed rather than guessing.

428 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CheckAndAnnounceLevelUp, MulBCD4ByWord, ComputeBarterPricingPreview

Named `sub_1B7DD` -> `CheckAndAnnounceLevelUp` (called from `UseItem`
and `UseTrainingItem`): resolves the active party slot via
`word_32924`/`g_partySlotAssignment`, calls `CheckForLevelUp`, and
shows `ShowLevelUpMessage` if `+0x1E` (pending level-up) is nonzero —
the standard post-item-use level-up check shared by both callers.

Named `sub_19CA1` -> `MulBCD4ByWord`: a new sibling of the existing
BCD4 arithmetic library (`ConvertWordToBCD4`/`CompareBCD4`/`AddBCD4`/
`SubBCD4`) — multiplies a packed-BCD4 value digit-by-digit by a 16-bit
word via repeated BCD addition, writing the product back in place.

Named `sub_1CCBC` -> `ComputeBarterPricingPreview` (called from
`PayGoldAndAcquireItem` and `SellClickedCatalogItem`): computes a
tiered discount/markup percentage from party record field `+0x68`
(plausibly the **BARTERING** skill, given the shop context and the
attribute/skill list found earlier this session) and applies it via
`MulBCD4ByWord` to preview a scaled price at `0xB30` — the same buffer
`PayGoldAndAcquireItem` charges against `g_partyGold` — plus scaled
enhance/repair cost previews when the clicked item qualifies.

431 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TickPartyAilmentIconBar

Named `sub_1A085` -> `TickPartyAilmentIconBar`, moderate confidence:
called from `RunDungeonGameLoop` and `ApplyMapTriggerEffect`. A
mechanically-clear but narratively-open sibling of `TickWorldAilments`:
periodically recomputes each roster member's ailment severity (via
`PrepareTrapEffectSlots` + helpers checking `+0x1C` status bits, or the
derived stat `+0x58` on a much slower path) and refreshes a per-member
icon bar via `ApplyEffectAndDrawIconBar`. Didn't force names onto the
lower-level helpers (`sub_1A14D`/`sub_1A195`/`sub_1A233`) or pin down
which specific ailments effect ids `2`/`0xE` represent — flagged as
open in file-formats.md rather than guessed.

432 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SelectClickedRosterPortrait

Named `sub_1930E` -> `SelectClickedRosterPortrait`: hit-tests region
table `0x6304` for one of 4 portrait slots (each gated on a
`word_328C6` visibility bit), sets the draw position, and resolves the
clicked slot's character via `g_partySlotAssignment` + `
SelectPartyRecordById`. Confirms `g_partySlotAssignment`'s exact base
address (`0x95EB`) and 4-slot layout, previously only described
generically. Called from the still-unnamed, large `sub_1869D`.

433 named of 769 functions as of this update.

### 2026-09-15 session update, continued: EMS screen-cache cluster (5 functions)

The naming-candidate ranker surfaced a cluster of 4 remaining
`MapUnmapPages`/EMS-page-`0x55D8` functions, all following the
established `RestoreXFromEMS`/`SaveXToEMS` shape:

- `RestoreFullScreenFromEMS` (was `sub_223D4`, called from
  `HandleMovementInput`): full-screen restore.
- `RestoreLargePanelFromEMS` (was `sub_191FC`, called from the
  still-unnamed `sub_1869D`): a large-but-not-full-screen area restore.
- `ClearPortraitPanelAreas` (was `sub_1B47A`, called from
  `RunShopScreen` and `RefreshPartyPortraits`): the odd one out — it
  *blanks* (fixed fill pattern) the same region in both the EMS cache
  and the live video buffer, rather than restoring. Page `0x55D8` is
  elsewhere documented as portrait-sized, and one caller is literally
  `RefreshPartyPortraits`, so this is plausibly resetting the portrait
  panel's cache to avoid a later restore showing stale data.
- `RestoreAndRedrawFixedStatusIcon` (was `sub_22402`, 6 call sites incl.
  `start`) + its helper `DrawFixedStatusIcon` (was `sub_225F1`):
  restores a small area then redraws a fixed picture (category `0x60`,
  the same directory `DrawPartyMemberPortrait` uses) at a fixed
  position — some HUD icon, exact identity not confirmed.

438 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CheckPartyWipeAndReinitLevel, ShowPartyWipeScreen

A significant find: named `sub_25AAC` -> `CheckPartyWipeAndReinitLevel`
and `sub_2ADE8` -> `ShowPartyWipeScreen`. `CheckPartyWipeAndReinitLevel`
(called from `RunDungeonGameLoop` and `ApplyEffectAndDrawIconBar`)
scans all 4 roster members and returns immediately if any one of them
is *not* flagged by one of `+0x1C` bits `6`/`10`/`11`/`12` (bits 10/11
being the confirmed timed-ailment flags). Only when every slot is
empty or flagged does it fall through to `ShowPartyWipeScreen` (stop
music, play a sound effect, full-screen picture, redraw the status
icon), then `RunGameDialog`, then `InitializeDungeonLevel` — i.e. a
"whole party incapacitated" handler that resets the level. Bit `6`'s
specific ailment and the `byte_2E400==0xFF` skip condition aren't
confirmed, but the overall "total party wipe" shape is well-evidenced.

440 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandleClueCategorySelection, RestoreClueBookBackgroundFromEMS

Named `sub_14D26` -> `HandleClueCategorySelection`: `RunClueEntryMenu`'s
category-switching input handler (keyboard + mouse, both funneling into
a shared "apply new category" block). Caught and fixed my own comment
before it was ever committed: initially guessed its `K`/`P` hotkeys'
gating bits (`word_328CC` `0x40`/`0x20`) were "plausibly the same
registration-lock bits `ShowClueBookRegistrationNag` checks" — wrong;
`DrawClueBookNavBar`'s own pre-existing comment already documents those
exact bits as the "d) LIST"/"c) MAP" hotkey-hint toggles, a completely
different flag from the actual registration lock (`word_328CA` bit 1 +
an entry's own `+2` bit `0x8000`).

Named `sub_14DFC` -> `RestoreClueBookBackgroundFromEMS`: a full-screen
EMS restore on its own dedicated page (`0x5616`), called only from
`ShowClueBook` to restore the screen behind it on close.

442 named of 769 functions as of this update.

### 2026-09-15 session update, continued: BuildItemDisplayName

Named `sub_14B24` -> `BuildItemDisplayName`: the item-detail sibling of
`BuildMonsterDisplayName`, called from `BuildClueEntryText` and
`ShowClueBookItemDetail`. Loads an item catalog record via
`LoadItemCatalogRecord` and joins 3 of its text fields (`+0x13`,
`+0x20`, `+0x2D`) with the same single-space separator
`BuildMonsterDisplayName` uses. Per-field semantics not confirmed.

443 named of 769 functions as of this update.

### 2026-09-15 session update, continued: FindItemInsideContainerLevel2/Level3

Named `sub_1CF50` -> `FindItemInsideContainerLevel2` and `sub_1CFC8` ->
`FindItemInsideContainerLevel3`: `FindItemInInventoryRange`'s container
recursion (via `FindItemInsideContainer`) turns out to be a fixed
3-level-deep chain, each level structurally identical (load a
container's 8 slots, scan for an item id in range, recurse one level
deeper on a flagged nested-container item) against different fixed
scratch-buffer offsets, confirmed terminal at level 3 (no further
recursion there). `FindItemInsideContainer`'s own pre-existing comment
already flagged the recursion into `sub_1CF50` by address, making this
an easy, high-confidence naming pass.

445 named of 769 functions as of this update.

### 2026-09-15 session update, continued: EditTextField

Named `sub_1D1D4` -> `EditTextField`, high confidence: a generic
single-line text input editor (buffer + max length), confirmed by one
of its 6 call sites falling inside `EditCharacterName`'s own address
range. Handles Enter (confirm)/Backspace (delete, or beep at empty)/
Escape (cancel)/printable characters (append, or beep at max length)
via `PollKeyboardInput`. Reused across 6 different text-entry screens.

446 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PromptBuyOreQuantity, PromptForBCD4Quantity

Named `sub_1AF49` -> `PromptBuyOreQuantity` and `sub_19BE6` ->
`PromptForBCD4Quantity`, found via a distinctive message cluster ("ORE
COSTS 10 GOLD PER UNIT.", "ENTER QUANTITY TO BUY", "GOLD COINS:").
`PromptBuyOreQuantity` (called from `UseItem`) is the purchase flow for
an Ore-type inventory item: shows the price/balance, reads a quantity
via `PromptForBCD4Quantity` (parses a digit string typed through
`EditTextField` into packed-BCD4), then validates affordability via
`CompareBCD4`. Plausibly connected to the already-documented but still-
unnamed `sub_1BBED`'s quantity-loop purchase path — not confirmed.

448 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawPartyStatusIconRow, DrawPartyStatusIcon — 450 named milestone

Named `sub_26C9E` -> `DrawPartyStatusIconRow` and `sub_26CFB` ->
`DrawPartyStatusIcon` (called from `HandleDungeonInput`): a third,
compact party display alongside the portrait panel and the fuller
combat status panel — a 4-icon row for the `g_partySlotAssignment`
roster, each icon drawn from `+0x12` (the same field
`DrawPartyRosterEntry` uses) with an incapacitation overlay
(cross-confirming `CheckPartyWipeAndReinitLevel`'s `+0x1C` bits
`0x1C40`) and a selection highlight. Also surfaced a new,
not-yet-documented party record flag, `+0x15E` bit `0x8000`.

450 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandleSpecialCellEntry, DrawMovementFeedbackIcon

Named `sub_116F3` -> `HandleSpecialCellEntry` and `sub_116CF` ->
`DrawMovementFeedbackIcon`, both called from `HandleMovementInput`,
moderate confidence. `HandleSpecialCellEntry` fires when the player's
destination cell type falls in the same `[_val32,_val31]` range
`IsMonsterStepBlocked` also checks — a `byte_2E400`=='H' branch pulls
`0xE551` tile-type table entries into scratch and refreshes the
dungeon screen, otherwise just a sound; both fall through to normal
movement. Neither the special cell type's identity nor 'H' 's meaning
(not one of the manual's documented hotkeys) are confirmed — flagged
as open rather than guessed. `DrawMovementFeedbackIcon` is a smaller,
similarly-uncertain icon draw used in a few `HandleMovementInput`
branches including the out-of-bounds case.

452 named of 769 functions as of this update.

### 2026-09-15 session update, continued: MarkIneligiblePartyMembers

Named `sub_2D7A7` -> `MarkIneligiblePartyMembers` (called from
`InteractWithContainer`): closes the loop on the `+0x15E` bit `0x8000`
flag found via `DrawPartyStatusIcon` earlier this round — this is the
function that *sets* it, for party members who fail an eligibility
check (`sub_27A66`, not traced) plus a status-flag/level test, before
forcing a status-panel redraw. Reads as flagging members who don't
qualify for whatever's in the interacted container, but the specific
restriction isn't confirmed.

453 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawEligibleItemList, CheckItemEligibilityAndCopyName

Named `sub_1B8EE` -> `DrawEligibleItemList` and `sub_1B818` ->
`CheckItemEligibilityAndCopyName` (called from `UseItem` and
`FinishItemUse`): iterates the item catalog and draws a 2-column x
5-row list of entries matching a category-flag filter plus 6
prerequisite flag ids checked via the already-named `TestGlobalFlag`,
recording each match's catalog index in a small result buffer for
later selection.

455 named of 769 functions as of this update.

### 2026-09-15 session update, continued: FinishPlacingHeldItem

Named `sub_2BA62` -> `FinishPlacingHeldItem`, moderate confidence:
called from `sub_271DC` and the still-untraced, container-related
`sub_2621C`. Clears the held-item cursor after loading the held item's
catalog record and OR-ing a value from one of its flag bytes into
`word_36C81` (not otherwise documented) — a related but distinct
action from the already-named drop-to-ground flow
(`TryDropHeldItem`/`PlaceItemOnGround`), plausibly a container/
inventory-slot placement instead, but not confirmed.

456 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplySavingThrowEffect

Named `sub_28BD2` -> `ApplySavingThrowEffect` (called from
`UseAbilityCommand` and `sub_2A788`): gated by `FailsSavingThrow`, on a
failed save applies a trap/ability effect — id `< 50` to the current
character only, id `>= 50` applies `(id-50)` to every non-incapacitated
party member, revealing that effect ids `50+` are the party-wide/area
variant of the id 50 lower. Also a useful cross-reference: it passes
the current character's `+0x6C` field (previously only tied to
`ShowLockStatus`'s lockpicking-detail gate) to `FailsSavingThrow` as
the save's resistance bonus — fitting a general perception/awareness
stat better than lockpicking specifically, so I updated that note in
file-formats.md rather than treating the two uses as unrelated.

457 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawAnimationFrameAndAdvance, WriteContainerSubBlock

Named `sub_2D3FE` -> `DrawAnimationFrameAndAdvance` (a small, generic
animation-frame-cycler called from the still-unnamed combat dispatcher
`sub_2C0FE`) and `sub_2776F` -> `WriteContainerSubBlock` (a low-level
`FileEntry_Write` helper called 3 times from an unnamed caller for 3
party-record sub-blocks whose identity isn't confirmed). Both are
narrow, self-contained, well-evidenced helpers named without forcing
names on their larger, still-ambiguous callers.

459 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SwapHeldItemWithSlot

Named `sub_268A0` -> `SwapHeldItemWithSlot`, moderate-high confidence:
called from `sub_2621C` (the still-untraced, 230-line container-
interaction input handler — now confirmed to host at least two named
item actions, this and `FinishPlacingHeldItem`). A classic drag-and-
drop swap via a save/restore dance around the held-item triple across
two unnamed calls (pick up the slot's item; place the original held
item into the slot).

460 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PlaceHeldItemIntoEmptySlot

Named `sub_2687B` -> `PlaceHeldItemIntoEmptySlot`, the simpler sibling
of `SwapHeldItemWithSlot` (no pickup step, since the target slot is
already empty), also called from `sub_2621C`. Its own `sub_266D4`
resists a confident name (large, multi-branch), but investigating it
revealed it touches the same 3 sub-block offsets `WriteContainerSubBlock`
writes, plus a running total at `+0x118` plausibly tracking carried
weight — a useful cross-reference added to that earlier finding rather
than a new name forced onto an under-evidenced function.

461 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PickUpItemFromSlot, PlaceItemInSlot, PickUpHeldItemFromSlot

Named 3 more functions in the item-slot cluster: `sub_26B4F` ->
`PickUpItemFromSlot` and `sub_266D4` -> `PlaceItemInSlot`, confirmed as
exact mirror images (subtract vs. add the same item value across the
same 3 equipment-section running totals plus a general total) —
resolving the "resists a confident name" caveat from the previous
round now that the symmetry makes the mechanical shape unambiguous.
Also named `sub_26864` -> `PickUpHeldItemFromSlot`, the simple
"pick up only" counterpart to `PlaceHeldItemIntoEmptySlot`. This
completes the 4-action item-slot cluster called from `sub_2621C`.

464 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CommitContainerWrite

Named `sub_26C0E` -> `CommitContainerWrite` (called from
`LoadNextContainerInChain` and `sub_2621C`): a minimal write-commit
step, distinguished from the similarly-shaped `SyncContainerContents`
by doing no descriptor setup of its own.

465 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawThreeStatBars — resolves the "third stat bar" mystery

A major cross-confirming find. Named `sub_25F10` -> `DrawThreeStatBars`,
`sub_25E5E` -> `FormatAndDrawFraction`, and `sub_25ED1` ->
`DrawCharacterNameHeader`. Dumping `DrawThreeStatBars`' message strings
gave exact labels — "HEALTH:", "MAGIC:", "WEIGHT:" — which:

1. **Resolves the long-flagged "third stat bar" in
   `DrawPartyMemberStatusPanel`** (`+0x118`/`+0x56`, "not identified"
   since early in the session): it's carried weight vs. max carry
   capacity.
2. **Reconciles with `GetInventorySlotPtr`'s "group base + 2" formula**
   from several rounds ago: the 2 bytes it skips at each inventory
   group's base (`+0x118` main, or `+0x180`/`+0x1A6`/`+0x1CC` for the 3
   alternate bags) aren't padding — they're a running weight-total
   counter, exactly matching what `PickUpItemFromSlot`/`PlaceItemInSlot`
   (named a couple rounds ago) add/subtract from.
3. **Confirms `+0x1C` bit `0x40`** (part of `CheckPartyWipeAndReinitLevel`'s
   `0x1C40` incapacitation mask) **as the "DEAD" flag** — `DrawThreeStatBars`
   shows "DEAD" instead of the HEALTH fraction when it's set.

This ties together five separate findings from across the session into
one coherent picture of the inventory/weight/incapacitation system.

468 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawAfflictionsList, DrawCharacterProtectionsList — the session's biggest cross-confirmation

The single biggest payoff of this whole naming pass. Named
`sub_25D82` -> `DrawAfflictionsList`: it draws "AFFLICTIONS:" and
tests every individual bit of the party record's `+0x1C` status word,
drawing the exact name for each. Dumping its 10 message strings gave
the **complete, definitive map** of all 9 affliction bits plus "NONE":
`0x2000`=DISEASED, `0x4000`=POISONED, `0x8000`=SICK, `0x400`=STONED,
`0x800`=FROZEN, `0x1000`=PARALYZED, `0x80`=CURSED, `0x100`=HEXED,
`0x200`=JINXED.

This single function resolves five previously-separate, partially-
guessed findings from across the session in one shot:
- `TickStatusEffects`/`ApplyStatusEffect`'s `0x400`/`0x800`/`0x1000`
  group = STONED/FROZEN/PARALYZED (the timed ailments).
- `CastSpell`'s `0x18` dispel bits (`0x2000`/`0x4000`/`0x8000`) =
  DISEASED/POISONED/SICK (the dispellable-only group).
- `CheckPartyWipeAndReinitLevel`'s `0x1C40` mask = DEAD (bit 6, itself
  confirmed via `DrawThreeStatBars` last round) + STONED + FROZEN +
  PARALYZED — a coherent "can't act" set.
- `TickPartyAilmentIconBar`'s two effect-id groups = DISEASED/
  POISONED/SICK (id 2, everyone) vs. CURSED/HEXED/JINXED (id 0xE,
  MP-gated).
- `DrawCharacterProtectionsList` (also named this round, was
  `sub_25FCD`) draws "PROTECTIONS:" pairing the already-found `+0x20`-
  `+0x30` resistance values with these exact same 9 names (DISEASE/
  POISON/SICKNESS/STONING/FROZEN/PARALYZE/CURSING/HEXING/JINXING),
  resolving that section's long-standing "not confirmed" guess too.

Also named `sub_25CFA` -> `DrawAbilityReadinessList` (draws a
character's learned special abilities, color-coded by whether their
charge/time-of-day requirements are currently met).

471 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawCharacterStatSheet

Named `sub_24D30` -> `DrawCharacterStatSheet` (the character sheet's
main stat renderer, called from `ShowCharacterSkills` and
`sub_23C18`), plus two small helpers it uses: `sub_250BB` ->
`DrawTrimmedThresholdValue` and `sub_256F0` -> `StripSpaces`. This
function draws the 6 core attributes and the still-mysterious
`+0x4C`/`+0x4E`/`+0x50` trio in one screen column (confirming they sit
in the same visual list, slots 7-9, without resolving what they are),
and a 13-entry derived-stat column (`+0x58`-`+0x70`) in the other,
where the last 5 entries highlight when this character holds one of 5
globally-assigned party roles (`word_36D03`/`05`/`07`/`09`/`0B`) — a
solid new lead for eventually naming those fields (navigator, mapper,
barterer, etc., per the attribute/skill string survey from early in
the session).

474 named of 769 functions as of this update.

### 2026-09-15 session update, continued: FormatAndDrawAlchemyFraction

Named `sub_1E2E5` -> `FormatAndDrawAlchemyFraction`, called only from
`DrawAlchemyStatusPanel`: a near-duplicate of the already-named
`FormatAndDrawFraction`, for the alchemy screen's MAGIC: bar.

475 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawStringColumn, WriteStringWithHighlightedChar

Named `sub_23B76` -> `DrawStringColumn` (draws N stacked
null-terminated strings, used this session for the PROTECTIONS/
AFFLICTIONS name lists) and `sub_23AF2` -> `WriteStringWithHighlightedChar`
(draws a string with exactly one character in a highlight color,
called 3 times from `ShowCharacterSkills`'s 15-skill-line setup —
already noted in an earlier round as "only draws label strings").

477 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawShadowedText, DrawShadowedTextAlt

Named `sub_161D0` -> `DrawShadowedText` and its byte-for-byte identical
duplicate `sub_11E4A` -> `DrawShadowedTextAlt` (likely duplicated
across overlay segments): a drop-shadow text/list draw effect, called
from character-creation step 3 (`sub_1559A`) and a still-untraced
function reached from `start` (`sub_11A10`) respectively.

479 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawQuitOrReturnLabel

Named `sub_25595` -> `DrawQuitOrReturnLabel` (called from
`ShowCharacterSkills` and `ShowCharacterInventory`): draws a bottom-left
exit button toggling between `QUIT "CREATE"` and `RETURN` based on
`word_328CA` bit `0x8000` — these screens are shared between viewing an
existing character and mid-chargen review.

480 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ground-item slot I/O cluster

Named `sub_1A34C` -> `ReadGroundItemSlot`, `sub_1A320` ->
`PrepareGroundItemSlotWrite`, and `sub_1A36A` -> `CommitGroundItemWrite`
— `PlaceItemOnGround`'s low-level slot read/write cycle, the ground-item
counterpart to the already-named `CommitContainerWrite`.

483 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RestoreAllPortraitsFromEMS

Named `sub_18F6C` -> `RestoreAllPortraitsFromEMS`, called from the
still-untraced `sub_1869D`: a simpler sibling of `RefreshPartyPortraits`
that restores all 4 portrait slots and clears their dirty bits without
drawing the shop-hint text.

484 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ConsumeAbilityChargeAndRefresh

Named `sub_17A65` -> `ConsumeAbilityChargeAndRefresh`, called from
`UseAbilityCommand`: shows `ShowResourceDepletedOverlay`, then
increments a plausible charge/uses counter and refreshes the dungeon
screen. Part of the same "use special ability" flow already tied
together via `ApplySavingThrowEffect`'s `word_32DC0`/`word_32DC2`
parameters.

485 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ResetWeaponSlotDisplayCache

Named `sub_1DC73` -> `ResetWeaponSlotDisplayCache`, called from
`HandleRangedOrCombatAction`: reuses the same 4 weapon-select-slot
globals and x-positions `DrawWeaponSelectIcon` draws, to clear a
dedicated EMS page region at the first empty slot's position — every
Nth call via a countdown. Plausibly a per-slot display/animation
cache reset.

486 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawAfflictionIconRow — independent re-confirmation

Named `sub_22615` -> `DrawAfflictionIconRow`, called from
`DrawPartyMemberStatusPanel`: an icon-based counterpart to last round's
`DrawAfflictionsList`, testing the exact same `+0x1C` affliction bit
groups (DISEASED/POISONED/SICK, STONED/FROZEN/PARALYZED, CURSED/HEXED/
JINXED) plus the 9 protection values (`+0x20`-`+0x30`) — independently
re-confirming both mappings from a completely different function found
by chance while ranking candidates.

487 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ListCompatibleClueBookItems

Named `sub_1472A` -> `ListCompatibleClueBookItems`, called from
`RunClueBookItemCategory` and `RunClueBookWeaponCategory`: checks the
current item's usability flags and, if eligible, scans up to 9 more
catalog ids drawing each compatible match — a filtered items list for
the F8 clue book's weapon/armor category view, using the same `0x6976`
region table `DrawSubIconSelectorRow` already ties to this cluster.

488 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandlePagedEntryNavigation

Named `sub_133EB` -> `HandlePagedEntryNavigation`, called from
`ShowPagedEntryScreen`: `I`/previous-page and `Q`/next-page navigation
(keyboard or mouse via region table `0x6960`), the latter gated past
page 5 by the same registration check `ShowClueBookRegistrationNag`
guards elsewhere in the clue book.

489 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ResolveAttackAndLatchFirstHit

Named `sub_2D195` -> `ResolveAttackAndLatchFirstHit`, called once from
the still-unnamed combat dispatcher `sub_2C0FE`: calls `ResolveAttack`
then latches a value into `word_2E49C` the first time through. Exact
field identities not confirmed — named for the clear mechanical shape
only, without forcing a name onto `sub_2C0FE` itself.

490 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PreloadMonsterStatsTable

Named `sub_124EC` -> `PreloadMonsterStatsTable`, called once from
`InitGame`: allocates a large one-time buffer and reads into it with
the same `errorCode=9` `LoadClueBookMonsterEntry` uses for WORLD.DAT's
MONSTER STATISTICS block, plausibly preloading the whole table at
startup rather than per-entry.

491 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PreloadWorldDataTable

Named `sub_12449` -> `PreloadWorldDataTable`, called from `InitGame`
immediately before `PreloadMonsterStatsTable`: an identically-shaped
sibling (allocate a large one-time buffer, read with `errorCode=9`)
but via a different resource-setup stub, so a different `WORLD.DAT`
block — not confirmed which one.

492 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ConfirmContainerInteraction

Named `sub_2D809` -> `ConfirmContainerInteraction`, called from
`InteractWithContainer`: shows a yes/no confirm prompt, storing the
result and the current slot selection for the caller to act on.

493 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ClearIneligibleFlagForAllMembers

Named `sub_2D7EA` -> `ClearIneligibleFlagForAllMembers`, called from
`InteractWithContainer`: the exact inverse of `MarkIneligiblePartyMembers`,
unconditionally clearing the `+0x15E` "needs attention" bit for all 4
party slots.

494 named of 769 functions as of this update.

### 2026-09-15 session update, continued: LoadClueBookSpellEntry

Named `sub_1D198` -> `LoadClueBookSpellEntry`, called from
`BuildClueEntryText` and `RunClueBookSpellCategory`: reads an 80-byte
spell record from its own dedicated EMS page, the spell-data
equivalent of `LoadClueBookMonsterEntry`'s `WORLD.DAT` read.

495 named of 769 functions as of this update.

### 2026-09-15 session update, continued: BuildAlchemySpellList, CheckSpellCastability

Named `sub_1E1A7` -> `BuildAlchemySpellList` and `sub_1E285` ->
`CheckSpellCastability`, both called from `RunAlchemyScreen`: builds
the alchemy screen's filtered known-spell list, then checks each
spell's affordability (MP, MAGIC ORE, NUORE) via
`LoadClueBookSpellEntry`'s cost data, marking castable ones.

497 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawAlchemySpellList, DrawSpellCostValue

Named `sub_1E3AF` -> `DrawAlchemySpellList` and `sub_1E340` ->
`DrawSpellCostValue`, both called from `RunAlchemyScreen`: the visual
counterpart to `BuildAlchemySpellList`, drawing each page's spell rows
and cost values.

499 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CompactPartyRosterSlots — 500/769 milestone

Named `sub_2BF3C` -> `CompactPartyRosterSlots`, called from
`ShowWorldMap`'s exit path (`D` key or equivalent mouse click) as a
cleanup-on-exit step: cascades non-empty roster entries down to fill
gaps across the 4 active `g_partySlotAssignment` slots *and* 3 more
"reserve" globals (`word_36E4D`/`word_36E4F`/`word_36E51`) not
previously documented — confirming the party roster extends beyond the
4 active members into at least 3 reserve slots, a genuinely new
structural finding.

**500 named of 769 functions as of this update** — roughly 65% of the
executable now named, up from 419 at the start of this session
(+81 functions this session).

### 2026-09-15 session update, continued: RedrawAllPartyStatusPanels

Named `sub_2ADD0` -> `RedrawAllPartyStatusPanels`, called from
`ApplyMultiStatEffect`, `RestCharacter`, and others: a batch helper
calling `DrawPartyMemberStatusPanel` for every occupied roster slot.

501 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SyncAlternateBagsToSave

Named `sub_2772C` -> `SyncAlternateBagsToSave`, called from
`sub_274B4`: writes each of the 3 alternate-bag inventory groups back
to `CURGAME` via `WriteContainerSubBlock`, only when populated —
resolving the earlier "identity of those 3 sub-blocks isn't confirmed"
note by tying them to the already-established `GetInventorySlotPtr`
group-base fields.

502 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandleStatusIconBarClick

Named `sub_270FE` -> `HandleStatusIconBarClick`, called from `start`
and `HandleDungeonInput`: hit-tests the same region table (`0x636C`)
`TryCureAilmentFromIconClick` uses, revealing the confirmed 6-slot
ailment table at `0x9519` is likely the tail of a larger 9-slot array
starting at `0x950D` — hits 1-3 (the first 3 slots) dispatch elsewhere
(`sub_271DC`, not traced) instead of the ailment-cure logic, plausibly
a different icon type sharing the same bar. Flagged as an open lead
rather than guessed further.

503 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PickRandomActivePartyMember

Named `sub_22A35` -> `PickRandomActivePartyMember`, called from
`sub_22989`: retry-picks a random party slot until it lands on one
that's occupied and not incapacitated (the confirmed `0x1C40` mask) —
a classic "pick a valid random target" utility.

504 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyRestEffectsToCharacter

Named `sub_1E943` -> `ApplyRestEffectsToCharacter`, called from
`RestPartyAndAdvanceClock`'s hourly tick loop: skips the incapacitated;
drains HP or MP instead of regenerating it when DISEASED or CURSED
(DISEASED HP loss reaching 0 sets the DEAD flag), otherwise applies
normal percentage-based regen — a nice cross-confirmation that resting
isn't purely beneficial when the party is afflicted.

505 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ConfirmAlchemyInteraction

Named `sub_1E4FA` -> `ConfirmAlchemyInteraction`, called from
`RunAlchemyScreen`: byte-for-byte identical to `ConfirmContainerInteraction`,
another instance of the overlay-segment duplication pattern found
earlier this session (`DrawShadowedText`/`DrawShadowedTextAlt`).

506 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ReadTypedInteger

Named `sub_1D146` -> `ReadTypedInteger`, called 7 times incl. from the
map editor's `EditWallLegendTypeNumber`/`EditFloorLegendTypeNumber`: a
generic typed-integer prompt built on `EditTextField`.

507 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ClassifyItemServiceTier

Named `sub_1AE9D` -> `ClassifyItemServiceTier`, called 6 times from
two other unnamed functions: loads an item and returns one of 3 tier
codes (or a 4th "wrong item type") based on its flags — plausibly
gating which service an item qualifies for, alongside the shop
cluster's `IsItemEligibleForEnhance`/`IsItemEligibleForRepair`, but not
confirmed.

508 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyItemEffectIconSlot

Named `sub_1AE4C` -> `ApplyItemEffectIconSlot`, called from `sub_1ACD7`
(the same caller as `ClassifyItemServiceTier`): populates an icon-bar
slot for a new effect id (`0`) tied to the current item, using the
same slot layout `TickPartyAilmentIconBar`/`ApplySavingThrowEffect`
established, then draws it via `ApplyEffectAndDrawIconBar`.

509 named of 769 functions as of this update.

### 2026-09-15 session update, continued: GetClassifiedItemStatField

Named `sub_1AE23` -> `GetClassifiedItemStatField`, called from
`sub_16BF6`: uses `ClassifyItemServiceTier` and selects one of two
`word_2E548` sub-fields based on the item's category flag, or 0 on
classification failure.

510 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyTriggerEffectIconSlot

Named `sub_1AC80` -> `ApplyTriggerEffectIconSlot`, called from
`ApplyMapTriggerEffect`: the map-trigger counterpart to
`ApplyItemEffectIconSlot` — a map trap reaching the same icon-bar-slot
effect machinery instead of direct item use.

511 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RemoveMultiStatEffect

Named `sub_1AC2F` -> `RemoveMultiStatEffect`, called from
`PickUpItemFromSlot`: the removal counterpart to the already-named
`ApplyMultiStatEffect` — reverses a magic item's stat bonuses when it's
taken off, confirmed by mirroring that function's own finish sequence
(`sub_1AA9B` + `UpdatePartyAverageStatTiers`).

512 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandleIconBarItemExpiry

A significant find: named `sub_1819B` -> `HandleIconBarItemExpiry`,
called from `ApplyEffectAndDrawIconBar`. When an icon-bar item's timed
effect expires, strips its stat bonuses via `RemoveMultiStatEffect`,
then either replaces the inventory slot with a new item (applying that
item's own effect) or destroys it outright (clearing the slot,
subtracting its weight) — the mechanism behind consumable magic items
that transform or run out, though the specific items involved aren't
identified.

513 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyMultiStatEffectForItem

Named `sub_1AA06` -> `ApplyMultiStatEffectForItem`, called 6 times
incl. from `HandleIconBarItemExpiry`: the exact ADD-side mirror of
`RemoveMultiStatEffect`, distinct from the higher-level command handler
`ApplyMultiStatEffect` (same core table-walking logic, but without
target confirmation/incapacitation checks/full redraw).

514 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SwapItemMultiStatEffect

Named `sub_276C5` -> `SwapItemMultiStatEffect`, called 3 times from
`sub_274B4`: ties together several findings from this round —
removes the current item's effect and swaps in a new item id from the
exact `word_2E548` fields `GetClassifiedItemStatField` selects between,
applying the new item's effect. Replaces one equipped item's stat
effect with a different item's, per category.

515 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CheckAndTickAvailableAilment

Named `sub_1A5A6` -> `CheckAndTickAvailableAilment`, called 3 times
from `sub_1A582`: matches `TickStatusEffects`' own pre-existing comment
noting this as one of its two call sites (the other being
`HandleGameCommand` directly) — loops calling `IsItemRangeAvailable`
and ticks the status effect whenever an item turns up in range.

516 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TryLoadNextContainerLink

Named `sub_18FC5` -> `TryLoadNextContainerLink`, called 9 times from
`sub_18C79`: a small guard wrapping `LoadNextContainerInChain`, only
calling it when the item is itself a container and a caller-supplied
flag allows it.

517 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyIconBarStatDelta

Named `sub_182CE` -> `ApplyIconBarStatDelta`, called from
`ApplyEffectAndDrawIconBar`: applies a capped/floored stat delta to a
data-selected party field, clears a status-bit range, then finishes
with `CheckForLevelUp` — suggesting a gradual/staged XP-granting use,
though the specific field isn't confirmed.

518 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SelectTrapEffectVariant

Named `sub_16DAA` -> `SelectTrapEffectVariant`, called twice from
unnamed `sub_16881`: picks between a record's primary trap effect id
(`[+0x6C]`) and an alternate (`[+0x6E]`). If flag `[+0xC]` bit `0x400`
is set, or there's no alternate, always uses the primary. Otherwise
rolls `RandomInRange(100)`: a 25% chance (roll `< 0x19`) swaps in the
alternate instead (and sets `word_328CA` bit `0x200`), else falls back
to the primary. Either way, resolves the chosen id via
`PrepareTrapEffectSlots`, staging the result into `word_3293E`/
`word_32940` — the same staging globals used elsewhere for the
icon-bar effect mechanism. `[+0x6C]` and `[+0x6E]` were already
identified separately (via `ShowLockStatus`/`ApplySavingThrowEffect`
and `ClassifyConversationSkillTier` respectively) as
lockpicking/perception- and charisma/persuasion-style fields — so this
reads as "usually trigger the trap's primary save-resistance effect,
but a 1-in-4 chance of a persuasion-flavored alternate instead," though
the caller `sub_16881` itself remains untraced and unnamed.

519 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SyncItemChargeFieldToCurgame

Named `sub_2778D` -> `SyncItemChargeFieldToCurgame`, called 3 times
from the still-unnamed `sub_274B4` (the large item-use/consumption
dispatcher already known to host `SwapItemMultiStatEffect` and
`SyncAlternateBagsToSave`). Before each call, the caller configures a
resource-stub helper pointing a heavily-reused shared scratch buffer
(`0xAFA8` — also used elsewhere for conversation-topic text) at
`CURGAME` (`FileEntry` `bx=0x8FFB`), then this function reads that
record (`errorCode=0xA`), applies the *exact same* category-dependent
charge/transfer/swap logic the caller applies to its own in-memory
copy (`word_328C8` bits `0x8000`/`0x4000`/`0x2000`: transfer a
secondary count into primary, decrement a shared charge counter with
depletion handling, or call `SwapItemMultiStatEffect`) to the field at
`[0xAFA8+dx]`, then writes the record back — except for the "transfer"
category with `dx==0`, where it instead subtracts the staged amount
(`word_3293E`) from generic scratch variable `word_38808` (previously
documented as reused for unrelated purposes elsewhere, not a specific
resource pool) and skips the write. In short: a CURGAME-persisting
mirror of the caller's in-memory item-charge logic. The exact identity
of the 3 fields it's called with (`word_3296C`/`word_32972`/
`word_3296E`) isn't confirmed, and `sub_274B4` itself remains untraced.

520 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TryResolveAttackAgainstTarget

Named `sub_2D171` -> `TryResolveAttackAgainstTarget`, called once from
unnamed `sub_2D4B6` (part of the large unnamed combat dispatcher
`sub_2C0FE`'s tree, alongside the previously-named
`ResolveAttackAndLatchFirstHit`). Skips the attack if `word_33306` bit
`0x100` is set and a per-target field (`[di+0x4E]`) already equals
global `word_332D8` (plausibly "already resolved this round/pass"),
otherwise calls `ResolveAttack(ax=[di+0x58], bx=[si+0x62],
cx=word_332E8)` with `si=word_328D4` (current/selected party member).
Deliberately did **not** identify `[di+0x4E]`/`[di+0x58]` with the
similarly-offset, already-documented party-record fields (the
still-open `+0x4C`/`+0x4E`/`+0x50` trio and the monster-detail-reveal
derived stat `+0x58`) since `di`'s record type at this call site isn't
confirmed — it could be a monster or a different combat-scratch
struct entirely, not necessarily the party-record layout. Also traced
`word_332D8` a bit further: besides this guard, it's compared against
small constants (9, `0xD`) elsewhere and has an odd
`ShowClueBookSpellDetail+0x1F6` read xref that looks like an IDA
tail-chunk attribution artifact rather than genuine logical overlap —
plausibly a reused scratch value like `word_38808`, not confirmed
single-purpose.

521 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyTargetResistancesToAttack

Named `sub_2D1C2` -> `ApplyTargetResistancesToAttack`, called once
from unnamed `sub_2D4B6` right after `TryResolveAttackAgainstTarget` —
the resistance/immunity-filtering half of the same damage-application
sequence. Bundles four distinct behaviors, each gated by comparing the
attack's flag words (`word_33304`/`word_33306`) against matching bits
on the target record (`[di+0x96]`/`[di+0x98]`): filters 6 "high"
status-effect bits by target immunity into the accumulated to-apply
flags (`word_2E49A`); fully negates the staged damage
(`word_2E49C`=0) on a match against 5 "low" bits; halves the staged
damage on a match against 7 resistance-category bits; and drains a
fixed amount (`word_332E0`) from one of 5 elemental resource fields on
the target (offset selected by a 5-way priority-encoded bit test),
floored at 0. As with the sibling function, deliberately did not
identify the elemental-resource field offsets (`0x10`/`0x54`/`0x56`/
`0x58`/`0x5A`) with the similarly-numbered party-record fields
documented elsewhere, since `di`'s record type here remains
unconfirmed.

522 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyAttackToTarget

Named `sub_2D4B6` -> `ApplyAttackToTarget`, called twice from the
large unnamed combat dispatcher `sub_2C0FE` — the orchestrator that
ties together this round's two new functions. Resolves base damage
(via `TryResolveAttackAgainstTarget`, or a direct
`word_332E8`/`[di+0x5A]/2` path), filters it through
`ApplyTargetResistancesToAttack`, bails if nothing survived, then
commits to the target: sets `[di+0xC] |= 3`, subtracts the final
damage from `[di+0x10]` (a HP-like current-value field), ORs any
surviving status flags into `[di+0xC]` (and conditionally `[di+0x96]`
too), overwrites `[di+0x1C]`/`[di+0x1E]` with fixed globals
`word_332EE`/`word_332FC`, and conditionally clears `[di+0xC]` bit 0.
With all three functions in this cluster now named, the shape of a
full "resolve → filter by resistance → commit" per-target attack
pipeline is solid, even though `di`'s exact record type (monster vs.
some other combat-scratch struct) is still unconfirmed. `sub_2C0FE`
itself, the dispatcher that calls this twice, remains untraced and
unnamed.

523 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyAttackAlongCorridorLine — resolves the `di` question

Named `sub_2D470` -> `ApplyAttackAlongCorridorLine`, called 3 times in
a row from unnamed `sub_2C0FE` (a sibling of the already-named
`ApplyDamageAlongCorridorLine`, but driving the full resistance-aware
pipeline). Loops 3 times over consecutive viewport rows starting at
`word_3292C` (incrementing it each iteration), calls
`GetMonsterAtViewportRow` to find a monster at each row, and — **this
resolves the open question from the last two rounds** —
`GetMonsterAtViewportRow`'s result (`si`) is moved directly into `di`
before calling `ApplyAttackToTarget(di)`. So `di` throughout the whole
`ApplyAttackToTarget`/`ApplyTargetResistancesToAttack`/
`TryResolveAttackAgainstTarget` cluster **is a monster record**, not a
party record — confirming the caution notes added for those three
functions were warranted, and that `[di+0x96]`/`[di+0x98]`/`[di+0x1C]`
etc. are monster-record fields distinct from the party record's
same-numbered fields. If any damage/status was left pending after the
attack, also calls a still-unnamed sibling, `sub_2D428` — a
similar-shaped commit sequence (a proper bit-by-bit compounding
resistance-halving loop, unlike `ApplyTargetResistancesToAttack`'s
single first-match halving) whose exact relationship to
`ApplyAttackToTarget`'s own commit isn't resolved, so left unnamed.

524 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawMinimapCompassIcon

Named `sub_2169C` -> `DrawMinimapCompassIcon`, called once from
`DrawMinimap`: draws the minimap's small facing/compass icon. Fixed
position (`0x110`,`0x20`), redirected to the offscreen buffer, reuses
the same fixed glyph `DrawMinimap` itself uses for its 7×9 grid
(`word_2E532=0x90`), and picks a 0–3 remap/variant value
(`word_2E530`) from the party's current facing via the same
`word_36CF5` tier-bit convention already documented across
`ShowCompassDirection`/`SpawnMonsterInFacingDirection`/
`DrawDungeonCellSideFeature` — a small graphical counterpart to
`ShowCompassDirection`'s text HUD readout.

525 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SaveClueBookBackgroundToEMS

Named `sub_150B8` -> `SaveClueBookBackgroundToEMS`, called once from
`ShowClueBook` before the already-named
`RestoreClueBookBackgroundFromEMS` is called later — its exact save
counterpart. Maps EMS page `0x5616` (the dedicated clue-book
full-screen page) and copies the entire real VGA screen (`0xA000`)
into it, the mirror image of the restore function's EMS-to-
`_videoBufferSeg` copy. Straightforward round-trip pair, quickly
resolved thanks to the restore function's existing comment.

526 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandleClueEntryScrollInput

Named `sub_12DD8` -> `HandleClueEntryScrollInput`, called once from
`RunClueEntryMenu` — a pagination handler in the same "I"/"Q" hotkey
family as the previously-named `HandlePagedEntryNavigation`, but for
the clue book's entry list, additionally driven by a mouse hit-test
(`HitTestRegionTable` against table `0x6960`). 'I'/hit-region-1 adopts
candidate index `word_2E3F0` into `word_2E3EE` (signaling change via
`errorCode=1`) unless `word_328CC` bit `0x100` defers entirely to
still-unnamed `sub_13014`; 'Q'/hit-region-2 mirrors this with
`word_2E3F2` (`errorCode=2`), deferring to still-unnamed `sub_12FED`
on `word_328CC` bit `0x80`. `errorCode=0` if nothing changed. The
exact identity of `word_2E3F0`/`word_2E3F2` (plausibly precomputed
clamped prev/next indices) and of the two deferred-to functions isn't
confirmed, so left open for a future round.

527 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ScrollClueEntryListPageUp / PageDown

Named the pair `sub_13014` -> `ScrollClueEntryListPageUp` and
`sub_12FED` -> `ScrollClueEntryListPageDown` — the two functions
`HandleClueEntryScrollInput` defers to in its "special mode" branches,
also shared with another unnamed caller (`sub_12D5C`). Both jump the
clue entry list's scroll offset (`word_2E3F0`) by a fixed page size of
`0x38` (56) — up clamped to a minimum of `word_2E3EA+2`, down clamped
against upper bound `word_2E3F2` — adjust `word_2E3EE` by the delta,
call still-unnamed `sub_12FC1` (plausibly a redraw), and set
`errorCode` (1/2) matching `HandleClueEntryScrollInput`'s own
convention. Resolves both of that function's previously-open
deferred-to callees in one round.

529 named of 769 functions as of this update.

### 2026-09-15 session update, continued: HandleClueEntryRowScrollInput + RecomputeClueEntryPageBounds

Named `sub_12D5C` -> `HandleClueEntryRowScrollInput` (called from
`RunClueEntryMenu`, a sibling to `HandleClueEntryScrollInput` handling
'H'/'P' keys for a single-row (step 4) scroll of the clue entry list,
paging via `ScrollClueEntryListPageUp`/`Down` only at the current
page's boundary) and `sub_12FC1` -> `RecomputeClueEntryPageBounds`
(called from those two page-scroll functions — **correction**: when
they were named last round it was speculated this might be "a
redraw"; tracing it directly shows it's actually pure bounds
arithmetic, no drawing at all). It recomputes the page's upper bound
(`word_2E3F2 = word_2E3F0+0x34`, clamped to total count `word_2E3F4`)
and visible-row count (`word_2E3EC`, defaulting to `0xE` or the
remaining-rows count on the last partial page) — the `/4` in that
remaining-rows math confirms a 4-entries-per-row grid layout for the
clue entry list, matching the step-by-4 stride in
`HandleClueEntryRowScrollInput`. This resolves the entire clue-entry-
list scrolling cluster (`HandleClueEntryScrollInput`,
`HandleClueEntryRowScrollInput`, `ScrollClueEntryListPageUp`/`Down`,
`RecomputeClueEntryPageBounds`) end to end.

531 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PollForEscapeKeyOnly / Alt

Named the byte-for-byte-identical pair `sub_11900` ->
`PollForEscapeKeyOnly` and `sub_15249` -> `PollForEscapeKeyOnlyAlt` —
another instance of this session's overlay-segment duplicate-function
pattern (alongside `DrawShadowedText`/`DrawShadowedTextAlt` and
`ConfirmContainerInteraction`/`ConfirmAlchemyInteraction`). Both poll
for a keypress but only care about ESC: any other key is silently
discarded (`byte_2E400` cleared to 0), and the function returns with
ZF set iff `byte_2E400==0x1B`. `PollForEscapeKeyOnly` is called 3x
from the already-named `ShowIntroPicture`; the `Alt` duplicate is
called 3x from still-unnamed `sub_15429`.

533 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PlayCharacterCreationIntroAnimation

Named `sub_15429` -> `PlayCharacterCreationIntroAnimation`, called
once from the very start of `RunCharacterCreation` (resolving the
"still-unnamed `sub_15429`" note from last round) — the character
creation screen's opening animated sequence. Decodes a 768-byte
simple-obfuscated (each byte `-0x3F`) graphics block, plays music
track `0x12`, then runs several staged sub-animations (frame loops of
63/5/20/10/10/20), each abortable via `PollForEscapeKeyOnlyAlt` and
each honoring a `word_328C4` bit `0x400` "fast/skip" check. The
various low-level draw helpers it drives (`sub_160D6`, `sub_152EF`,
`sub_16180`, `sub_1618E`, `sub_1619F`, `sub_160C3`, `sub_2589A`,
`sub_160B0`) are animation-frame primitives specific to this sequence,
not traced/named this round.

534 named of 769 functions as of this update.

### 2026-09-15 session update, continued: WaitFrameTicksOrEscape

Named `sub_119E0` -> `WaitFrameTicksOrEscape`, called once from
`ShowIntroPicture`: busy-waits for `word_328C4` bit `0x400` ("tick
ready", the same flag `PlayCharacterCreationIntroAnimation`'s staged
sub-animations check — plausibly raised by an untraced timer/vsync
interrupt handler), checks for ESC via `PollForEscapeKeyOnly`
(returns immediately if pressed), else clears the bit and repeats for
`cx` total ticks — a frame-paced wait-with-abort primitive.

**Correction to last round's `PlayCharacterCreationIntroAnimation`
entry**: while adding the note above, noticed `file-formats.md`
already documents `ShowIntroPicture` performing the *exact same*
`al=[si]; al-=0x3F; [di]=al` copy loop to build a palette
fade-interpolation buffer (`0x475A`, sourced from a `WORLD.DAT`
palette read). `PlayCharacterCreationIntroAnimation`'s own opening
step runs the identical loop (`0x442A` -> `0x4D5C`, 768 bytes) — so
its "decodes a simple-obfuscated graphics/data block" description was
wrong; it's almost certainly building the same kind of palette fade
buffer for its own animation, not decoding graphics. Fixed the IDA
comment (`ida_scripts/fix_char_creation_intro_anim_comment.py`) and
both docs entries in place.

535 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ReapplyDamageWithCompoundedResistance — resolving a previously-flagged open lead

Named `sub_2D428` -> `ReapplyDamageWithCompoundedResistance`, called
once from `ApplyAttackAlongCorridorLine` right after
`ApplyAttackToTarget` — the function left deliberately unnamed two
rounds ago pending clarification of its relationship to
`ApplyAttackToTarget`'s own commit. Traced it fully this round: since
`ApplyAttackToTarget` only ever leaves `word_2E49C`/`word_2E49A`
nonzero by having already run its own complete commit (every
early-return path re-checks both are 0 first), this function runs as
a genuine **second** commit pass on the same target. It re-filters
status flags by immunity (idempotent — bits stay set), but then
recomputes a *compounded* resistance halving — looping all 16 bit
positions and halving the staged damage once per matching bit among
the same 7 resistance-category bits `ApplyTargetResistancesToAttack`
already checked (which only applies a single first-match halving) —
and subtracts that newly-recomputed amount from `[di+0x10]` **again**,
genuinely double-applying damage on top of `ApplyAttackToTarget`'s own
subtraction. Named it based on this now-fully-traced mechanical
behavior, but honestly flagged that *why* the game deliberately
re-applies damage this way for corridor-line attacks specifically
(compounding elemental damage as an area-effect design choice, vs. an
artifact of the original code) is not resolved.

536 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the intro-animation wipe effect (3 functions)

Named a 3-function cluster used only by
`PlayCharacterCreationIntroAnimation`'s staged sub-animations: 
`sub_1614D` -> `ComputeVgaOffsetFromRowCol` (a small VGA mode-13h
linear offset helper, `row*320+col`, called only by the two functions
below), `sub_1619F` -> `SetWipeEffectPixel` (stashes the pixel
currently at `(word_328FA,word_32900)` into `_font_bgColor` —
repurposed here as a one-pixel scratch stash, not an actual font
color — then overwrites it with `_font_fgColor`), and `sub_1618E` ->
`RestoreWipeEffectPixel` (writes the stashed value back, undoing the
punch). The intro animation's loops call these while stepping the
position by ±2 each iteration and sometimes decrementing
`_font_fgColor` — a moving highlight/scan-line wipe effect, not a
persistent draw.

539 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DimDungeonViewport + ClearActionIconHighlightMask

Named two small standalone offscreen-buffer visual-effect helpers.
`sub_2BC56` -> `DimDungeonViewport` (called once from the large
unnamed combat dispatcher `sub_2C0FE`): darkens a 224×136 region of
the offscreen buffer — row 8, col 8 of the 320-byte-wide mode-13h
buffer, matching the dungeon viewport's on-screen position/size — by
subtracting 2 from every byte (palette index), i.e. one step of a
fade/dim effect. `sub_2BBD7` -> `ClearActionIconHighlightMask` (called
from the already-named `HandleRangedOrCombatAction` and
`HighlightSelectedAbilityIcon`): fills ~11.7KB of the offscreen buffer
with byte `0xFF` via 105 overlapping row-strided `rep stosw` passes —
clearing whatever mask/overlay buffer backs the action-icon highlight
effect before it's redrawn.

541 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ComputeAlchemyRefinementYield

Named `sub_2AD32` -> `ComputeAlchemyRefinementYield`, called once from
`CastSpell`: caps a BCD counter (`word_32904`, a pointer reused
generically elsewhere so its resource identity here isn't confirmed —
plausibly MAGIC ORE or NUORE given the alchemy context) at 100 (or
reads its binary value via still-unnamed `sub_19B3E` if under 100),
then divides by a divisor (2/4/5/10) tiered by the current party
member's `[+0x70]` stat — the last of the 13-entry derived-stat
column, and one of the 5 fields highlighted for a party "role" holder
— higher stat, smaller divisor, better yield. A small new data point
for that still-open "5 assignable roles" lead: `[+0x70]` gating an
alchemy/refining-flavored yield calculation is consistent with an
"alchemist"-type role. The final `div;mul;div` sequence is
mathematically redundant with a single division and isn't explained.

542 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShiftPaletteShadeClamped — resolves the tile-table "remap parameter" open question

Named `sub_2A653` -> `ShiftPaletteShadeClamped`, called 9 times from
`DrawPicture` and sibling picture-drawing code. This resolves a
long-standing open question flagged during the minimap/tile-table
investigation earlier this session: "the varying table value ... feeds
`word_32926`, a parameter `DrawPicture` passes (as `[bp+var_21]`) ...
[but] it never reads `[bp+var_21]` at all" (`sub_2A53C` was ruled out
as the reader). `sub_2A653`, sharing `DrawPicture`'s stack frame, *is*
the actual reader: it treats the input color as belonging to a
16-entry palette "hue block" (typical VGA RPG layout — 16 hues × 16
shades), computes that block's floor (`al&0xF0`) and ceiling
(`al|0x0F`), adds the `[bp+var_21]`/`word_32926` delta, and clamps the
result to stay within the same hue block rather than wrapping into a
different hue (no-op if the delta is 0 or the input is `>=0xD0`, a
reserved high palette range). In short: a signed shade-shift primitive
clamped within one hue block — exactly the kind of building block a
distance/light-based dungeon-corridor dimming effect would use. The
two tile-lookup tables themselves are still not fully traced, but
their "picture-id-like values" are now understood to be shade deltas,
not remap/color-table indices.

543 named of 769 functions as of this update.

### 2026-09-15 session update, continued: first crack into DrawViewportSprite's internals

Named two internal RLE sprite-blit primitives called from
`DrawViewportSprite` (632 lines, previously "internals not traced") —
found via `ShiftPaletteShadeClamped`'s new cross-references. Both
process a run-record table (6 bytes/record: outer-repeat-count,
inner-draw-count, skip-amount) and shade each pixel via
`ShiftPaletteShadeClamped`, tail-looping to the next record.
`sub_2A589` -> `DrawRleMaskedShadedRun`: a contiguous `lodsb`/`stosb`
run copy, optionally treating `0xFF` as transparent, with two more
chained (still-unnamed) per-pixel effects, `sub_2A4B0`/`sub_2A217`.
`sub_2A5F7` -> `DrawRleScaledSpriteColumn`: structurally different —
the destination always advances by `0x140` (320, one full mode-13h
screen row) while the source steps by a caller-supplied stride — the
classic shape for drawing one perspective-scaled sprite column,
top-to-bottom. This is a first foothold into how `DrawViewportSprite`
actually draws monster/object sprites at varying depths; the two
chained effect functions and the run-record producer(s) remain
untraced.

545 named of 769 functions as of this update.

### 2026-09-15 session update, continued: InvokePixelEffectCallback + RemapOrMaskColorByHueTable

Named the two chained per-pixel effects from
`DrawRleMaskedShadedRun`'s call chain, resolving that function's
remaining open callees. `sub_2A217` -> `InvokePixelEffectCallback`:
tiny — stashes `ax` into `[bp-0x4C]` then tail-jumps to a
caller-configured function pointer at `[bp-0x2A]`, a per-pixel effect
callback hook (doesn't itself return; the callback presumably returns
to `DrawRleMaskedShadedRun`'s own return address). `sub_2A4B0` ->
`RemapOrMaskColorByHueTable` — turned out to be called from
`DrawPicture` directly, not the sprite-blit chain: splits a color byte
into hue-group (high nibble) and shade (low nibble), searches a
16-entry stack table for a matching hue-group, and on a match either
forces full transparency (`al=0xFF`, when the table entry's low byte
is `0x0F`) or remaps just the hue-group nibble while preserving the
shade — plausibly a status-effect tint/recolor mechanism (stun flash,
poison tint) that recolors a sprite's whole hue band without
disturbing its shading gradient.

547 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawShadedPixelRun + CopyShadedViewportRows

Named two more members of the `ShiftPaletteShadeClamped` blit family.
`sub_2A51B` -> `DrawShadedPixelRun` (called twice from
`DrawViewportSprite` directly, not through the RLE record table): the
simplest member — a straight run copy of `cx=[bp-0x14]` pixels,
shaded, with masking (`0xFF`=skip) only when `[bp-0x20]` is nonzero,
otherwise unconditional. `sub_2A0FC` -> `CopyShadedViewportRows`
(called repeatedly from unnamed `sub_29FF6`, 262 bytes, references
`_videoSegment`, not traced this round): copies a 224-pixel-wide row
(matching `DimDungeonViewport`'s viewport width) from `si` to `di`,
shading every pixel, then advances both by `0x140` (320, one screen
row) and repeats for the caller's outer count — a "copy this
viewport-width region N rows at a time" primitive. `sub_29FF6` itself
is a solid next-round candidate now that its main inner loop is
understood.

549 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplyDistanceShadingToFloorOrCeiling — a real lighting-model find

Named `sub_29FF6` -> `ApplyDistanceShadingToFloorOrCeiling`, called
twice from `DrawDungeonFloorAndCeiling`. A genuinely interesting find:
it re-shades the just-drawn floor or ceiling buffer *in place*
(`CopyShadedViewportRows` called with `si==di`), branching on
`word_2E532` for floor (`0xA08`) vs. ceiling (`0x5788`) addressing,
and walks a **fixed 7-entry shade-delta gradient table**
(`word_328E6` through `word_328F2`, consecutive words, not traced
this round) as successive per-row-band deltas — 7 row-bands of
10/11/10/10/9/9/3 rows for the floor, 4/9/9/10/10/11/21 in reverse
gradient order for the ceiling. This is very likely the concrete
mechanism behind the "plausible torch-fuel/light-source" derived stat
(`+0x64`) flagged much earlier this session: a precomputed 7-step
brightness falloff applied band-by-band with distance from the
viewer. Where the `word_328E6`-`word_328F2` gradient values themselves
get computed (presumably from `+0x64`) is a good next lead.

550 named of 769 functions as of this update.

### 2026-09-15 session update, continued: confirmed the lighting-gradient table's other consumer, and fixed a stale comment

Followed up on the `word_328E6`-`word_328F2` gradient table by
checking `RenderDungeonViewport` (already named), which turned out to
already reference all 7 globals directly — and its pre-existing
comment called them "different row-data pointers," which this
session's new evidence shows is wrong. `RenderDungeonViewport` copies
each of `word_328E6`..`word_328F2` into `word_32926` before each of 7
`RenderDungeonViewRow` calls (decreasing cell counts
`0x11/0x11/5/3/3/3/3`) — and `word_32926` is exactly the shade-shift
delta `DrawPicture`/`ShiftPaletteShadeClamped` apply per pixel (traced
two rounds ago). So the same 7-entry gradient
`ApplyDistanceShadingToFloorOrCeiling` walks for the floor/ceiling is
also applied to every wall/monster picture drawn in each of the 7
depth rows via `RenderDungeonViewRow` — a single, shared, consistent
distance-lighting gradient across the whole dungeon viewport (walls,
monsters, floor, and ceiling alike). Fixed `RenderDungeonViewport`'s
comment in place (`ida_scripts/fix_render_dungeon_viewport_comment.py`)
to reflect this. No new function named this round — this was a
documentation correction — but it substantially firms up the lighting
model. Where the gradient values themselves get computed (presumably
from `+0x64`) remains the next lead.

### 2026-09-15 session update, continued: AccumulateLearnedAbilityFlags

Named `sub_29040` -> `AccumulateLearnedAbilityFlags`, called from
`UseItem` and `UseAbilityScroll`: resolves the current character
(`word_32924` id) and reads their learned-abilities bitmask
(`[+0xB4]`, the confirmed "which of at least 4 special abilities this
character has learned" field), then walks a table at `word_2E54C`
(stride `0x3A`, count `word_2E432`, identity not otherwise traced),
ORing each qualifying entry's `[+0x16]`/`[+0x18]` fields into two
accumulator globals (`word_2E40C`/`word_2E40E`). An entry qualifies
if its `[+0x10]` bit 1 is set (always applies), or its `[+0x0E]` bit 8
is set and its `[+0x12]` ability-bitmask overlaps the character's
learned abilities. Reads as "accumulate the flags contributed by
every catalog entry this character's known abilities unlock" —
plausibly determining available item-use options — though the
`word_2E54C` table's own identity isn't confirmed.

551 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the ErrorTable's 4 anomalous trailing entries

Named the last 4 entries of the pre-existing `ErrorTable` jump table
(indices 16-19): `sub_28A19`/`sub_28A1F`/`sub_28A25`/`sub_28A2B` ->
`ErrorExitCode281`/`ErrorExitCode285`/`ErrorExitCode289`/
`ErrorExitCode289Alt`. Every earlier `ErrorTable` entry sets `ax` to
`offset aXxx`, a real string pointer into the message-text block
starting near `aMemoryAllocati` (actual address ~`0x28715`+); these
last four instead set `ax` to a tiny raw value (`0x281`, `0x285`,
`0x289`, `0x289` again) — two orders of magnitude too small to
address that same string block, and the last two entries share the
identical value, breaking the `+4`-per-entry progression the first
three suggest. This looks like incomplete/vestigial reserved
error-code slots that never got real message text — possibly related
to content stripped from this shareware build — rather than anything
with a discoverable message. Named modestly, without inventing
message content that isn't actually there; flagged as an open
curiosity rather than a confirmed finding.

555 named of 769 functions as of this update.

### 2026-09-15 session update, continued: Sound Blaster environment-variable auto-detection

Named a 3-function cluster behind sound hardware setup, called from
`InitSoundSystem`. `sub_2D578` -> `FindEnvironmentVariable`: the
classic DOS technique — gets the PSP segment (`INT 21h AH=0x51`),
reads the environment segment pointer at `PSP+0x2C`, and scans the
env block for a caller-supplied search string, copying the matched
value out. `sub_28619` -> `ParseSoundBlasterEnvironmentVariable`:
calls it twice against two env-var name constants, then parses the
returned value for `'A'<3 digits>` into `word_32916` and `'I'<1
digit>` into `word_32914` — exactly the classic `BLASTER=A220 I5 D1
T3` Sound Blaster format (base address + IRQ), setting
`word_36CE3`/`word_36CE1` on success and `g_driverStateFlags`
fallback bits on failure. While tracing this, found a nearby string
cluster (`BLASTER=`, `SOUND=`, `EMMXXXX0`, `FMDRV`) that strongly
supports this reading, though the exact byte addresses of the two
name constants (`0xCC1`/`0xCB8`) didn't line up precisely with those
strings on manual inspection — the naming rests on the parsing logic
itself, which is unambiguous, not on a byte-exact string match.
`sub_2827E` -> `WaitForSoundDriverIdle`: a small gate that returns
immediately unless `g_driverStateFlags` bit `0x8` is set, in which
case it busy-waits for `word_2E494` to reach 0 — "wait for the sound
driver's current operation to finish," used by several sites including
the still-open `sub_2D498` lead from earlier this session.

558 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TryPlaySoundCue / Alt

Named another byte-for-byte-identical overlay-segment duplicate pair
(joining `DrawShadowedText`/`Alt`, `ConfirmContainerInteraction`/
`ConfirmAlchemyInteraction`, and `PollForEscapeKeyOnly`/`Alt`):
`sub_16234` -> `TryPlaySoundCue` and `sub_11EAE` ->
`TryPlaySoundCueAlt`. Both drop a sound cue (`cx`=sound/note id) if
`WaitForSoundDriverIdle` had to wait (driver was busy) or `cx` is the
`0xFFFF` no-op sentinel; otherwise dispatch via a still-unnamed
per-segment helper (`sub_1616F`/`sub_11E39`). Noted while investigating
this pair that they behave the *opposite* way from the earlier,
harder-to-read `sub_2D498` (which dispatches its own sound command,
`sub_28412`, only when the driver *was* busy and had to be waited out,
and otherwise just delays — the reverse of "drop it if busy"), and
`sub_2D498` itself carries an IDA "sp-analysis failed" flag plus a
`cmp ax,0; jz` branch that appears to jump back into its own `pop ax`
instruction — a genuine anomaly, not just an unfamiliar convention.
Left `sub_2D498` deliberately unnamed rather than force an
interpretation past what's actually confirmed.

560 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PrepareAmbientMusicBlockRead, StripCommasAndSpaces, FormatNumberCompact

Named three small unrelated helpers. `sub_2801A` ->
`PrepareAmbientMusicBlockRead` (called once from the already-named
`UpdateAmbientMusicForRegion`): configures a resource-read descriptor
in the same shape as this codebase's other resource-stub helpers
(`sub_27DE5`/`sub_27DC6`/`sub_27E3A`), specific to ambient-music
region data — writes a fixed pair from table `0xCDFB` and global
`_blockSize5`. `sub_2570C` -> `StripCommasAndSpaces` (called from
`ShowItemAbilityEffectInfo` and others): strips `,`/` ` from an
in-place string — the natural counterpart to `FormatNumber`'s
thousands-separator insertion. `sub_28138` -> `FormatNumberCompact`
(called repeatedly from unnamed `sub_28034`): chains the two — formats
a number into the shared `0xAFA8` buffer then immediately strips its
separators back out, returning a compact numeric string.

563 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawLabeledNumberRow

Named `sub_13C86` -> `DrawLabeledNumberRow`, called from
`ShowClueBookSpellDetail` (for its "MP:"/"NUORE:"/"ORE:" cost-field
rows) and from sibling `sub_13C1D` (using it for a fixed value of `1`
with a special highlight color, gated on a 2-entry class-id match
against `word_3330A` — plausibly part of the documented "6-class
eligibility marker row"). A generic row-drawing primitive: writes a
caller-preset string at `x=0x7A`, a `FormatNumber`+
`StripCommasAndSpaces`'d number at `x=0x68`, writes again at `x=0x2C`,
then advances `_textPos_y` by 6 and clears `errorCode`. The exact
visual relationship between the two string draws (whether the second
redraws the same text or something `FormatNumber` left behind) wasn't
independently confirmed — `writeString`'s own effect on `si` isn't
traced.

564 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawSpellLevelForCurrentClass + DrawClassEligibilityMarker

Named two more `ShowClueBookSpellDetail` helpers built on
`DrawLabeledNumberRow`. `sub_13C4B` -> `DrawSpellLevelForCurrentClass`:
searches a 20-level × 2-class-slot table for a match against the
current class id (`word_3330A`), and on a match draws the matched
level via `DrawLabeledNumberRow` with a fixed "LEVEL:" label —
resolving the documented "LEVEL:" field's mechanism. `sub_13C1D` ->
`DrawClassEligibilityMarker`: checks a 2-entry candidate array against
the same `word_3330A` and, on a match, draws a fixed `1` in a
highlight color — plausibly one cell of the documented "6-class
eligibility marker row." Both round out the spell-detail screen's
per-field drawing logic.

566 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SetRecordFlag_CA + TestRecordFlag_CA

Named `sub_27A4E` -> `SetRecordFlag_CA` and `sub_27A66` ->
`TestRecordFlag_CA`, completing the accessor family for the `+0xCA`
per-record flag bank to match the sibling `+0x10C` bank's already-named
`GetRecordFlagBitAndWord_10C`/`SetRecordFlag_10C`/`TestRecordFlag_10C`
trio. Simple wrappers around the already-named
`GetRecordFlagBitAndWord_CA`: OR its bit mask into `[si]` to set, or
`[si] & mask` to test. Called from `UseTrainingItem`/`sub_25456`
(set) and `BuildAlchemySpellList`/`MarkIneligiblePartyMembers` (test).

568 named of 769 functions as of this update.

### 2026-09-15 session update, continued: IsItemEligibleForCommand

Named `sub_26A75` -> `IsItemEligibleForCommand`, called once from
`sub_2621C` (the container/inventory interaction handler, called from
`ShowCharacterInventory` and `sub_1869D`) — a generalized eligibility
gate, taking `ax=word_2E40A` (the current key/command code, the same
convention documented for other screens' dispatch) and `bx` (an
item's catalog record, from the immediately-preceding
`LoadItemCatalogRecord` call). Command codes `<=8` always pass; codes
9 through `0x14` (20) each check a specific bit of either the item's
own `[bx+0xC]` flags or the separate `word_2E548[+2]` flags (the same
field `ClassifyItemServiceTier`/`GetClassifiedItemStatField` use), two
of them (`0xB`, `0xD`) adding an extra check against the current party
member's own record. Sets `errorCode=1` on ineligibility, triggering
the caller's `FlashStatusWarning`. This generalizes the pattern
already seen in `IsItemEligibleForEnhance`/`IsItemEligibleForRepair`
into one combined switch; the individual command codes' specific
meanings weren't identified.

569 named of 769 functions as of this update.

### 2026-09-15 session update, continued: CloseAllAlternateBags + ClearDepletedResourceCounterForCommand

Named two more small helpers. `sub_266A9` -> `CloseAllAlternateBags`
(called once from `RestorePortraitAreaAtPosition`): clears the current
party member's `[+0x15C]` high status bits, then calls
`SaveAndCloseContainer` for each of the 3 documented alternate-bag
marker offsets (`0x17C`/`0x1A2`/`0x1C8`) — closing every open bag for
that member, e.g. when leaving the inventory screen. `sub_26928` ->
`ClearDepletedResourceCounterForCommand` (called from `PlaceItemInSlot`
and `PickUpItemFromSlot`): zeroes one of `[si+0xBE]`/`[si+0xC0]`/
`[si+0xC2]` depending on the current command code (`word_2E40A`) — the
same 3 fields the still-untraced `sub_274B4` clears for specific item
types via its own `ShowResourceDepletedOverlay` path, adding another
data point toward eventually tracing that dispatcher.

571 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ApplySecondaryClassTierFlags — extends the confirmed +0x1C bitfield

Named `sub_25456` -> `ApplySecondaryClassTierFlags`, called once from
`ShowCharacterSummary` — a genuinely new structural finding. The
party-record `+0x1C` bitfield was previously fully mapped for its
affliction bits (`0x40` and up: DEAD, CURSED, HEXED, JINXED, STONED,
FROZEN, PARALYZED, DISEASED, POISONED, SICK); this function reveals
that the **low 6 bits (`0x1`-`0x20`)** of the same field encode which
"secondary class" (ids 4-9 of the 27-class table: MONK, ALCHEMIST,
PALADIN, MAGE, DRUID, MARKSMAN) the character has reached. Gated on
`[bx+0x94]` being nonzero (an unconfirmed "eligible" marker,
`bx=word_328D4`), it maps the character's class id (`[bx+0xE]`) to the
matching bit and ORs it into `[bx+0x1C]`, then also sets up to 2
class-specific ability flags from a small fixed table via the
already-named `SetRecordFlag_CA` (the `+0xCA` per-record ability
bank) — plausibly a class-promotion/unlock effect, re-applied each
time the summary screen is shown rather than a one-time event.
`[bx+0x94]`'s own trigger condition remains unconfirmed.

572 named of 769 functions as of this update.

### 2026-09-15 session update, continued: FormatNumberZeroPadded, StripCommasZeroPadSpaces, ClampDragCursorPosition

Named three more small helpers. `sub_2A766` -> `StripCommasZeroPadSpaces`
(called once from `sub_2572C`): strips commas from an in-place string
like `StripCommasAndSpaces`, but converts each space to `'0'` instead
of removing it — a zero-padding variant (e.g. `"  42"` → `"0042"`
after removing thousands separators). `sub_2572C` ->
`FormatNumberZeroPadded` (called from still-unnamed `sub_2044C`/
`sub_2047B` and others): chains `FormatNumber` with
`StripCommasZeroPadSpaces` on the shared `0xAFA8` buffer — the
zero-padded sibling of `FormatNumberCompact`. `sub_239CD` ->
`ClampDragCursorPosition` (referenced from a data/jump table in
`seg073`, not a direct call): clamps an accumulated drag position
(`word_2E782`/`word_2E784`) within bounds, then offsets it by `(8,8)`
unless the currently-held item type (`word_31946`) is `0` (none) or
`0x1D` (a specific item type that apparently doesn't need the hotspot
offset) — plausibly the cursor position used to draw a held/dragged
item.

575 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawMapEditorCoordinateReadout

Named `sub_2044C` -> `DrawMapEditorCoordinateReadout`, called from
`RunMapEditorScreen` at multiple points: draws `word_2E384`,
zero-padded via `FormatNumberZeroPadded`, at a fixed screen position
`(4,1)`, then skips the first 2 characters of the formatted result
before drawing — plausibly trimming a fixed-width zero-padded value
down to its last 2 significant digits. A small coordinate/position
readout in the editor's corner; the exact field `word_2E384`
represents (row, column, or cursor index) isn't confirmed. Also
looked at `sub_22989` (called from unnamed `sub_2278C`, combines
`PickRandomActivePartyMember`, the still-open `+0x50` field from the
unidentified attribute trio, a facing-direction dispatch on
`word_36CF5`, and an unclear helper `sub_227F5`) — plausibly a
random-ambush/surprise-encounter setup, but too many unresolved
dependencies to name confidently this round; left as an open lead.

576 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TriggerSideTrapForRandomPartyMember — resolves last round's open lead

Named `sub_227F5` -> `RollTrapAvoidanceMagnitude` and `sub_22989` ->
`TriggerSideTrapForRandomPartyMember`, resolving the open lead flagged
last round. `RollTrapAvoidanceMagnitude` is a save-vs-trap-style
avoidance roll: subtracts a character's stat (the still-mysterious
party-record field `+0x50`) from a trap record's threshold; if
positive, rolls `RandomInRange(100)` against the remaining margin, and
on success computes a magnitude scaled by that margin into
`word_2E49C` — "the higher this stat relative to the trap's
threshold, the less likely and smaller the effect."
`TriggerSideTrapForRandomPartyMember` picks a random active party
member (`PickRandomActivePartyMember`), rolls this avoidance check
using their `+0x50` stat against a trap/side-feature record's
threshold and magnitude cap, and — only if the party's current facing
(the same `word_36CF5` tier-bit convention as
`DrawDungeonCellSideFeature`/`ShowCompassDirection`) matches one of 4
direction bits on the trap record's own flags — finishes populating an
icon-bar-style output record for the effect system. In short: a
wall/door-embedded "side trap" trigger, gated on both a stat-based
avoidance roll and facing direction. This is a second, independent
data point (alongside `ComputeAlchemyRefinementYield`'s `+0x70` use)
that `+0x50`/`+0x70` and their siblings in the still-open
"`+0x4C`/`+0x4E`/`+0x50` trio" lead are general character stats
reused across multiple unrelated systems, not single-purpose fields.

578 named of 769 functions as of this update.

### 2026-09-15 session update, continued: the complete side-trap pipeline — ProcessSideTrapsOnMovement + PresentTriggeredSideTrapEffects

Named the two remaining functions that complete the "side trap"
system uncovered this session. `sub_2278C` -> `ProcessSideTrapsOnMovement`
(called directly from `start`, likely after each movement step):
fast-exits unless `word_328C8` bit `0x10` is set, otherwise walks an
80-entry wall/cell table (stride `0x9C`) calling
`TriggerSideTrapForRandomPartyMember` for every entry flagged with a
side trap, then hands off to `sub_2281F`. `sub_2281F` ->
`PresentTriggeredSideTrapEffects`: a rich resolution/presentation
sequence for up to 4 triggered results — plays the trap's sound cue
once the driver is idle, draws weapon-style icons and does a full
dungeon-screen refresh, picks the highest-severity result to drive a
scaled projectile-style animation (`AnimateProjectileStep`), and
finally transfers the results into the confirmed icon-bar slot table
(`0xC50`) via `ApplyEffectAndDrawIconBar`. This completes a fully-
traced pipeline from trap detection through avoidance roll, sound,
visual effects, and icon-bar application — a satisfying close to a
thread that started several rounds ago as an unexplained `sub_16DAA`
disassembly dump.

580 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RedrawAllPartyStatusPanelsAlt + MaybeForceTickWorldAilments

Named two more small helpers. `sub_222F8` -> `RedrawAllPartyStatusPanelsAlt`
(called from `HandleMovementInput` and `InitGame`): unconditionally
redraws all 4 `g_partySlotAssignment` status panels via far calls —
another instance of this session's recurring overlay-segment
duplicate-function pattern, though not byte-identical to the
already-named `RedrawAllPartyStatusPanels` (which instead loops with
an occupied-slot check via near calls). `sub_1FC3F` ->
`MaybeForceTickWorldAilments` (called from `ApplyMapTriggerEffect` and
`RestPartyAndAdvanceClock`): gated on `word_3295A` bit `0x800`, resets
a counter (`word_1F984`) to `0x270F` (9999) and calls the already-named
`TickWorldAilments`; a no-op otherwise.

582 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DeductAlchemySpellCosts + RestoreOrSelectAlchemyCaster

Named two more `RunAlchemyScreen` helpers. `sub_1E61B` ->
`DeductAlchemySpellCosts`: subtracts an MP cost from the current party
member's MP (`[bx+0x54]`), then pays NUORE and MAGIC ORE costs via
`SubtractFromBCDCounter` against the confirmed `0x94BB`/`0x94B7`
counters — matching the documented "MP:"/"NUORE:"/"ORE:" cost fields.
`sub_1E473` -> `RestoreOrSelectAlchemyCaster` (called at screen entry):
re-validates a cached caster id against `g_partySlotAssignment` (using
the same `[+0x94]` "class-tier eligible" marker `ApplySecondaryClassTierFlags`
gates on) and sets `word_32924` (the same party-record slot pointer
`AccumulateLearnedAbilityFlags` reads) to the matching slot, falling
back to unnamed `sub_1E447` otherwise.

584 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SelectDefaultAlchemyCaster + CopyPartyStatBlockToEmsCache

Named `sub_1E447` -> `SelectDefaultAlchemyCaster`, resolving
`RestoreOrSelectAlchemyCaster`'s fallback path: scans
`g_partySlotAssignment` for the first occupied slot whose record has
`[+0x94]` set and adopts it as the default alchemy caster. Also named
`sub_1CC70` -> `CopyPartyStatBlockToEmsCache` (called from
`UseItemType_400` and `UseTrainingItem`): copies 30 words from the
party member's record into the same relative offsets in a separate
EMS-mapped segment (`word_2E4AA`), then calls
`UpdatePartyAverageStatTiers` — plausibly a before/after stat snapshot
for a stat-changing item, though the destination buffer's exact
purpose isn't confirmed.

586 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ComputeAfflictionHealingCost

Named `sub_1BA35` -> `ComputeAfflictionHealingCost`, called twice from
`UseHealingItem`: sums a per-affliction cost over the confirmed
`+0x1C` bitfield — SICK `+5`, POISONED `+10`, DISEASED `+20`,
PARALYZED `+40`, FROZEN `+50`, STONED `+60`, JINXED `+20`, HEXED `+30`,
CURSED `+40` — returning the combined total. Ties directly into the
already-named `ShowHealingCostPrompt`, giving that screen's cost
calculation a concrete mechanism.

587 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ClampValueAtSlotToTypeCap

Named `sub_1B5DA` -> `ClampValueAtSlotToTypeCap`, called twice from
unnamed `sub_1B4C2`: a saturating clamp — if `ax` exceeds a
type-dependent cap, writes the cap into `[si]` (otherwise a no-op).
The cap is `0x270F` (9999) when `word_32940` equals `'R'`/`'T'`
(plausibly single-character item-type markers, not confirmed), else
`0x3E7` (999) — the same cap `ApplyMultiStatEffectForItem` already
uses for its own stat additions.

588 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RecomputeEquipmentStatBonuses — a new equipment-slot layout

Named `sub_1B30C` -> `RecomputeEquipmentStatBonuses`, called from
`sub_18C79`, `sub_1AA9B`, and directly from the long-open
`sub_274B4` dispatcher (right after `ApplyMultiStatEffectForItem`) —
a meaningful new structural finding. For the current party member, it
resets two 5-word "derived equipment bonus" blocks
(`[si+0x48..0x50]`/`[si+0x88..0x90]`) from base values
(`[si+0x32..0x3A]`/`[si+0x72..0x7A]`, both pairs `0x16` apart — a
*different* pairing scheme than the confirmed `+0x40` base/derived
attribute convention), then walks the character's equipped-item
slots — main weapon (`+0x13A`), a second slot (`+0x142`, whose
catalog-flag bits select which bonus-field pair its value adds into),
a 3-entry array (`+0x146`), and a 5-entry array (`+0x152`) — adding
each equipped item's catalog stat bonus into the running totals via
`LoadItemCatalogRecord`. In short: a full equipment-derived stat
recompute triggered whenever gear changes. This is the first solid
structural read on `sub_274B4`'s own equipment-slot layout, though
the dispatcher itself remains untraced.

589 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RefreshCarryCapacityAndAttributeBonuses — new data point on `+0x3E`/`+0x7E`

Named `sub_1AA9B` -> `RefreshCarryCapacityAndAttributeBonuses`, called
from `HandleIconBarItemExpiry` and `ApplyIconBarStatDelta`. Recomputes
carry capacity (`[+0x56]`/`[+0x96] = [+0x3C]`/`[+0x7C] * 10`,
re-confirming the documented Strength-derived carry-capacity fields),
then for 4 fields — `[+0x3C]→[+0x38]`, `[+0x3E]→[+0x3A]`,
`[+0x7C]→[+0x78]`, `[+0x7E]→[+0x7A]` — zeroes the target unless the
source exceeds `0x48` (72), in which case `20%` of the excess is
scaled in via `ScaleByPercentRounded`. Finishes by calling
`RecomputeEquipmentStatBonuses`. This gives `+0x3E`/`+0x7E` (one of
the 6 core attributes previously noted as having "no secondary use
found yet") its first concrete secondary use: values above 72 grant a
scaled threshold bonus into `+0x3A`/`+0x7A`.

590 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TickTravelResourceAilments

Named `sub_1A582` -> `TickTravelResourceAilments`, called once from
`TravelToDestination` — resolves the caller referenced in an earlier
`CheckAndTickAvailableAilment` entry. Clears `word_328C4` bit `0x40`,
then calls `CheckAndTickAvailableAilment` 3 times with distinct
(item/effect id, range/count) pairs — checking 3 resource/consumable
item types for availability during world travel, plausibly
food/water/light-source tracking, though the specific item ids aren't
confirmed.

591 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TickPerceptionGatedAilmentSlot

Named `sub_1A233` -> `TickPerceptionGatedAilmentSlot`, called once
from `TickPartyAilmentIconBar`. No-ops for dead characters (`+0x1C`
bit `0x40`); otherwise tiers the character's `+0x58` perception stat
against 6 ascending thresholds to a *decreasing* severity/duration
value (higher perception, smaller value), and — if nonzero — fills an
icon-bar slot with it alongside the staged effect id/magnitude pair
(`word_3293E`/`word_32940`) and the character pointer, then sets
`word_328CA` bit `0x100`. A perception-gated ailment effect,
plausibly confusion/disorientation-flavored, whose severity shrinks
as perception rises.

592 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TickCurseHexJinxAilmentSlot

Named `sub_1A195` -> `TickCurseHexJinxAilmentSlot`, called once from
`TickPartyAilmentIconBar` — pins down the concrete address for the
"id `0xE`" helper already described in `file-formats.md`
(CURSED/HEXED/JINXED, MP-gated). No-ops if incapacitated or out of
MP; otherwise sums a weighted severity (`0x10`/`8`/`4` per active
affliction) and, if nonzero, fills the icon-bar slot the same way its
sibling `TickPerceptionGatedAilmentSlot` does. This resolves the last
unaddressed piece of `TickPartyAilmentIconBar`'s three helper paths.

593 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TickDiseasePoisonSickAilmentSlot — completes the TickPartyAilmentIconBar cluster

Named `sub_1A14D` -> `TickDiseasePoisonSickAilmentSlot`, called once
from `TickPartyAilmentIconBar` — the concrete "id `2`" helper already
described in `file-formats.md` (DISEASED/POISONED/SICK, the normal
path). No-ops if incapacitated; otherwise sums a weighted severity
(`0xC`/`6`/`3` per active affliction) into the icon-bar slot the same
way its two now-named siblings (`TickCurseHexJinxAilmentSlot`,
`TickPerceptionGatedAilmentSlot`) do. With all three helpers named,
`TickPartyAilmentIconBar`'s entire dispatch structure is now fully
traced end to end.

594 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ExtractDecimalDigit

Named `sub_18041` -> `ExtractDecimalDigit`, called repeatedly from
`FormatNumber`'s own implementation: the digit-extraction helper for
its digit-by-digit decimal conversion loop — divides the remaining
value by a power-of-10 divisor, writes the quotient as an ASCII digit,
subtracts it back out, and blanks a leading zero to a space unless a
flag marks this as the final (ones) digit. Gives `FormatNumber`'s own
internals a concrete, named building block.

595 named of 769 functions as of this update.

### 2026-09-15 session update, continued: BuildShopCategoryTabList

Named `sub_179AE` -> `BuildShopCategoryTabList`, called once from
`RunShopScreen` near the start: clears an EMS-mapped scratch buffer
then walks a fixed 8-entry category table gated by a per-shop-type
bitmask (`byte_32DCC`), storing `[category_id, value]` pairs — ids
`1`/`2`/`3` get fixed globals (plausibly special currency-like tabs
such as gold/ore/nuore exchange), other categories pull a value from
their own catalog record when eligible. Sets up the shop screen's
category tab list.

596 named of 769 functions as of this update.

### 2026-09-15 session update, continued: TryHandleCatalogSlotClick

Named `sub_17270` -> `TryHandleCatalogSlotClick`, called from
`RunShopScreen` and the main input loop `sub_1869D` — a small gate
distinct from the already-documented mouse-click "buy" handler
`sub_17032` (called from the same two sites at different offsets).
Bails on no hit or an empty slot, otherwise dispatches to a large,
untraced helper (`sub_219FA`, 251 lines) — plausibly a select/preview
interaction separate from the purchase flow. Named narrowly around
its confirmed gate behavior; `sub_219FA` itself remains an open lead.

597 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ParseCommandLineSwitches

Named `sub_16F84` -> `ParseCommandLineSwitches`, called directly from
`start` at program entry — a classic DOS command-line switch parser.
Reads the PSP command-tail (`INT 21h AH=0x51`) and scans for
`/`-prefixed switches: `/P` sets `word_328C4` bit `0x8000`; `/NOM`
sets `word_328C8` bit `2` (plausibly no-music, tying into the
BLASTER/sound-driver detection traced earlier this session); `/NOS`
sets `word_328C8` bit `1` (plausibly no-sound).

598 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PlayPcSpeakerBeep + TriggerSoundEvent — 600 named, and the sound-driver dispatch lead finally resolved

Named `sub_16DEA` -> `PlayPcSpeakerBeep`: a classic PC speaker beep —
programs the 8253/8254 PIT channel 2 with a tone divisor, gates it to
the speaker via PPI port `0x61`, waits 4 ticks, then disables it.
Called from the long-open `sub_28412` — which this round is also
finally named, `TriggerSoundEvent`. That function's own pre-existing
comment already explained most of its shape (a sound-event dispatcher
taking a command in `ax`); with `PlayPcSpeakerBeep` now identified,
the picture is complete: when the Sound Blaster driver isn't active
(`g_driverStateFlags` bit `3` clear), only `ax==3` does anything — the
PC-speaker fallback beep — every other command is silently ignored.
When the driver *is* active, it reads driver data and forwards to the
loaded driver's own routine via `g_soundDriverFarPtr(bx=6)` — likely
"load+play a sound effect" in the driver's own external protocol,
which (being outside this binary) still isn't independently
confirmed. **Correction**: when `PlayPcSpeakerBeep` was first named
this round, its comment speculated it might correspond to "command
6" — wrong; it's actually reached via `ax==3` in the driver-*inactive*
fallback path, while `6` is the unrelated value forwarded to the
*active* driver. Fixed in place. This closes out a lead that had
stood open since very early in this session's work.

600 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PlaySoundSequenceGH + TriggerSoundEventAfterDriverWait — the last flagged sound-cluster lead resolved

Named `sub_11E1C` -> `PlaySoundSequenceGH` (called from `sub_11A10`):
waits for the sound driver to go idle (via `TryPlaySoundCueAlt`'s own
idle-wait, called with the guaranteed-no-op sentinel `cx=0xFFFF`),
triggers sound event `0x47`, waits again, triggers `0x48` — a
two-part sound cue.

Also finally named `sub_2D498` -> `TriggerSoundEventAfterDriverWait`
(called twice from the large unnamed combat dispatcher `sub_2C0FE`),
resolving the "structurally odd" lead flagged and deliberately left
open two rounds ago. With all three of its callees now named
(`WaitForSoundDriverIdle`, `wait`, `TriggerSoundEvent`), its shape is
fully legible even though it's genuinely unusual: if the driver was
already idle, the sound command is *discarded unplayed* and it just
waits 6 ticks; only if the driver was busy (and `WaitForSoundDriverIdle`
had to wait it out) does the sound actually play. This is the inverse
of the sibling `TryPlaySoundCue`/`Alt`'s "drop it if busy" pattern.
The `ax==0` branch still contains the genuine anomaly noted before (a
loop back into its own `pop ax`, flagged by IDA's own sp-analysis
failure) — left exactly as observed rather than reinterpreted, most
likely unreachable dead code. This closes out every function this
session's sound-system investigation had flagged as open.

602 named of 769 functions as of this update.

### 2026-09-15 session update, continued: WaitForKeypressTickingMusic

Named `sub_162B6` -> `WaitForKeypressTickingMusic`, called from 19
sites across the codebase — a core "wait for a key, ticking ambient
music each iteration" building block, distinct from the larger,
already-named `PollKeyboardInput`. Loops `UpdateAmbientMusic` +
`INT 21h AH=6` key polling; stores a received key into `byte_2E400`,
or loops again unless `word_3195C` bits `0x2400` gate an early exit
(clearing the key buffer, or forcing a synthetic ESC). Flushes the
DOS keyboard buffer before returning. With 19 call sites, this is one
of the more heavily-used primitives named this session.

603 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RunPaletteFadeSequence + SetPaletteToWhite — resolves two of PlayCharacterCreationIntroAnimation's animation-frame primitives

Named `sub_160D6` -> `RunPaletteFadeSequence`, called twice from
`PlayCharacterCreationIntroAnimation` — a multi-step palette fade, a
sibling of the already-named `FadePaletteStep`, that ties together
the palette-buffer addresses already identified this session (`0x4D5C`
current / `0x442A` target / `0x475A` output). For each of several
steps, nudges each "current" byte toward its "target" byte, mirrors
the result into the output buffer unless a `0x80` sentinel bit is set,
then applies that step via `SetPaletteRange`. Also named `sub_16244`
-> `SetPaletteToWhite`, called twice from `sub_1559A` (character
creation step 3): fills the palette buffer with the maximum 6-bit DAC
value and applies it — a full-palette white flash, plausibly a
screen-transition effect. Together these resolve 2 of the 8
previously-unnamed animation-frame helpers flagged when
`PlayCharacterCreationIntroAnimation` was first named.

605 named of 769 functions as of this update.

### 2026-09-15 session update, continued: InsertDecimalPointFromEnd

Named `sub_16262` -> `InsertDecimalPointFromEnd`, called from
`DrawLabeledNumberIfNonzero` and `FormatAndDrawAlchemyFraction`: a
fixed-point decimal formatting helper — inserts `.` into an in-place
number string, `word_2E4AC` digits from the end (e.g. `"1234"` →
`"12.34"` for `word_2E4AC=2`) by shifting the trailing digits right.
No-op if the string is empty or the digit count isn't positive.

606 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SetPaletteToWhiteAlt

Named `sub_11EBE` -> `SetPaletteToWhiteAlt`, called once from
`sub_11A10` — byte-for-byte identical to the just-named
`SetPaletteToWhite`, another instance of this session's recurring
overlay-segment duplicate-function pattern.

607 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ClearOffscreenBuffer + ClearOffscreenBufferAlt

Named another byte-for-byte-identical overlay-segment duplicate pair:
`sub_152E1` -> `ClearOffscreenBuffer` (called from
`FinalizeCharacterCreation` and `sub_1559A`) and `sub_11D58` ->
`ClearOffscreenBufferAlt` (called from `sub_11A10`) — both clear the
entire offscreen buffer (`0x7D00` words, a full mode-13h screen) to 0.

609 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ReleaseEmsHandles

Named `sub_12624` -> `ReleaseEmsHandles`, called directly from `start`:
releases both EMS handles the game uses (`_emsPointer1?`, the primary
handle used throughout the icon-bar/trap/portrait EMS system, and a
second one, `word_2E504`) via `INT 67h AH=0x45`, skipping each if
already unallocated (`0xFFFF`). A classic shutdown/cleanup routine.

610 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PaintCursorCellAndPersist

Named `sub_20652` -> `PaintCursorCellAndPersist`, called once from
`RunMapEditorScreen`: paints the selected tile at the cursor's map
cell, persists it via `FileEntry_Write`, and redraws it via
`DrawCellIconPair` — the single-cell counterpart to
`FillVisibleAreaWithSelectedTile`'s bulk `PaintCellAndPersist` loop.

611 named of 769 functions as of this update.

### 2026-09-15 session update, continued: PaintCursorOverlayCellAndPersist

Named `sub_2070C` -> `PaintCursorOverlayCellAndPersist`, called once
from `RunMapEditorScreen` — the overlay/wall-tile sibling of
`PaintCursorCellAndPersist` (byte-for-byte the same structure, with
different globals): paints into the cell record's `[si+2]` (overlay
field) at a second cursor position, persists, and redraws.

612 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawAndCacheStatusIcon

Named `sub_22315` -> `DrawAndCacheStatusIcon`, called from
`HandleMovementInput` and `ProcessLevelMonsters`: draws a small status
icon (one of 2 variants, gated on `word_328CA` bit `0x1000`, the
same "in combat" style flag seen elsewhere) at a fixed position, then
caches the drawn region into EMS page `0x55D8` — the confirmed
portrait/dungeon-screen cluster page — for later restoration, the
same convention already documented for the portrait cache.

613 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowAbilityDescriptionColumn

Named `sub_29461` -> `ShowAbilityDescriptionColumn`, called from
`UseAbilityCommand` and `UseAbilityOnTarget`: clears the status panel
if dirty, positions text at a fixed spot with a highlight color,
restores the cursor background if dirty, then calls the still-unnamed
`DrawStringColumn` (a heavily-referenced multi-line text renderer) —
plausibly showing an ability's description or effect text.

614 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawIndentedTextColumn

Named `sub_28A76` -> `DrawIndentedTextColumn`, called from
`ShowHealingCostPrompt` and unnamed `sub_1A5F6` — a mode-dispatching
wrapper around the plain `DrawStringColumn` and a per-line helper
`sub_28B94`, selecting between several "how many leading lines get
zero indent" wrapping patterns based on `word_328C4` bits. Reads as a
word-wrapped text column with a caller-selectable hanging-indent
style, where `fontOffset` is the per-line indent and the selected mode
controls how many leading lines start unindented before continuation
lines pick up the accumulated offset.

615 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawWordToken

Named `sub_28B94` -> `DrawWordToken`, the per-line helper called
repeatedly by `DrawIndentedTextColumn`: draws one word from a
pre-formatted text buffer, skipping leading spaces, writing
non-space characters one at a time, then skipping trailing spaces.
If that trailing-space skip hits a NUL byte, it resets the text
cursor back to the line's left margin, advances to the next text
row, and decrements the caller's remaining-lines counter — this
word was the last one on a pre-wrapped line. If it hits another
non-space character instead, it returns without decrementing the
counter, so the caller loops back to draw the next word on the same
line. This reveals that the game's message-text buffers are
pre-wrapped into lines at load/format time, with NUL bytes marking
line boundaries — `DrawIndentedTextColumn` and `DrawWordToken`
consume that pre-wrapping rather than computing word wrap from
on-screen width themselves.

616 named of 769 functions as of this update.

### 2026-09-15 session update, continued: RedrawItemDescriptionAndMaterials, DrawMapEditorInteractionTypeOverlay

Named `sub_21C79` -> `RedrawItemDescriptionAndMaterials`, called from
the shop buy handler `sub_17032` and the trade/inventory dispatcher
`sub_1869D`, always immediately after `LoadItemCatalogRecord`: draws
a 3-line text field from `word_2E546`+0x13 (the current item
record's description text, per earlier confirmed uses of
`word_2E546` as the current-item-record pointer) via
`DrawStringColumn`, then refreshes `ShowMaterialCounterHud` and
`DrawMouseCursor`. This is the standard post-item-load redraw in the
shop/trade screens; `sub_1869D` uses it as the default branch when no
barter-pricing preview is needed (the alternate branch instead calls
`ComputeBarterPricingPreview`).

Named `sub_2075B` -> `DrawMapEditorInteractionTypeOverlay`, called
from `RunMapEditorScreen`, gated on `word_328C4` bit `0x400` (a
debug/overlay toggle — returns immediately if clear). When active,
scans a 40x24 grid of map-editor tile positions and calls
`TryInteractAtPosition` per cell, then draws a single debug letter
('N', 'I', 'M', or 'C', or nothing) chosen from `errorCode` and
several other flag words. Read as a map-editor debug overlay
labeling what `TryInteractAtPosition` considers present at each
visible tile; the exact meaning of each letter code is not
independently confirmed.

618 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawGameDialogMenuLabels, DrawMapEditorFloorTypeReadout

Named `sub_1F217` -> `DrawGameDialogMenuLabels`, called once from
`RunGameDialog` (the in-game pause/system menu handler) right after
dialog setup: draws each of the SAVE/LOAD/NEW GAME/DOS/ANIMATION/
RETURN menu labels, each one skipped if its own `word_328C4` bit is
already set (`0x80`/`0x40`/`0x20`/`0x10`/`0x8`/`0x4` respectively —
the ANIMATION case calls `sub_1F884` instead of
`GameDialog_drawAnimation` when its bit is set), then always draws
MUSIC/SOUND FX labels gated on `g_driverStateFlags` bits 1/4, and two
`DrawCheckboxIndicator` calls gated on `g_driverStateFlags` bits 8/2.
Reads as the dialog's one-time initial label draw, where each
`word_328C4` bit suppresses a label that isn't applicable in the
current context; the exact per-bit "why unavailable" reason is not
confirmed.

Named `sub_2047B` -> `DrawMapEditorFloorTypeReadout`, called from
`RunMapEditorScreen`: draws `word_2E386` (the map editor's
currently-selected floor tile type — confirmed by existing comments
as set by the 'F' picker handler and consumed by
`DrawFloorTypeLegendRow`) zero-padded at a fixed top-of-screen
position, the floor-type sibling of the already-named
`DrawMapEditorCoordinateReadout`.

620 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawGameDialogMenuLabelsHighlighted, DrawAnimationSpeedLabel

Named `sub_1F53E` -> `DrawGameDialogMenuLabelsHighlighted`, called
from `RunGameDialog` right after `GameDialog_drawButtons`, both at
initial dialog entry and again after each `SelectGameDialogOption`
cycle: loops 6 times over a fixed table (text + a flags byte per
entry) paired with a position table, drawing each entry's text in a
highlighted color if its flags byte has bit `0x40` set, else the
normal color. This is the selection-highlight redraw counterpart to
the earlier `DrawGameDialogMenuLabels` (which draws the same 6
pause-menu entries once at setup, gated on different `word_328C4`
availability bits) — here bit `0x40` marks the currently
highlighted/selected entry.

Named `sub_1F884` -> `DrawAnimationSpeedLabel`, called from
`CycleAnimationSetting` and from `DrawGameDialogMenuLabels`'s
ANIMATION branch (in place of `GameDialog_drawAnimation`, when
`word_328C4` bit `0x8` is set): draws one of 3 messages at the
animation label position selected by `word_36CE7`'s value (1, 5, or
9). `CycleAnimationSetting` confirms `word_36CE7` cycles through
exactly `{1, 5, 9}` on each activation, so this is the label for a
3-way animation speed/mode setting; the exact text of each of the 3
messages (e.g. slow/normal/fast) was not independently confirmed.

622 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawClueBookMapCategoryHeader, EnforceDemoBoundary

Named `sub_14122` -> `DrawClueBookMapCategoryHeader`, called once
from `RunClueBookMapCategory`: formats a label via `sub_27B84`, loads
a graphic via `FileEntry_Read`, draws it as a banner picture, then
draws the formatted label text next to it, followed by the mouse
cursor — the header draw for a clue book map category page.

Named `sub_1075E` -> `EnforceDemoBoundary` — a genuinely interesting
find. It checks the party's position against one hardcoded
coordinate triple, and if matched (and `word_328CA` bit `0x2` is
clear), shows a 2-line "REGISTER TODAY!" message and blocks the
caller's subsequent `TravelToDestination` call. This is the classic
shareware "you've reached the edge of the demo area, please
register" boundary gate. **However, it is confirmed dead code in
this binary**: `start` unconditionally sets `word_328CA` bit `0x2`
right after `ParseCommandLineSwitches` (`or word_328CA, 2`), and a
search of every `and word_328CA, <imm>` mask in the whole binary
confirms none of them ever clears that bit again — so the `jnz` at
the top of `EnforceDemoBoundary` is always taken and the block can
never fire. A second, related dead branch was found in the same
sweep: at the game's shutdown sequence in `start`, right after
`ReleaseEmsHandles`, the same bit is tested again, and when clear
(never, per the above) prints via DOS `INT 21h AH=9`: "Thank You for
playing Yendorian Tales Book I Chapter 2" / "Please register your
copy today." — also unreachable. Read together, `word_328CA` bit
`0x2` looks like a "registered version" flag that this particular
`SW.EXE` build forces on unconditionally, permanently disabling both
the in-game demo-boundary nag and the exit-time registration
reminder, while leaving the shareware-era code paths themselves
intact in the binary.

624 named of 769 functions as of this update.

### 2026-09-15 session update, continued: SaveUiStateForClueBook, RestoreUiStateForClueBook

Named `sub_14F92` -> `SaveUiStateForClueBook` and its exact mirror
`sub_14E28` -> `RestoreUiStateForClueBook`, a matched pair called
once each from `ShowClueBook`. `SaveUiStateForClueBook` allocates a
scratch memory block (handle kept in `fe`) and serializes roughly 45
global UI/display-state words — video position, font colors, EMS
pointers, the current item/spell/selection state words, and several
`word_328Cx`/`word_3290x`/`word_3293x`/`word_3296x`/`word_3297x` UI
flag words — into it via `stosw`, plus several fixed-size `rep movsw`
copies of other data tables. `RestoreUiStateForClueBook` reads the
same block back out via `lodsw` in the identical order, in
`ShowClueBook`'s cleanup path after the scratch block itself has been
freed and the outer `fe` handle popped back off the stack. Together
they snapshot/restore the UI state that opening the full-screen clue
book overlay would otherwise clobber.

626 named of 769 functions as of this update.

### 2026-09-15 session update, continued: DrawDebugPositionOverlay, DrawResourceCounterPanel

Named `sub_28034` -> `DrawDebugPositionOverlay`, called from a raw
address (`seg000:0AFA`) very early in the binary that IDA never
resolved into a named function — not independently confirmed, but
plausibly a low-level tick hook given how early the call site sits
and how heavily this function is gated against interfering with
other screens. Only runs when a specific debug flag combination
holds (`word_328C6` bits `0x80`/`0x200` clear, `word_328CA` bit
`0x1000` clear, `word_328C4` bit `0x2000` set), then draws 3 rows of
labeled numeric pairs: `'H'`/`'V'` for `word_2E55C`/`word_2E564`,
`'H'`/`'V'` again for `word_36CF7`/`word_36CF9` (the confirmed party
world X/Y position), and `'B'`/`'F'` for a pair read via a
`word_328D2`-indexed table. Reads as a debug HUD overlay; the exact
meaning of the `H`/`V` and `B`/`F` field pairs beyond the confirmed
row being world X/Y was not established.

Named `sub_2714A` -> `DrawResourceCounterPanel`, called from
`start`'s main status-panel redraw path (right after
`ShowResourceDepletedOverlay`) and from `sub_271DC`: draws the status
panel's fully-labeled resource readout at (`0xF0`,`0x60`) — "GOLD
COINS:" + the BCD4 counter at `0x94B3` (confirmed `g_partyGold`),
"MAGIC ORE: " + `0x94B7`, "NUORE: " + `0x94BB` (the same 3 confirmed
global material counters documented in `file-formats.md`'s "Global
material counters" section). This is the detailed, fully-labeled
resource panel shown in the normal status-panel area, distinct from
the compact icon-based `ShowMaterialCounterHud` (gold only, shop
screens).

628 named of 769 functions as of this update.

### 2026-09-15 session update, continued: ShowStatusPanelMessage, DrawClueBookMonsterStatRow

Named `sub_23B19` -> `ShowStatusPanelMessage`, called from `start`'s
main command loop with a caller-supplied message pointer/line count:
sets up the status panel position/colors, clears/restores as needed,
then draws the message via `DrawStringColumn`. Confirmed via string
dump at both traced call sites: "NOTHING HERE" (after a failed
`ProbeFacingTile` search) and "YOU ARE NOT" / "YET READY!" (after a
3-flag quest-gate check fails) — a generic "show an N-line message in
the status panel" utility.

Named `sub_148B2` -> `DrawClueBookMonsterStatRow`, called 7 times
from `ShowClueBookMonsterDetail`, once per bestiary stat row. Draws a
caller-supplied label, then reinterprets the caller's `ax` as a field
offset within the loaded monster record (popped into `bx`): if that
field is zero, the value draw is skipped entirely (stat not
applicable to this monster), otherwise the field's numeric value is
drawn right-aligned at a fixed column. A milestone: **630 named of
769 functions as of this update.**

### 2026-09-15 session update, continued: monster immune/resistant flag rows, weapon skill-type row

Named `sub_1496B` -> `DrawClueBookMonsterImmuneFlagRow` and
`sub_14A5E` -> `DrawClueBookMonsterResistantFlagRow`, siblings of
`DrawClueBookMonsterStatRow`, both called twice from
`ShowClueBookMonsterDetail`: draw a caller-supplied label, then test
the loaded monster record's flag word at a fixed offset (`[si+0x96]`
or `[si+0x98]`) against the caller's `ax` bitmask, drawing "IMMUNE"
or "RESISTANT" (confirmed via string dump) if any bit matches.

Named `sub_23BA4` -> `AdvanceToNextPackedString`, a small generic
utility (also called from `BuildClueEntryText`) that walks `bx` past
the current NUL-terminated string and the NUL itself, to the start of
the next string in a packed string table.

Named `sub_14A87` -> `DrawWeaponSkillTypeRow`, called from
`ShowWeaponDetailRow`: draws "SKILL:" then selects one of 5 packed
strings — confirmed via string dump to be "PROJECTILE"/"SLASHING"/
"BASHING"/"POLEARM"/"CASTING" — by testing `word_2E548`'s `+2` flag
word against `0x8000`/`0x4000`/`0x2000`/`0x1000` in descending order
(walking forward with `AdvanceToNextPackedString` for each bit that
doesn't match), defaulting to "CASTING" if none of the 4 bits are
set. This identifies `word_2E548+2` as the held item's weapon
skill-type classification.

634 named of 769 functions as of this update.

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
