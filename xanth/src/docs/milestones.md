# Milestones

Each milestone ends with a check against the oracle (`xanth/engine/host`, the original program running on an emulated PC; see `verification.md`) and is committed
separately. The standalone game stays runnable from the first milestone that has a window.

| # | Milestone | Content | Done when |
|---|---|---|---|
| 0 | Scaffold and data formats | CMake project, `common/` streams and file lookup, `legend/formats` (DCL, PIC, RGN, message strings, MIDI archive, VOC directory, RS clips) without exceptions | `ctest` decodes every picture, string, region, tune and clip of the CD version |
| 1 | Tables and object model | `tools/gen_tables.py` writes `xanth/tables/` from the analysis database (498 objects, attribute descriptors, handler and exit tables, verb grammar, conversation tables, region tables); `ObjectModel` (location, siblings, attributes, properties); 205 save regions and the save file | object-model answers and a save/restore round trip equal the oracle's |
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
