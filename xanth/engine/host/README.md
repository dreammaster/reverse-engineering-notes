# Host for the lifted Companions of Xanth game logic

Runs the *original* game code (lifted from `xanth.asm` to C++, see `../README.md`) on the *original* data, with a small host that provides what the
code expects from DOS and the engine's drawing/sound layers.

```
bash engine/host/build_host.sh <scratch dir>        # from xanth/: image + symbols from the IDB, lift, compile  ->  engine/host/host_test.exe
engine/host/host_test.exe game game_out/image.bin reversing/analysis/xanth_objects.tsv objects|scan|look [room]|rooms
```

## What is in it

* **Memory image** (`game_out/image.bin`, 585,249 bytes, written by `reversing/scripts/dump_image.py`): the flat 1 MB layout of the IDB (linear address =
  segment * 16 + offset). It holds the compiled-in object table, record data, attribute descriptors, handler/exit/event/conversation tables, strings and the
  data segment's initial values. The host adds what `game_init` loads from files (`OBJECT.DAT`, the object-name pool, at 0x80740), points `SS` at the data
  segment like a DOS small-model program, and calls lifted functions with the original stack convention.
* **Lifted code** (`gen/`, generated, git-ignored, ~1,020 functions / 79,000 instructions): all 448 object handlers (including the eight that IDA left as code
  chunks and `apply_xanth_handlers.py` never named: 1, 27, 30, 45, 86, 177, 180, 335), exit/event/conversation functions, the verb handlers and the game logic they call
  (object model, `run_command`, `call_object_handler`, `go_to_room`, `print_obj` with its article logic, timers, random numbers, string helpers) plus every
  function reachable through the compiled function tables (about 100 functions that are called through pointers). Calls through pointers in game memory
  (`call_object_handler`) go through a hook that maps the far address - or an RTLink stub - to the lifted function.
* **Engine functions** the code calls but that are not game logic (about 100: screen, mouse, fonts, palettes, sound, files, dialogs): by default they only record
  the call with its stack arguments (`gen/host_defaults.cpp`); `host_impl.cpp` implements the ones that matter for text: `Font_WriteMessage` /
  `show_text_box` (message id or format string, `%s` far pointers read from emulated memory), `ResourceText_GetStr`, `wputchar?`, `realsound_play`, `_sprintf`.
  Message ids are decoded with `XANTHSTR.DAT` through `../legend_formats.cpp`.

## What was checked (all against the original game files)

| test | result |
|---|---|
| `objects`: lifted `object_has_attr?` / `object_get_location?` / `object_get_prop_a?` / `object_parent_is?` vs the object table decoded independently by `xanth_objects.py` | 11,454 attribute checks, 414 locations, 484 `prop_a` values: **0 mismatches**; `object_parent_is?(x, location(x))` true for 297 of 297 objects (argument order is (child, parent)) |
| `look`: `run_command(LOOK_AT, obj)` through the real pipeline | the bedroom prints the right descriptions, e.g. the desk listing its contents with the original list formatting and the bookshelf using `print_obj`'s article ("The bookshelf contains ...") |
| `scan`: 492 objects with handlers x phases 1-6 x 16 verbs = 47,232 scenarios | 1,196 print text (994 distinct messages); **0 problems**: every message id exists, every `%s` argument is a NUL-terminated string, every object / attribute / verb / event / conversation id passed to the object model, `go_to_room?`, `schedule_event?`, `Conversation_Show`, ... is in range (3,946 calls checked; calls with object 0 are allowed only where the original guards them: `print_obj`, list accessors); no scenario ran away |
| `rooms`: enter each of the 70 rooms with `go_to_room`, LOOK_AT everything in it (215 commands) | 67 rooms entered (two redirect the player on entry - chasm edge -> 384, library -> 426 - and **troll bridge** loops in a conversation the host does not run, see below); 213 of 215 looks print text; 318 distinct messages; one invalid argument: entering Dark Cavern calls `object_move?(0, 0x5B, 0)` with the companion object 0 (the state has no companion) |

The sweeps use the image's initial state with one synthetic command per scenario, so they exercise every branch that state allows, not the game's story.

## Known gaps

* `Conversation_Show` (menus, conversation callbacks, voice) is recorded, not executed: code that waits for a conversation to change a flag (troll bridge on entry) loops
  until the step limit. The conversation engine is the next piece to write on top of `xanth_conversations.tsv` and the `conv_cb_*` functions.
* No pictures, regions, input, sound or save/load: `RoomItem_Draw`, `Mouse_*`, `Screen_*`, `realsound_play`, `save_game?` ... are recorded only.
* The lifted code is exact as far as `../gen/cpu.h` models the 8086: no interrupts, `pushf/popf` and the parity flag are ignored, the direction flag is assumed clear.
* Argument checks cover the object model, rooms, events and conversations; the other engine calls are not validated beyond their message/string arguments.
