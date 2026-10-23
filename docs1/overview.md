# Yendorian Tales Book I, Chapter 1 — overview / session log

Append-only: correct a stale entry with a new dated note, not a silent rewrite.

## 2026-10-22 — IDA databases created for every executable

### The game directory

`yendor1/game/` is a full install of the shareware Chapter 1 (EGA and SVGA
video drivers present). Unlike Chapters 2/3, which are a single large `SW.EXE`,
Chapter 1 is a set of **20 separate programs**: a small `SW.EXE` launcher and
`PROG0.EXE` … `PROG18.EXE` (plus `INSTALL.EXE` and two video drivers — 23
inputs in all), which are presumably chained/exec'd by `SW.EXE`
(not yet traced — that is the first thing to establish). Data files:
`WORLD.DAT`, `TOWN.DAT`, `MINE.DAT` (256 KB each), `FILE00`–`FILE06`
`.16`/`.PAL` (16-colour graphics + palettes), `CURRGAME/` and `SAVEGAME.A/`
(`AREA.DAT`, `NPC.DAT`, `PARTY.DAT`, and `BATPIC.DAT` in CURRGAME), and the
video drivers `VID16.DRV` / `VIDP16.DRV`. `YENDOR1.TXT` and `REGISTER.TXT` are
the shareware readme / order form.

### Databases

One IDA database per executable, named by the lower-cased stem and kept
directly under `yendor1/` (`sw.idb`, `prog4.idb`, …). Every input except
`VIDP16.DRV` is a 16-bit MS-DOS MZ executable and was loaded with IDA 8.3's
default DOS EXE loader. Function counts are `proc` lines in the freshly
generated listing — nothing has been named or analysed yet, so treat them as
rough.

| Program | Size (bytes) | Funcs | Notes |
|---|---:|---:|---|
| `SW.EXE` | 15,217 | 43 | main startup executable |
| `INSTALL.EXE` | 12,953 | 56 | installer |
| `PROG0.EXE` | 6,120 | 12 | |
| `PROG1.EXE` | 13,676 | 50 | |
| `PROG2.EXE` | 14,598 | 53 | |
| `PROG3.EXE` | 11,128 | 37 | |
| `PROG4.EXE` | 99,387 | 300 | largest program |
| `PROG5.EXE` | 35,685 | 88 | |
| `PROG6.EXE` | 12,480 | 47 | |
| `PROG7.EXE` | 25,423 | 57 | |
| `PROG8.EXE` | 54,287 | 102 | |
| `PROG9.EXE` | 53,627 | 106 | |
| `PROG10.EXE` | 90,624 | 204 | |
| `PROG11.EXE` | 54,181 | 109 | |
| `PROG12.EXE` | 55,030 | 44 | few functions for its size — mostly data? |
| `PROG13.EXE` | 58,490 | 44 | ditto (12–14 look like siblings) |
| `PROG14.EXE` | 64,136 | 44 | ditto |
| `PROG15.EXE` | 45,639 | 106 | |
| `PROG16.EXE` | 12,972 | 58 | |
| `PROG17.EXE` | 12,208 | 43 | |
| `PROG18.EXE` | 15,814 | 56 | |
| `VID16.DRV` | 6,695 | 15 | video driver; MZ format |
| `VIDP16.DRV` | 817 | 1 | flat code blob, see below |

`SW.EXE`'s own database: entry `1000:0` (`start`), md5
`a5faa33223e3db48efd0866b0cf3b18d`, 21 segments (IDA splits an MZ file into
one segment per relocation target), 43 functions, 1 named (`start`).

**`VIDP16.DRV` is not an MZ file.** It begins `E9 C5 02` (`jmp 02C8h`) with no
header at all; IDA's default loader guesses "MS-DOS SYS-file (device driver)"
and annotates it with bogus Strategy/Interrupt routine labels. It is loaded
with the Binary loader instead (16-bit x86, base 0), and
`ida_scripts/seed_vidp16.py` marks the entry code at offset 0.

### Pipeline (`yendor1/ida_scripts/`)

Derived from the `yendor2`/`yendor3` pipeline, with the database name made a
parameter since there are 23 inputs. Run from inside `ida_scripts/` with the
PowerShell tool (IDA 8.3 at `C:\Program Files\IDA Pro 8.3\idat.exe`):

