# Host for the lifted Companions of Xanth game

Runs the *original* game code (lifted from `xanth.asm` to C++, see `../README.md`) on the *original* data, on an emulated PC: the lifted program is the whole
game (startup, C runtime, overlay manager, engine, game logic), and the host only emulates what sits underneath it - DOS, BIOS, EMS, the mouse driver and the
hardware ports - not the engine.

```
bash engine/host/build_host.sh <scratch dir>        # from xanth/: image + symbols from the IDB, lift, compile  ->  engine/host/host_test.exe (about 3 minutes)
engine/host/host_test.exe game game_out/image.bin reversing/analysis/xanth_objects.tsv <command>
    objects | scan | look [room] | rooms | init | clock | text | save | room [n]
```

Diagnostics (environment variables): `HOST_TRACE_DOS` (every int 21h), `HOST_TRACE_INT`, `HOST_TRACE` (every engine call; `room` prints them), `HOST_SHOT=file.ppm`
(screen at the end), `HOST_SHOT_TEXT=file.ppm` (screen when the game first prints text), `HOST_MAIN` (run `main` instead of `init_game`), `HOST_KEYS=n` (queue n Enter keys),
`HOST_LABELPFX=prefix` / `HOST_WATCH=label` / `HOST_MEMWATCH=off[,HOST_MEMWATCH_SEG=seg]` / `HOST_DSWATCH=ds` / `HOST_NZ=label` / `HOST_SRC=label` (register and memory watches on
the lifted code's labels - they found the bugs listed under "What the host found").

## What is in it

* **Memory image** (`game_out/image.bin`, 585,249 bytes, written by `reversing/scripts/dump_image.py`): the flat layout of the IDB (linear address =
  segment * 16 + offset) in a 2 MB host memory. It is the initial state of the executable (compiled-in tables, data segment initial values); `SS = DS` is the data
  segment like a DOS small-model program; the stack top is `FFF0`.
* **Lifted code** (`gen/`, generated, git-ignored, 2,837 functions / 138,000 instructions = every function of the program except the eight below): the entry points `main` and
  `init_game`, all 448 object handlers (including the eight code chunks 1, 27, 30, 45, 86, 177, 180, 335), exit/event/conversation functions, the verb handlers, the object model,
  `run_command`, `go_to_room`, the text decoder, the picture decoder and blitters, fonts, the conversation menus, the C runtime (`start`, `__cinit`, `fopen`, `setjmp`, `malloc`, ...),
  the RTLink startup/shutdown code, the timer interrupt handler, and the code chunks the game stores as far pointers (the 100 region-visibility callbacks `dd loc_xxxx`).
  `lift_handlers.py --host --every` builds it. Calls through pointers in game memory (`call_object_handler`, video-mode tables, driver vectors) go through `farHook`, which maps the
  far address - or an RTLink stub - to the lifted function; a vector that still points at the host's `IRET` is a no-op.
* **`longjmp`**: `_longjmp` throws a C++ exception; functions that call `_setjmp` (`main`'s command loop and `serialize_state?`'s I/O error exit) catch it, restore the
  registers from the jmp_buf and continue after their `_setjmp` call, exactly as the C runtime would. A jump to a buffer whose function is not running (a test calling
  `save_game?` directly instead of through `main`) ends that call and is counted in `Host::stray`.
* **The PC** (`host_machine.cpp`):
  * DOS `int 21h`: open/create/read/write/seek/close on the game folder (files the program creates, like `restart.dat` and the save games, live in memory), memory allocation
    (48h/49h/4Ah), get/set vector (25h/35h, kept in the IVT at 0000:0000), date and time (the clock advances), terminate (throws `GameExit`).
  * BIOS `int 10h` (video mode), `16h` (keyboard queue `Machine::keys`), `1Ah` (tick count); mouse `int 33h` (position/buttons from the script `Machine::mouseScript`);
    EMS `int 67h` (the "EMMXXXX0" device header, page frame at E000, 16 KB pages in host memory).
  * Ports: PIT, PIC, VGA DAC (`3C8/3C9` writes are captured in `Machine::palette`), `3DA` retrace toggling so the vsync waits finish, everything else reads 0xFF.
  * Timer: every 1024 labels the host delivers IRQ 0 through the program's own handler (`INT 08h` or `INT 1Ch`, whichever the game installed - `timer_tick_fn` does the
    palette fades and the clock).
  * `LEGEND.INI` (GAMEDATA / SAVEDATA / MOUSE / MUSIC / SOUND) is supplied by the host.
* **Host functions** (`host_impl.cpp`; the only functions of the program that are not lifted): `_sprintf` (the C library's variadic formatter), `__int86` (runs the interrupt on the
  machine), `_longjmp`, the RTLink overlay manager (`rtlink_countFlags`, `rtlink_loadSegment`, `rtlink_openFile2`, `rtlink_thunk`: everything is already in memory, so "loading" always
  succeeds) and `sub_676B6`. **`sub_676B6`** is the helper of the three big cutscene routines (`sub_67E2A`, `sub_69FF7`, `sub_6C42D`): its ten far calls go to `421E:008B`, which is the
  *middle of an instruction* inside `sub_421E0` (`DB C4 36 ...`), so the call target in the flattened image is wrong (most likely an RTLink overlay thunk whose address range is occupied by other
  overlay code in the flattened image; the arguments - a file number, three pointers, a flag - look like a call to a file loader). The host records the call and reports success. In addition, `Font_WriteMessage`,
  `show_text_box`, `wputchar?`, `Font_WriteString`, `ResourceText_GetStr` and `realsound_play` have *recording overrides* so the tests can read the text instead of the pixels (the lifted
  versions exist too, `text` runs the original `ResourceText_GetStr`).

