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