- `create_idb.ps1 <EXE>` / `-All` [`-Force`] — builds the database from
  `game\<EXE>` in one `idat.exe` pass (`-c -o`), then exports `<name>.asm` and
  `<name>.idc`. Refuses to overwrite an existing `.idb` without `-Force`, since
  a database with renames in it is the real work product. Non-MZ inputs get the
  Binary loader automatically.
- `run_ida_script.ps1 <name> <script.py> [-NoExport]` — runs a script headlessly
  against `yendor1\<name>.idb`, then re-exports. `-NoExport` is **not** a dry
  run (same caveat as the other games).
- `batch_run_and_export.py` — the driver; logs to the (gitignored)
  `batch_run_and_export.log`.
- `identify.py` — read-only report; `export_only.py` — no-op target used for the
  initial build; `seed_vidp16.py` — see above.

Nothing in `yendor1/game/` is modified: the databases are written under
`yendor1/`, and `yendor1/game/` is gitignored along with `*.idb`.

### Next

1. Disassemble `SW.EXE` properly (43 functions): establish how it launches the
   `PROGn.EXE` programs (DOS `exec`/overlay, command-line, shared memory?).
2. Identify what each `PROGn.EXE` is (title, town, mine, combat, …) from its
   strings, then name functions per program.
3. Decode the data files (`WORLD.DAT`/`TOWN.DAT`/`MINE.DAT`, `.16`/`.PAL`,
   `CURRGAME` records).

## 2026-10-22/23 — analysis, video layer and emulator

- **Analysis (see `video-api.md`, `program-graph.md`, `file-formats.md`, `strategy.md`):** the `INT 62h` driver ABI is
  documented and every call site in all programs is labelled; the program graph and the parameter-block/handle
  inheritance protocol are recorded; ~1,100 of 1,700 functions are copies of one shared runtime (names propagated with
  `shared_names.json`).
- **`src1/` C++ code:** `Yendor1::Shared`, `Data` (tiles, palettes) and `Vid` (a software EGA transliteration of the driver),
  plus an SDL2 host and tools (`tilesheet`, `vidshot`, `tilebrowser`).
- **`yemu` (src1/tools/yemu):** an 8086 + planar EGA + DOS/BIOS/mouse emulator that runs the original executables on a
  scratch copy of the game directory. The real `VID16.DRV` and `VIDP16.DRV` run inside it as TSRs, so its screen captures are
  what the original produces. `SW.EXE` boots through `PROG0`, the startup message, `PROG1` (logo, title) and `PROG2`.
  Needed DOS details: names are cut to 8.3 (`PARTY.DAT$` opens `PARTY.DAT`), the whole 128-byte command tail is copied to the
  child's PSP, children inherit the first 20 handles, `INT 21h/67h` raises the handle limit, TSR exit keeps memory.
  Usage: `yemu <gameDir> SW.EXE --fonts <dir> --script steps.txt --out <dir>` (script: `wait`, `key`, `ext`, `text`, `move`,
  `moveto`, `click`, `shot`, `quit`; 1 virtual second = 3M instructions). Run it from PowerShell (Git bash cannot find
  the MinGW DLLs and exits 127). Fonts: the BIOS ROM fonts are not game files; `src1/tools/fon2raw.py` makes stand-ins.
- **Verification of `Vid`:** `yemu --compare` runs `Yendor1::Vid::Video` in lockstep with the real driver: at every `INT 62h` the
  emulated state is copied into `Video`, the call is made on both, and EGA planes, displayed page, the context block, image
  buffers and the cursor store are diffed. Over a run through the startup message, the intro, the title screen and the
  prologue (~232,000 calls, 22 functions: pixels, rects, text, image get/put, tile copies, page flips, wipes, mouse
  show/hide/refresh/bounds/position) there are **zero** differences. Not yet exercised by any run: the scroll calls (`0Eh`–`11h`),
  `13h` keyed copy, lines (`05h` only 270 calls, all matched) at other slopes, the `15h`/`17h`/`20h`/`21h` wipes.

## 2026-10-23 — PROG1 and the Sw shell reimplemented and verified

`src/prog1` (the intro/credits program) and `src1/sw` (the shell up to its PROG3/PROG4 dispatch loops) are C++ transliterations
of the disassembly, with shared furniture in `src1/shared` (`Runtime`: file records, input polling, strings, tile draw;
`Ui`: book frame, icon bar and its click handling, the pixel dissolve). Texts are read from the original executables at run
time (`data/ExeImage`), not copied into source.

