# Milestones

Each milestone ends with a check against the oracle (`xanth/engine/host`, the original program running on an emulated PC; see `verification.md`) and is committed
separately. The standalone game stays runnable from the first milestone that has a window.

| # | Milestone | Content | Done when |
|---|---|---|---|
| 0 | Scaffold and data formats (**done**) | CMake project, `common/` streams and file lookup, `legend/formats` (DCL, PIC, RGN, message strings, MIDI archive, VOC directory, RS clips) without exceptions | `ctest` decodes every picture, string, region, tune and clip of the CD version |
| 1 | Tables and object model (**done**: object tables, `ObjectModel`, `Vars` for the 205 save regions, `GameState` with the original's save format; the verb grammar, handler, exit and conversation tables follow with the logic that uses them) | `tools/gen_tables.py` writes `xanth/tables/` from the analysis database (498 objects, attribute descriptors, handler and exit tables, verb grammar, conversation tables, region tables); `ObjectModel` (location, siblings, attributes, properties); 205 save regions and the save file | object-model answers and a save/restore round trip equal the oracle's |
| 2 | Graphics core | 320x200 screen with palette, fades, `Picture`/`Image` decoding and blitting (all pixel operations of the original blitters), fonts (`*.FNT`), text layout and word wrap, windows, mouse cursors | a room picture, its items and a line of text are pixel-identical to the oracle's screen |
| 3 | User interface | main loop and event handling, verb list, sentence builder, compass, inventory, status/text window, WAIT / MAP / UNDO, dialogs, the scene regions (hover names, click, double-click = default verb) | the scripted opening (switch, kitchen, telephone, letter) gives the oracle's screens and state |
| 4 | Game logic | all 448 object handlers, exit functions, event functions and conversation callbacks as readable C++ (607 handlers exist as generated readable code in `../engine/gen`; the 100 others are rewritten from the instruction-level lift), the verb handlers and default responses, the turn (`execute_command`) | handler scenarios (`scan`) and room entry give identical messages and state |
| 5 | Conversations, events, timers, endings | menus, replies, enabled flags, scheduled events, time of day, score, game-over and ending sequences, cutscenes | the walkthrough runs to the end with identical state at every step |
| 6 | Sound | digital sound effects (`*.RS`) and voice (`XANTH_01.VOC`) through the audio-stream interface, music through the MIDI/OPL interface (stubbed for the standalone build), voice switched off when the file is missing | each sound is played at the right moment (the oracle logs the calls) |
| 7 | Completeness | walkthrough run (https://www.walkthroughking.com/text/companionsofxanth.aspx), coverage of every room, object handler and conversation, save/restore of every state, floppy-style data without voice | scripted full game with no divergence from the oracle |

Lessons from the reverse-engineering work that shape the order (details in `../../reversing/analysis/xanth_overview.md`):

* the engine is table driven: the object table, attributes, handler tables and conversations are data, the 707 handlers are small functions over the object model;
* one game turn is `execute_command` (sentence builder -> `run_command` -> handler phases 1-6 -> notification phase 7 -> timed events), with `go_to_room` as the
  only other way to change state;
* pictures are streamed (PKWARE DCL) into 64 KB-segment buffers by the original code, but only the decoded result matters for a re-implementation;
* the original interface is data driven too (window table, region tables per room); it is rebuilt from the oracle's behaviour, not from pixel positions guessed
  from screenshots.

## Order of the next work

The game logic (milestone 4) is worked on **before** the graphics and interface: it runs headless, and the oracle's `scan` (47,232 scenarios: every object handler x phases 1-6 x 16 verbs, each giving the
messages printed and the state afterwards) and room-entry runs compare it exactly - no pixels needed. The pieces, each verified against the oracle as it lands:

1. the command core: verbs, `run_command`, `call_object_handler`, `go_to_room`, the turn (`execute_command`, `advance_time`), the event list, score, light, `print_obj` and its text formatting;
2. the default verb handlers (`cmdOpen`, `cmdTake`, `cmdLookAt`, `cmdGive` ...);
3. the 707 handlers, regenerated from the instruction-level analysis into structured, named C++ (enums for objects, verbs, attributes, variables; message ids as constants) and reviewed by hand;
4. conversations (menus, enabled flags, callbacks) behind the same message interface;
then milestones 2 and 3 (graphics and interface) on top of the finished logic, and 6 (sound).

## Status (2026-11-10)

Milestone 4, game logic: **in progress**.

* `xanth/game.*`: the command core (`run_command`, `go_to_room`, `advance_time`, the event queue, `add_score`, `print_obj`, message formatting) written by hand from the original, `Ui` interface with a recording headless implementation.
* `xanth/logic/`: about 850 functions (object handlers, exit and event functions, conversation callbacks and texts, default verbs, engine rules, the 49 name callbacks) generated by `reversing/scripts/gen_logic.py` with the structured decompiler; the text of the original's data segment is committed as engine text (`engine_strings.inc`), the static tables it indexes by segment:offset as `exe_tables.inc` / `exe_dseg.inc`.
* Verification: `host_test ... scanlog` (oracle) and `test_scan` (standalone) run every handler x phase x verb scenario (47,232) and compare return value, state hash, event queue and printed text: 46,990 are identical, 65 start a conversation or cutscene (not comparable headless). The remaining differences come from functions the decompiler's output does not compile for yet (they are stubbed: `reversing/analysis/xanth_logic_manual.tsv`, `logic/failed.txt`), from tables the logic reads through pointers into the data segment, and from garbage the original itself reads for out-of-range indices.
* Next: reduce the stubs (typing of text pointers, string instructions, `bp+si` arrays), hand-port what the decompiler cannot do (conversations: `Conversation_Show`, cutscenes), then the interface (milestones 2 and 3).
