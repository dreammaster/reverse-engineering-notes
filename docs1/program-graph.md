# Program graph and inter-program interface

Chapter 1 is a set of DOS programs run one at a time by the shell `SW.EXE`.
This document records what each program is, who launches whom, how state is
passed between them, and the exit-code protocol. Everything here was read from
the disassembly on 2026-10-22; items marked *(inferred)* are not yet confirmed.

For the video driver all programs share, see [`video-api.md`](video-api.md).

## What each program is

Roles come from each program's strings, its `INT 21h`/`INT 62h` usage and who
launches it (`ida_scripts/program_survey.py`, `annotate_exec.py`).

| Program | Size | Role |
|---|---:|---|
| `SW.EXE` | 15 KB | Shell: loads the drivers, opens the data files, runs the other programs in a loop (below). Also shows the shareware "thank you / register" end screen. |
| `PROG0` | 6 KB | Save-slot handling: lists `SAVEGAME.*` directories ("Following Parties Found", "Select Party to be active"), copies game files between a save slot and `CURRGAME\` ("Copying game files"). No video driver calls (text mode). Mode is passed in the parameter block. |
| `PROG1` | 14 KB | "About the Authors" credits screen. |
| `PROG2` | 15 KB | Prologue / introduction story text (backstory of Yendor, the Athaneum, Nuore ore, the monster raids). |
| `PROG3` | 11 KB | The main menu, styled as a book: Table of Contents with *Chapter I Character Information* (Create / View / Remove), *Chapter II Party Information* (Assemble party members, Open book = **Play**, Bookmarker = Save/Restore), *Chapter III Other Information* (About the Authors, Introduction, Close the book = DOS). |
| `PROG4` | 99 KB | **The main game**: exploration (movement, rest, town guards/jail, passwords, NPC triggers), 300 functions. |
| `PROG5` | 36 KB | Party assembly and character sheet (name, class, weapon/armor/shield, attributes, health/magic, status conditions: asleep, wounded, near death…). |
| `PROG6` | 12 KB | In-game / book menu: save and continue, save and return to DOS, restore, return without saving, make camp or rest at an inn. |
| `PROG7` | 25 KB | Item list / inventory screen (shares the item-name table below). |
| `PROG8` | 54 KB | Weapon/armor shop (Required Strength/Dexterity, Absorption rating, Price). |
| `PROG9` | 54 KB | Item/shop-type screen sharing the item-name table. |
| `PROG10` | 91 KB | A second large engine, 204 functions, with 73 cursor-mode calls; shares the item-name table. Launched by `SW.EXE` on exit code 3 of `PROG4`; likely the mines/dungeon *(inferred; `MINE.DAT`)*. |
| `PROG11` | 54 KB | Item-related screen (item-name table). |
| `PROG12`/`13`/`14` | 55–64 KB | NPC conversation: a question/keyword → reply table (topics such as the Athaneum, mines, weapons, geography). **Identical code, different data** — three conversation sets, `PROG4` picks one. |
| `PROG15` | 46 KB | Item-related screen (item-name table). |
| `PROG16` | 13 KB | Story cutscene: the Great Hall presentation, Zamora presents the orb, then is struck down. Followed by a closing screen. |
| `PROG17` | 12 KB | Story cutscene: the orb is returned, Zamora revives, the Society of Wizards judges Paltivar. |
| `PROG18` | 16 KB | Casino: the card games "21" and "High-Low" with rules screens. |
| `INSTALL.EXE` | 13 KB | Installer/configuration (drive, game copies; runs `PKUNZIP.EXE`, which is not shipped, and loads `VID16.DRV` for its menus). Not part of the game. |
| `VID16.DRV` | 7 KB | Resident video/mouse driver (`INT 62h`). |
| `VIDP16.DRV` | 1 KB | Resident palette helper hooking `INT 1Ch`/`INT 10h` *(inferred)*. |

Item-name table: `PROG7`, `PROG9`, `PROG10`, `PROG11`, `PROG15` each embed the same list
(None, Gold Coins, Coins in a Chest, Processed Nuore, Nuore, ores, Arrows, Bolts,
Deed to a horse, Magical Horse Shoe, Mine Cart, World Maps, shields, armor …).

## Who launches whom

`annotate_exec.py` names each database's EXEC helper `execProgram` and comments
every call with the target.

```
SW.EXE ──► VID16.DRV, VIDP16.DRV      (or the *256 pair, absent in this install)
       ──► PROG0   party/save-slot selection and save/restore file copy
       ──► PROG1   credits
       ──► PROG2   prologue
       ──► PROG3   book menu ─────► PROG5 (party), PROG6 (save/restore menu)
       ──► PROG4   main game ─────► PROG5, PROG6, PROG7, PROG8, PROG9, PROG11,
       │                            PROG12/13/14 (conversation), PROG15
       ──► PROG10  second engine ─► PROG5, PROG7, PROG8, PROG11, PROG15
       ──► PROG16, PROG17          story cutscenes
       ──► PROG18                  casino