**How each is verified (the workflow to repeat for every further program):**
1. `yemu <game> SW.EXE --fonts src1/fonts --script steps.txt --dump <ref> --log62 <ref>/trace.txt` runs the original and writes, per
   program start/end, a snapshot (`NN_PROG_start.ydmp`: parameter block, driver context, cursor store, EGA planes) and one trace
   line per `INT 62h` call with the context fields it reads and a hash of any image drawn. Scripts need absolute `moveto`, `click`,
   `key`, `wait`, `shot`. Variants of the game directory (e.g. `PARTY.DAT` byte 25h = 1 or 2) reach other program modes.
2. A test (`tests/test_prog1.cpp`, `tests/test_sw.cpp`) starts the C++ code from the start snapshot (or, for the shell, from a fresh
   machine) and compares every driver call with the trace (`tests/TraceCompare.h`: AL and the drawing colour are masked where the call
   does not read them; the random-order dissolve is compared as an unordered set; trailing mouse refreshes caused by scripted
   movement are ignored) and the final EGA planes with the end/next-start snapshot. Tests skip when the env variables
   (`YENDOR1_REF_DIR`, `YENDOR1_REF2_DIR`, `YENDOR1_REFS1_DIR`, `YENDOR1_REFS2_DIR`) or the game files are absent.

**Results:** PROG1 modes 0/1/2/3 match the original over 223,682 / 223,089 / 595 / 115 calls (modes 0, 1, 3 also compare final video
memory byte for byte); the shell from a fresh machine up to the first program not yet ported matches over 224,845 (new game, to
PROG2), 224,252 (state 1, to PROG3) and 1,277 (state 2, to PROG3) calls, with identical video memory at the hand-over.

**Notes found on the way:** the game's random generator (shared_9c11a6) has short cycles (15, 17, 34, 360, 389, 1023 values), so an
unlucky time seed traps the dissolve for a while. **Correction (PROG2):** the original is not stuck for ever -- the retry counter in
`shared_1f8a83` (`ds:1C76h`, starting at FDA6h) wraps after 602 failed draws and the code next to it (`mov cs:word_134CC, 0`) clears the
generator's seed (that word *is* the seed, DS:1C5Ch), so the next draw is seeded from the DOS clock again. The port does the same
(`Shared::BookUi::random`, `System::dosClock()`). `PROG0` is treated as a no-op
success for now. Unverified (no reference run yet): the shell's end path (`showEndScreen`, PROG16/17/18 hand-overs), the PROG3/PROG4
dispatch loops (they follow the documented exit-code table), and PROG1's click-on-icon path (only the key path was exercised).

## 2026-10-23 -- PROG2 (the prologue) reimplemented and verified

`src1/prog2` is a transliteration of PROG2.EXE: five book pages with story text and animated pictures. Texts (64-byte lines at DS:0A41h,
0CC1h, 0F41h, 11C1h, 1441h) and the animation table (DS:1641h: six chains of 18-byte entries -- tile, x, y, columns, rows, delay,
counter, next, random range; the delays are loop counts multiplied by 10 on a 386-class CPU) are read from PROG2.EXE at run time
(data segment at image offset 1870h). Pages 1 and 2 leave on a timeout (20 s / 5 s on the BIOS clock) with a page curl (8 groups of 16
frames, `curlFrame`) or a pixel dissolve of 64x64-pixel cells (`pixelDissolve`, 4096 passes of 12 pixels); pages 3-5 animate until the
"More" icon is clicked (or its key, `M`, is pressed). Esc skips the whole prologue when SW passes parm_03 = 1. `shared_21818f` (the
wait-for-input routine) is now ported in full as `BookUi::input(keyMask, clickMask, flags)`; sound cues (`shared_728316`) and the
party-record preload (`shared_d2992b`) have no visible effect and are omitted.

**Verification** (`tests/test_prog2.cpp`, reference run `YENDOR1_REF2_DIR`): all 110,415 driver calls match and the final video memory is
identical. What had to be recovered from the trace because it comes from the outside world: the generator's seed (7943, found by
searching all 16-bit seeds against the first 40 dissolve passes), its state at the start of page 3 (8817, found by simulating the
animation counters against the order of draw events), and when each page's wait ended (the test delivers the click once the port has
made as many driver calls as the original had when its click arrived). The dissolve's pixel *order* depends on clock readings at each
reseed (see the correction above) and was only recovered for the first one, so only the set of pixels and not the coordinates left in
the context by the last pixel are compared. The Esc-skip path (parm_03 = 1) is not covered by a reference run.

## 2026-10-23 -- PROG3 (the book menu) reimplemented and verified