## What was checked (all against the original game files)

| test | result |
|---|---|
| `init`: the original startup `init_game` | runs to completion on the emulated PC: reads `LEGEND.INI`, opens the data files, sets video mode 13h, installs the timer handler, creates and re-reads `restart.dat` (a 17,964-byte snapshot of the initial state) |
| `text`: `ResourceText_GetStr` (the original decoder) vs `MessageFile` (the C++ port) | 5,542 strings, 0 mismatches |
| `save`: the original save code writes `restart.dat` after the state was changed, then the original load code reads it back after a reset | 5 bytes differ from the initial snapshot; room and the desk's location are restored; the load ends in one `longjmp` to the main loop. **The IDB names are swapped**: `load_game?(1)` writes and `save_game?(1)` reads (mode 1 = `restart.dat`, 2 = a numbered slot, 3 = memory snapshot, 4 = `HELP.DAT`) |
| `room 34`: `go_to_room` with the original room-entry code | runs through `describe_room?`, `RoomItem_Draw` (13 item images through `PictureDecoder__start`), the text and the music call without an invalid argument or an unlifted call. **The pixels are not right yet**: the screen stays black and the item images come out almost empty (a 68x20 image with 2 non-zero pixels), so the picture decoder path (`PictureDecoder__start` / `sub_14FB9` / `sub_16804`) does not yet produce the C++ port's output |
| `objects`: lifted `object_has_attr?` / `object_get_location?` / `object_get_prop_a?` / `object_parent_is?` vs the object table decoded independently by `xanth_objects.py` | 11,454 attribute checks, 414 locations, 484 `prop_a` values: 0 mismatches |
| `look`: `run_command(LOOK_AT, obj)` through the real pipeline | the right descriptions, e.g. the desk listing its contents with the original list formatting |
| `scan`: 492 objects with handlers x phases 1-6 x 16 verbs = 47,232 scenarios | 1,300 print text (998 distinct messages); **0 invalid arguments** in 4,480 checked calls (every object / attribute / verb / event / conversation id passed to the object model, `go_to_room?`, `schedule_event?`, `Conversation_Show` is in range); 77 scenarios start a conversation and stop at its menu |
| `rooms`: enter each of the 70 rooms with `go_to_room`, LOOK_AT everything in it (215 commands) | **all 70 rooms entered** (6 print text on entry); 213 of 215 looks print text; 229 distinct messages; one invalid argument in 3,562 checked calls: entering Dark Cavern calls `object_move?(0, 0x5B, 0)` with the companion object 0 (the state has no companion) |

The sweeps use the image's initial state with one synthetic command per scenario, so they exercise every branch that state allows, not the game's story.

## What the host found in the lifter (all fixed)

Running the game code, not just checking its traces, found problems the trace test cannot see: `cs:` tables read from the wrong segment; `size X` treated as memory; stdcall
`retf N` ignored; `call near ptr` to far procedures pushing an extra word; struct members of size 4 that are far pointers; `jmp <own proc>` becoming recursion; code chunks and labels used as function
pointers; **`setjmp`/`longjmp`** (a `longjmp` returned to the caller of `longjmp` instead of the `setjmp` site, leaving DS = 0); **computed jumps** that IDA never resolved (the picture
blitter `sub_16804` jumps through `[bp+var_2E]` / `var_30` / `var_32` to one of four pixel operations selected by stored offsets, and `pic_copyRect` jumps through
`VIDEO_MODE.fn_copyRect` into a chunk of its own body: all spelled out with `PATCHES` in `lift_handlers.py`); and the missing code chunks stored as far pointers.

## Known gaps

* The conversation menus (`Conversation_Show`) are the original code but wait for mouse/keyboard input; the sweeps do not script it yet, so 77 scenarios stop at the first menu.
* **Pictures**: the lifted decoder is not yet validated - `room 34` loads `XANTH_02.PIC` image 2 and the room items, but the decoded pixels are (almost) all zero and the screen
  stays black. The checks that exist (`HOST_CMPIMG=Image_Free`, `HOST_NZ=label`, `HOST_MEMWATCH`) compare an `Image` in emulated memory with every same-sized image of every `XANTH_nn.PIC`
  decoded by the C++ port. (An earlier note here claimed an exact match for XANTH_18 image 18: that image is entirely black, so the match proved nothing.)
* `main` runs the intro (copyright text, fades) but the host does not yet script the title/menu input.
* Sound devices (SoundBlaster / MIDI) are not emulated; `realsound_play` is recorded.
* The lifted code is exact as far as `../gen/cpu.h` models the 8086: `pushf/popf` and the parity flag are ignored, the direction flag is assumed clear.