PROG6 ─► PROG0     (to perform the actual save/restore copy)
```

Programs other than `SW.EXE` return to their parent by exiting; only `SW.EXE`,
`PROG3`, `PROG4`, `PROG6`, `PROG10` (and `INSTALL.EXE`) call EXEC.

## Process model and state hand-off

Each program is a separate DOS process that exits when finished; `SW.EXE`
stays resident underneath. Nothing is shared by memory pointers. The
interface has four parts:

1. **Exit code** (`INT 21h/4Dh` in the parent) — the edge label in the graph.
2. **A 127-byte parameter block in the child's command tail** (PSP:`0080h`–
   `00FFh`). `SW.EXE` builds it at `DS:0006` (`execProgram` passes
   `AX=6` as the tail offset) and every child copies it at startup
   (`rep movsb`, 7Fh bytes). Slot names in the databases are `parm_XX` (XX = offset
   in the block; see `ida_scripts/tail_usage.py`, output `tail_*.txt`):

   | Offset | Writer → readers | Meaning |
   |---|---|---|
   | `+02`, `+03` | SW, PROG3, PROG4, PROG10 → most | small mode/state bytes (`+03` is set 12 times in SW before launching: the current screen/state selector) *(inferred)* |
   | `+04`…`+0C` | PROG4, PROG10 → PROG8, PROG11, PROG15 | selection parameters for the shop/item screens *(inferred)* |
   | `+0D`, `+0E`, `+10`, `+12` | SW/PROG3/PROG4/PROG6 → PROG0, PROG5, PROG6, PROG12–14 | command and mode words; `+0E` selects PROG0's operation (1 = pick a party, 3 = copy files for a save/restore) and `+10` is read 42 times in `PROG4` *(inferred)* |
   | `+58` | SW → all | magic `1234h`; **every child exits with code `FFh` if absent** |
   | `+5A` | SW → all | segment of the video driver's context block (`es` for all `int 62h` field writes) |
   | `+5C`…`+6A`, `+70`…`+7A` | SW → all | DOS file handles of the data files SW opened (see 3) *(inferred)* |
   | `+6E` | SW | not referenced by the children |

3. **Inherited open files.** `SW.EXE` raises its handle limit (`INT 21h/67h`
   to 64) and opens the game's data files once (`FILE00.16`…`FILE06.16`,
   `CURRGAME\PARTY.DAT`, `NPC.DAT`, `AREA.DAT`, `BATPIC.DAT`, `WORLD.DAT`,
   `TOWN.DAT`, `MINE.DAT`, plus the `.PAL` files). Each child grows its own handle
   table to 64 and copies the parent's (reading the JFT pointer from the
   parent's PSP: `mov bx, es:16h` = parent PSP), then takes the handle numbers
   from the parameter block into its own file descriptors. A file descriptor
   is a small record: `[+0]` handle, `[+2]` buffer offset, `[+6]` record size,
   `[+8]` record number, `[+0Eh]` ASCIZ name; the shared record API is
   open (`AL`=mode), seek-to-record, read-record, read-to-segment, write-record.
4. **Persistent state in files and driver memory.** `CURRGAME\*.DAT` hold the
   running game (party, NPCs, area); the video driver's context block and tile
   cache live in memory owned by `SW.EXE` and outlast every child.

The parent also overwrites its own PSP fields (`es:16h`, `es:18h`) after every
child returns — a quirk of the video image pointer (ctx `16h:18h`), not part
of the protocol *(inferred)*.

## SW.EXE state machine

`start` initialises, then runs this loop (addresses are `sw.idb`):

1. Shrink memory, raise the handle limit to 64, zero the parameter block.
2. Open the data files (`sub_105AF`/`sub_1122C`); on failure exit with the
   index of the failing file in `byte_12877` and print an error from the string
   table (`aErrorsHaveBeen…`, "Problems with loading a party", "Please run the
   install program first", "The required VESA driver is not loaded", …).
3. If not already in the "party chosen" state (`byte_11A4B != 2`), run **PROG0**
   with mode 1 (choose the party). Exit codes: `0` ok; `20h` "No party was
   selected" → quit; `21h` "Please run the install program first" → quit with
   `FFh`; anything else prints "Problems with loading a party" and exits with
   that code.
4. Load the party record, start the video driver (`sub_10873`: EXEC
   `VID16.DRV`, `INT 62h` fn00, EXEC `VIDP16.DRV`) — or the 256-colour path
   (`sub_108AC`) when the party's video setting is 2 — and measure CPU speed
   class (`sub_10B08` → `ctx+8/0Ah/0Ch` delay counts; 8088/286/386+).
5. Show the credits (**PROG1**) unless skipped, then the **prologue**
   (**PROG2**) the first time (`byte_11A4B == 0`).
6. **Book menu loop** — run **PROG3**; its exit code:

   | Exit | Action |
   |---|---|
   | 9 | copy the screen to page 1, then the **end path** (shareware end screen, fade, exit) |
   | 8 | replay the prologue (PROG2) then back to the menu |
   | 7 | credits (PROG1) then back to the menu |
   | 5 | **play**: enter the game loop (7) |
   | 1 | end path |
   | other (incl. 0) | store the code as the DOS exit code and quit with the standard message |

7. **Game loop** — run **PROG4**; its exit code:

   | Exit | Action |
   |---|---|
   | 0 | back to the book menu |
   | 1 | end path (shareware end screen, then exit) |
   | 2 | run **PROG16** (Great Hall cutscene), then the closing screen and exit |
   | 3 | run **PROG10** (second engine); its exit `1` → run PROG0 (mode 3 = save/restore copy) and resume; otherwise resume PROG4 |
   | 4 | run **PROG0** (mode 3) to copy game files, resume PROG4 on success |
   | 5 | run **PROG17** (Zamora recovers cutscene), closing screen, exit |
   | 6 | run **PROG18** (casino), then resume PROG4 |
   | other | quit |

8. End path (`loc_10265` → `sub_103AB`, `sub_10984`): show the end screen, fade the palette, release the tile cache and driver memory
   (`INT 62h` fn01), restore text mode, print the closing message, exit with
   the stored code.

The shell's own screens: a **shareware end screen** (`sub_103AB`: a thank-you
and registration appeal built from tiles and text, shown on the end path) and
two short **closing messages** used after the PROG16/PROG17 cutscenes
(`sub_10A46`, `sub_10984`).

## Common runtime in every program

`SW.EXE` and the children contain copies of the same small runtime (different
addresses, same code), which the reimplementation should provide once (in
`Yendor1::Shared`, after confirming each copy matches):

- **File record API** — open, seek, read, read-to-segment, write over the
  descriptor records described above (`sub_1122C`, `sub_1124F`, `sub_11276`,
  `sub_112A0`).
- **Tile cache** (`sub_1174C` and its table of operations at `DS:1DC5h`): a
  32x32 planar tile (512 bytes) is identified by a file number (`byte_1287A`,
  0–6 = `FILE00`–`FILE06.16`) and a tile index (`word_12878`). Operation `AL`:
  0..5 (draw; draw-with-key; mark free; probe; …). `sub_118BD` looks the pair up
  in the 200-slot directory (`ctx+2`: entries of `{used:byte, type:byte,
  index:word, segment:word}`), `sub_11957` loads the missing tile from the
  file into a free slot. Tiles are drawn with `vidPutImage`.
- **Input polling** (`sub_1134E`): `INT 21h/06h` keys (letters upper-cased,
  extended keys flagged), else the mouse event bits in `ctx+953h` (left press →
  Enter `0Dh`, right press → Enter variant), then `vidMouseRefresh`.
- **Timing**: BIOS tick delay (`sub_111F8`, `INT 1Ah`), CPU-class detection
  (`sub_111BC`), PC-speaker tone on/off (`sub_11724`/`sub_11741`), text-string
  draw with the mouse hidden (`sub_11678`), palette fade in/out for the VGA path
  (`sub_114B2`, `INT 10h`/VGA DAC ports).
- **Random numbers** (`sub_116F2` region: a time-seeded generator).

## Consequences for the reimplementation

- `Yendor1::Sw` is a loop over `Yendor1::ProgN::run()` calls returning an exit
  code, exactly as above; the "command tail" becomes a plain struct passed to
  `run()` (the magic, ctx segment and file handles disappear).
- Because the programs share state only through files and that small block,
  each `ProgN` can be implemented and tested independently against the real
  `CURRGAME` files.
- `PROG0` (text-mode file shuffling) and `INSTALL.EXE` need no video driver
  and are mostly host-file operations; in a ScummVM port they become the
  save/load dialog.