`src1/prog3`: the table of contents with its nine entries (Create, View, Remove, Assemble party members, Open book, Bookmarker, About
the Authors, Introduction, Close). Up/Down/Enter or a click choose an entry (hit boxes at DS:11EBh); the exit code is the entry
number (7 About, 8 Introduction, 9 Close -- or 1 when the game has not started yet, parm_0D != 2 --, 5 starts the game).
Entries 1-4 and 6 start PROG5 / PROG6 through the new `Shared::Launcher` interface (the child gets its own copy of the parameter block;
`Sw::Launcher` is now a typedef of it). The conditions come from record 0 of CURRGAME\PARTY.DAT: word 0 is a record count (0Ah = no
characters, more than 1Bh = no more new ones), word 26h non-zero = a party has been assembled; otherwise a boxed one-line message
("There are no players to view", ...) is shown until a key or click. "Close the book" opens the shared "are you certain" panel
(`BookUi::openConfirmPanel/closeInfoPanel`, shared_1d0ebe: it saves the bottom-right strip with vidGetImage and restores it with
vidPutImageHdr). `BookUi::drawBox` is shared_23f116. `Runtime::workDir()` tells programs where CURRGAME lives.

**Verification** (`tests/test_prog3.cpp`, script `tests/refs/prog3_menu.txt`): all 1,012 driver calls and the final video memory match for a
run that exercises the Open book / View / Remove / Assemble messages (keyboard and mouse), the Close confirmation (cancelled with Esc)
and Introduction. New test tooling: `tests/ScriptedInput.h` (an `InputScript` that plays keys and mouse moves/clicks one step per poll,
recorder, video diff, context printing); yemu now writes `# end <program>` markers to the trace and `TraceCompare::sliceAfterEnd`
reads what a parent does after a child returned. Not covered by a reference run: the child launches (entries 1-4, 6; this game has
no characters, so only Create would start PROG5), the first-launch variant (parm_03 = 0 uses the right-to-left wipe, which the
menu shown by the new-game run in `ref2` exercises only up to its first click), and exits 5 and 9.

## 2026-10-23 -- PROG6 (the Bookmarker) reimplemented and verified

`src1/prog6`: the save/restore popup (3 entries from the book menu, 4 or 5 in a running game depending on parm_03/parm_05/parm_12), with
the shared confirmation panel, Esc icon, mouse and keyboard selection (a click on an entry selects *and* activates it). Saving and
restoring are done by PROG0 (modes 2 and 3, parm_10 = byte 21h of PARTY.DAT record 0) through `Shared::Launcher`. `BookUi::drawFrame`
(shared_0bb58f) is now shared with PROG3. Not ported yet: `drawPartyStrip`, the shared character renderer PROG6 calls after a restore for
each party member (it will come with PROG5); the restore path is therefore only verified up to the PROG0 hand-over.

Verification (`tests/test_prog6.cpp`, script `tests/refs/prog6_bookmarker.txt`): two runs match call for call and in video memory (280 calls:
navigation, RESTORE?/SAVE? confirmations cancelled by key and by the Esc icon, Esc exit; 206 calls: Enter, confirm, hand-over to PROG0).

## 2026-10-23 -- PROG0 (the saved-game copier) reimplemented and verified

PROG0 never draws with the video driver (DOS text screen + files), so `src1/prog0` is verified by the files it leaves behind. It copies
CURRGAME\{PARTY,NPC,AREA}.DAT to or from SAVEGAME.<x>\ : the number of PARTY records is the first word of record 0, there are always 446
NPC records, and the number of 128-byte AREA records is the word at offset 4 of NPC record 0 (so a save whose area count is below the
file's leaves the rest of AREA.DAT as it was). Mode 1 (SW.EXE, game not begun) lists the saved games and asks for a number when there is
more than one (text via the new `System::textOut`/`readLine`, default no-ops; the DOS directory order is approximated by probing letters),
exit codes 21h (nothing saved), 20h (no party chosen), 1/3 (CURRGAME file will not open), 9/0Ah/0Bh (saved game's file will not open), FEh.
The inherited DOS handles of modes 2/3 (parm_5C, 76, 78) have no counterpart: the port opens the files itself. Note for the shell: with
PROG0 real, every start of an unfinished game overwrites CURRGAME with the saved game, as the original does (`test_sw` now supplies a
SAVEGAME.A patched like CURRGAME).

`tests/test_prog0.cpp`: the three modes produce byte-identical files to the original's (yemu runs of the original on a game directory whose
current and saved games differ; the scripts are in `tests/refs/`), and the error codes 9 and FEh. **Housekeeping:** `src1/tests/test_sw_work/`
(copies of original game data written by test_sw) had been committed by mistake; it is now untracked and ignored (`.gitignore`), but it
remains in the history of commits before this one.

## 2026-10-23 -- PROG5 (the character screens) reimplemented and verified

`src1/prog5` ports the four modes the book menu starts: Create (parm_0E = 1), View (2), Remove (3), Assemble party (4), as the original
does them: the program works on a 64 KB data segment loaded from PROG5.EXE (`Shared::DataSegment`; the initial tables -- class ranges, item
table, sprite tile tables, texts -- are read from the file at run time), with the whole PARTY.DAT (10 records of 100 bytes, characters from
record 10 on, record 0's first word = record count) held at DS:05F7h and the character being worked on at DS:09DFh. Names of the offsets
are in `prog5/Prog5Layout.h`. Pieces worth knowing:

* **The character renderer** (`drawPartyStrip` and its helpers) composes a character from body, head, shield and weapon tiles of the
  character's picture file off-screen, then copies the 5x4 tiles to CURRGAME\BATPIC.DAT -- tile "file" 7 of the tile cache is that file
  (`Runtime::readBatpic/writeBatpic`; `drawTile(7, n)` draws record n). tileOp's six operations are: 1 draw (load if not cached), 2 load,
  3 release, 4 get a pointer, 5 invalidate.
* **Party assembly** keeps six slots (words at DS:061Dh); a character's `flags` byte has 01h = in the party, 40h = portrait needs rendering,
  80h = has had the starting supplies (50 of item 1 and a few class items are added to the party stock at DS:06BFh, indexed by item number).
* Attributes are rolled uniformly between per-class minimum and maximum (tables at DS:160Bh); the shared RNG is the one in `BookUi`.
* Money is four BCD bytes at DS:09FCh with the original's quirky decimal correction (`Prog5::money`, flags emulated).
* Stock, status and the "in the party" checks mean View lets the player change weapon/armour/shield of party members (Left/Right through
  the items they can use, Enter keeps).
* `byte_1481F` is three things at once in the original -- the last input result, a file error flag and the screens' cancelled flag; the port
  keeps one `_status` the same way.

**Verification** (`tests/test_prog5.cpp`, scripts in `tests/refs/prog5_*.txt`): five runs match the original call for call, in video memory and in
the party files left behind (including BATPIC.DAT): create 926 calls, assemble 476, view 1,806, remove 1,504, remove-assigned 211. The
attribute rolls of the create run needed the generator's state: 6157, found by searching all 16-bit states for the two rolls shown on the screens.

**Not ported:** the variants started from the game (parm_02 = 1 or 2: choosing a member, the party rearranging UI with its page-flip
animation, `sub_10F8D`'s notice that a dead leader cannot lead) -- PROG5 returns FCh for them; they come with PROG4. `drawPartyStrip` of
PROG6 (used after a restore) is another copy of the same renderer with its own data offsets; it still needs to be connected to the port in
`Prog6`.

## 2026-10-23 -- the large programs are translated mechanically (asm2cpp)

PROG4 (14k lines of assembly), PROG10 and the shop/conversation/cutscene programs are too large to transliterate by hand with the
confidence the hand ports have, so `src1/tools/asm2cpp.py` translates the IDA export (`yendor1/progN.asm` + `.idc` + the exe) into C++
(`src1/gen/ProgNGen.{h,cpp}`, regenerate everything with `tools/gen_all.sh`). The output is deliberately a literal rendition of the 8086
code, not readable game logic: one member function per routine, C++ labels and `goto`, the register file, the flags (`alu::` helpers set
exactly the flags the original sets), the stack and `seg:off` addressing all kept, running on `Shared::Machine16`. Machine16 provides:

* the 64 KB data segment (loaded from the exe like `DataSegment`), the parameter block the parent would give (command tail / `parm_XX`
  fields, magic word, inherited file handles numbered by their parm offset), the video context block as ES (mapped onto the live
  `Vid::Context`, so the ES: fields and the driver always agree), the code segment as read-only bytes where needed;
* the INT 62h driver calls (straight into `Vid::Video`, the image pointer ES:16h:18h bridged onto Machine16's memory), the tile store the
  driver leaves for the programs (ES:0 slots, ES:2 the slot table, 200 slots with 512-byte buffers in the program's memory, so the
  programs' own `tileOp`/`tileLookup`/`tileLoad` run translated, composed tiles included), the palette mailbox natively, and the DOS/BIOS services the programs use (file I/O, EXEC through `Shared::Launcher`, time, keyboard
  and mouse polling through `Shared::System`);
* `ProgramExit` for INT 21h/4Ch; `unsupported()` for lines the translator could not handle (it reports and stops the program).

`runGenerated<Gen::ProgNGen>()` (shared/GeneratedProgram.h) is what the shell's launcher calls for the programs without a hand port.

Three things the IDA export needed before the translation was complete: (1) *code reached only through jump tables* has no label in the
export; `ida_scripts/label_orphans.py` names every instruction that is not reached by falling through (`loc_XXXXX`), and the translator
builds each `jmp reg` into a switch over all labels of the same segment whose offset occurs as a word in the data segment image or as
an `offset` operand; (2) functions IDA ended early ("endp ; sp-analysis failed") leave their remaining code outside any function; the
translator appends it to the preceding routine; (3) a few blocks are still undecoded bytes (`db`) in PROG18 -- they are skipped with a
warning and need `make_code` in IDA before PROG18 can be verified.

**Verification** is by the same call traces: `tests/test_prog4.cpp` runs the generated PROG4 from the start snapshot of a yemu run and
compares every driver call with the original's. Make the references reproducible with `yemu --clock 4660` (INT 21h/2Ch then returns DX =
0x1234 always, so the RNG seeds are the same in the port, which the test's `dosClock()` answers). `yemu --logpc <hex image offset>`
(optionally `--tracewin N`) logs the registers and NPC record at every execution of an address into the trace, and was how the missing
jump-table targets were found (compare label sequences of the original and the port with the generated code compiled with a `TR()` macro
after each label). The idle tavern scene (490 calls including the NPCs' random walks) now matches exactly.

Things the first PROG4 scenarios taught (all fixed in the translator/runtime): a `proc far` called with a plain far call pushes two words
(cs and ip), the translated `retf` pops four bytes, so the caller must push both unless it did `push cs` itself (a frame-based routine
like `formatNumber` printed wrong digits); the tile cache must be the original's, because the program fills slots itself with pictures
it composed (tile "file" 0Fh); PROG5 started from the game (parm_02 = 1/2) and PROG6 run through the generated code until they are
ported. `keys` (5,864 of 5,884 calls) and `walk`, `idle` match call for call through PROG4, 11, 5, 6, 8 and 7; the last 20 calls of `keys`
differ (an NPC takes a step in the original that does not in the port; the NPC records and random seeds were equal at every visit
before it, so something outside them differs -- not yet found).


## 2026-10-23 -- verifying the other translated programs: running them in place of PROG4

Playing the original to every program is slow, so `test_game` also runs a game directory in which `PROG4.EXE` is replaced by another
program (`YENDOR1_SWAP=n`; the reference run is made with yemu on that directory, `tests/refs/make_game_ref.ps1`): SW.EXE starts it where
it would start PROG4, with SW's parameter block. The programs run with whatever input a script gives them, which is enough to compare the
port with the original call for call. Results (scripts `swap_convo.txt`, `swap_generic.txt`): PROG12, 13, 14 (conversation, typed words),
PROG9, PROG16 (the Great Hall cutscene with its dissolve), PROG17, PROG18 (casino rules screens) match completely; the three-character
`keys` run through PROG4 and its children matches 7,842 of 7,842 calls from the start of SW.EXE. PROG10 and PROG15 started this way
diverge, and not because of the translation: with parameters PROG4 never gives them they take paths the original itself only survives by
accident (PROG10's setup loop `retn`s into whatever is on the stack, PROG15 reads uninitialised data); they need a real route to their
doors (PROG15: a shop entrance in a town; PROG10: PROG4's exit code 3) and are the next thing to cover.

What the replay needs to be exact (all in `tests/ReplaySystem.h` and the translated runtime): keys are given at the poll the original read
them (see above), the BIOS tick values the original read (INT 1Ah) are replayed from a sparse tape per program (`T` lines in
`trace.txt.events`), and a translated program that loops without calling the system (the original loops forever too when given
parameters it was not written for) is stopped by the platform through a watchdog at every jump back (`Machine16::loopTick`).
`Shared::System` got `pollKeyFor`, `ticksFor`, `programStarted/Ended` for this; real systems answer them with `pollKey()`/`ticks()`.
The shell now runs PROG6 through its translation as well (the hand port lacks the party strip a restored game redraws).
