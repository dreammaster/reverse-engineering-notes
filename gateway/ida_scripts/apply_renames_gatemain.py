"""
IDA Pro script: master list of symbol renames (functions + globals) for
gatemain.idb (GATEMAIN.EXE / gatemain_decoded.exe -- the main game
engine).

Single accumulating script per the convention established for gate.idb
(apply_renames_gate.py) and the sibling ultima1 project. Whenever a
finding is confirmed, add an entry to RENAMES below and re-run.

Convention: DRY_RUN starts True. Run once with DRY_RUN True, check the
output, then flip and re-run.

For fuller justification of each rename, see the matching section of
docs/overview.md.

    .\\run_ida_script.ps1 -Idb gatemain -ScriptName apply_renames_gatemain.py -NoExport
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    # -- first pass: the "prehandler chain" interpreter primitives,
    # picked up while re-running rank_unnamed_functions.py (now that
    # RTLink-thunk noise is filtered out) and finding sub_11635 still
    # the single highest-value unnamed target (196 callers). Confirmed
    # by direct read, cross-referencing the already-named
    # Logics_getPrehandlerMode/Logics_getPrehandler (in turn confirmed
    # readable only after last session's collapsed-function fix) and a
    # real call site in main(). See
    # docs/overview.md#prehandler-chain-primitives-named. --

    (0x11635, "Logics_prehandlerChainReaches",
     "sub_11635(logicNum, targetLogicNum): walks logicNum's prehandler "
     "chain stage by stage (bounded per-type by METHOD_SECTION_INFO, "
     "same table Logics_getPrehandlerMode consults) -- each stage's "
     "Logics_getPrehandlerMode result, if nonzero, is itself an object "
     "index the search recurses into with the SAME targetLogicNum "
     "(the recursive call is sub_11635(mode, targetLogicNum), not "
     "sub_11635(targetLogicNum, mode) -- confirmed by the exact push "
     "order). Returns 1 as soon as any stage's delegate chain "
     "eventually reaches targetLogicNum, 0 if every stage is "
     "exhausted without a match. Confirmed real call site in main() "
     "(main+0xA9E-ish): sub_11635(vocab_list_0._logicNum, "
     "Logics_logicNum211) -- both arguments are logicNum-shaped "
     "(proc_table indices), not one vocab id and one logicNum as an "
     "earlier guess assumed."),
    (0x115CE, "Logics_prehandlerHasMode",
     "sub_115CE(logicNum, mode, requiredVal2): sibling to "
     "Logics_prehandlerChainReaches but checks EXACT equality against "
     "each stage's Logics_getPrehandlerMode result (no recursion into "
     "a delegate object) -- returns 1 as soon as any stage's mode "
     "equals the given mode value, 0 otherwise. requiredVal2, if "
     ">= 0, gates the whole check on Logics_getVal2_2(logicNum) first "
     "matching it (skipped entirely if requiredVal2 < 0)."),

    # -- second pass, same session: the scoring subsystem, confirmed by
    # actually decoding the real GATESTR.DAT strings these functions
    # reference (a standalone Python re-implementation of
    # huffman_decompress, run against c:\games\gw\GATESTR.DAT -- not an
    # IDA script, so not checked in, but the decoded text is quoted
    # verbatim in the notes below and in docs/overview.md). "Persisted_"
    # names come from an earlier session's SaveField-table enumeration,
    # not deliberately chosen -- renamed to their real meaning now that
    # it's confirmed. --

    (0x1535E, "Score_add",
     "Confirmed by decoding msgId 0x803/0x804 from real GATESTR.DAT: "
     "'[Your score has just gone up by %d.' / ' NOTE: You can activate "
     "and deactivate score-change notification using the NOTIFY "
     "command.'. Adds its argument to _score, and if _scoreNotifyEnabled "
     "is set, prints the above (the NOTE only once, gated by "
     "_scoreNotifyTipShown)."),
    (0xCBB78, "_score",
     "Confirmed via Score_add (adds to this) and msgId 0x2A: 'You have "
     "achieved a score of %d out of 1600, in %d turns.' -- the two "
     "printf args at that call site are (_turnCount, _score) in that "
     "order, matching the message's two %d's."),
    (0xCB7FE, "_turnCount",
     "Incremented once per main() game-loop iteration (a classic turn "
     "counter); confirmed as the first %d in msgId 0x2A's "
     "score-and-turns status message, paired with _score -- see above."),
    (0xCC6AB, "_scoreNotifyEnabled",
     "Gates whether Score_add prints its notification at all -- the "
     "in-game NOTIFY command's persisted toggle, per msgId 0x804's "
     "text (see Score_add's note)."),
    (0xCBB7A, "_scoreNotifyTipShown",
     "One-time flag: Score_add prints the NOTIFY-command explanation "
     "(msgId 0x804) only the first time a score notification fires, "
     "then sets this so it isn't repeated."),
    (0x5668C, "_gameTicks",
     "Confirmed via msgId 0x29: 'It is Dorman day %d.', computed as "
     "_gameTicks/480 + 1 right before that message -- a real-time game "
     "clock in ticks, 480 ticks per in-universe 'Dorman day'."),

    # -- third pass, same session: sub_14A37 (80 callers) and its four
    # previously-unnamed callees turn out to be a generic chunked
    # streaming file loader, not anything filename-specific. Confirmed
    # mechanically: reads a whole file into up to 8 RAM buffers (32-byte
    # header + payload each, since a single DOS allocation can't span
    # the whole file), abortable at any point via Events_isKeyPending
    # and two word_C8582 config-flag bits, then dispatches each filled
    # buffer through a CALLBACK function pointer (word_C8582 -- set by
    # whichever caller configured this run, not traced further), then
    # frees every buffer. High confidence on the mechanical architecture
    # (a real streaming/chunked loader with a pluggable per-chunk
    # handler and player-interruptible loading, era-typical for hiding
    # disk latency behind something else like on-screen text); lower
    # confidence on the exact end use since the callback target itself
    # wasn't traced. See docs/overview.md#stream-subsystem-named. --

    (0x14A37, "Stream_loadFile",
     "Top-level entry: flushes the active window's pending text "
     "(TextWindow_flushPendingText), then reads (Stream_readChunks), "
     "processes (Stream_processChunks), and frees "
     "(Stream_freeChunks) a whole file's worth of chunked buffers."),
    (0x28E2C, "TextWindow_flushPendingText",
     "Flushes Windows_pendingText for the current Windows_activeWindow "
     "via TextWindow_addDirect if that window has buffered/queued text "
     "waiting -- distinct from the already-named TextWindow_flushText."),
    (0x1DDF6, "Stream_readChunks",
     "Opens filename, seeks to find its total size, then reads it into "
     "up to 8 separately-allocated RAM buffers (dword_C84A6[] array, "
     "each up to ~64KB since one DOS allocation can't hold an arbitrary "
     "file), aborting the whole read early if a key is pressed "
     "(Events_isKeyPending) or if word_C8582 bit 8 is clear or bit "
     "0x40 is set."),
    (0x1E052, "Stream_processChunks",
     "Walks the same buffer array Stream_readChunks filled, dispatching "
     "each non-empty one through Stream_processChunk -- again "
     "abortable via Events_isKeyPending between chunks, and gated by "
     "the same word_C8582 bits."),
    (0x1E0E8, "Stream_freeChunks",
     "Frees all 8 of the buffer-array slots via kill_pointer_ -- the "
     "cleanup step after Stream_readChunks/Stream_processChunks."),
    (0x180E3, "Stream_processChunk",
     "Per-chunk handler: reads a couple of header bytes from the "
     "buffer (offsets 5 and 7, before the 32-byte header/payload "
     "boundary already established) into cx/bx, then calls the actual "
     "processing logic through a configurable callback function "
     "pointer (word_C84D0) with the payload pointer (buffer+0x20) -- "
     "not traced further since the callback's actual target varies by "
     "caller and wasn't identified this pass."),

    # -- fourth pass, same session: pulled on the word_C84D0 callback
    # thread. word_C84D0 isn't 4 independent functions -- it's several
    # NEAR-call entry points into one shared decoder continuation body
    # (confirmed by resolving the raw offsets 0x220/0x238/0x493/0x6EE
    # against segment sg09a4's base, 0x1802A -- 3 of the 4 land exactly
    # on loc_ labels *inside* other already-existing functions, not
    # function starts, matching the same "no clean chunk boundary"
    # pattern seen with RTLink thunks two sessions ago). Traced back to
    # its own configuration call, sub_1DDC0, and from there to
    # gatemain_start's own argv-parsing code -- which turned out to
    # directly resolve gate.idb's long-standing "5 unidentified globals
    # passed to GATEMAIN.EXE" open item from the very first gate.idb
    # session: argv[1]->Mouse_enablement, argv[2]->videoMode,
    # argv[3]->cmdline_param3 (renamed here), argv[4..8]->
    # cmdline_param4..8. gate.idb's word_2A256/58/5A/5C/5E are exactly
    # cmdline_param4/5/6/7/8 on this side -- worth a gate.idb pass to
    # rename them there too using this same confirmed mapping. See
    # docs/overview.md#word_c84d0-traced--a-shared-decoder-continuation-not-4-functions. --

    (0x1DDC0, "Stream_configure",
     "Called once from gatemain_start with argv[6]/[7]/[8] (as "
     "cmdline_param6/7/8), i.e. purely launch-time configuration, not "
     "anything computed during play. If cmdline_param6==4: stashes "
     "cmdline_param7/8 into word_C84F3/byte_C84F5 (role not traced), "
     "then always calls Stream_selectHandler(cmdline_param6)."),
    (0x18042, "Stream_selectHandler",
     "Dispatches on its mode argument (0/1/2/4, confirmed as "
     "cmdline_param6 by its only caller, Stream_configure) to wire up "
     "word_C84D0/word_C84D2/word_C84D4 -- NOT 4 independent callback "
     "functions, but several different NEAR-call entry points into one "
     "shared decoder continuation body (spanning what IDA shows as "
     "several separate small subs -- e.g. mode 1's target is 32 bytes "
     "into what IDA calls sub_18242, skipping a precondition check mode "
     "0's target doesn't skip). The actual decoded resource type "
     "wasn't identified this pass -- deferred, see roadmap.md."),
    (0xC85A4, "_soundMode",
     "argv[3] (via atoi) in gatemain_start's own parameter parsing -- "
     "confirmed positionally to be the same soundMode argument gate.idb's "
     "_main passes as arg3 to _execl (that session left it named only "
     "as 'soundMode' locally, not cross-referenced to this global). "
     "Manipulated with bit tests/masks elsewhere (e.g. 'and "
     "cmdline_param3, 0F7h'), so it's a bitflag word, not a plain enum."),
    (0xC85AA, "_streamMode",
     "argv[6] -- confirmed as Stream_configure/Stream_selectHandler's "
     "mode argument (0/1/2/4), a pure launch-time configuration value, "
     "not anything computed during play."),

    # -- fifth pass, same session: kept pulling on the word_C84D0
    # thread and found something more fundamental than "resource type" --
    # Stream_selectHandler's mode branches are gated by a CPU SPEED
    # RATING, not a file/resource format. sub_18182 (called
    # unconditionally at Stream_selectHandler's start) installs a
    # temporary custom INT 70h handler and reprograms the 8253/8254 PIT
    # to a known frequency; sub_18148 then runs a fixed CPU-bound
    # busy-loop (imul-based) until that ISR has decremented a tick
    # counter to 0, and the number of loop iterations completed becomes
    # word_C84DA, this whole cluster's CPU speed rating -- exactly the
    # value Stream_selectHandler compares against 0x160 to pick a
    # performance-tier code path. sub_181D8 restores the original INT
    # 70h/72h vectors, the PIT's original divisor, AND the system
    # date/time (since running the PIT at a different frequency drifts
    # DOS's own clock) -- a complete, correctly-bracketed calibrate/
    # restore pair. This refines rather than replaces the earlier
    # Stream_* naming: Stream_selectHandler genuinely does select a
    # handler, just gated on CPU speed rather than resource type as
    # hedged before -- see docs/overview.md's updated note. --

    (0x18182, "Cpu_beginSpeedTest",
     "Saves the real INT 70h/72h vectors and current DOS date/time, "
     "installs a temporary ISR (just decrements byte_C84E8 and irets), "
     "and reprograms the 8253/8254 PIT (port 43h mode/command, port "
     "40h count) to a known frequency for Cpu_measureSpeed's busy-loop "
     "to count against."),
    (0x18148, "Cpu_measureSpeed",
     "Sets byte_C84E8=1, then runs a fixed imul-based busy loop "
     "(8 outer iterations) until Cpu_beginSpeedTest's temporary ISR "
     "decrements byte_C84E8 to 0 on a timer tick; the iteration count "
     "reached (shifted right 3) becomes word_C84DA -- a CPU speed "
     "rating, faster CPUs completing more busy-work per fixed-length "
     "timer tick."),
    (0x181D8, "Cpu_endSpeedTest",
     "Restores the real INT 70h/72h vectors and PIT divisor "
     "Cpu_beginSpeedTest saved, then re-sets DOS's date/time from the "
     "same saved values -- correcting for clock drift caused by "
     "running the PIT at the calibration frequency in between."),
    (0xC84DA, "cpuSpeedRating",
     "Cpu_measureSpeed's result; Stream_selectHandler compares this "
     "against 0x160 to choose between performance-tier code paths -- "
     "the actual criterion behind that mode dispatch, not a resource-"
     "type distinction as hedged in the fourth-pass notes above."),

    # -- sixth pass, same session: back to rank_unnamed_functions.py's
    # top of the (now-shorter) list. See
    # docs/overview.md#queue_remove-and-logics_checkmoverestriction-named. --

    (0x12ED2, "Queue_remove",
     "Sibling to the already-named Queue_add: searches _queueTable "
     "(the same array Queue_add appends to, confirmed by the identical "
     "seg126_93/-0x73FCh offset access) for an entry whose _id matches "
     "its argument, and if found, memmove-compacts every later entry "
     "down by one slot -- classic remove-and-compact on a flat array."),
    (0x14B64, "Logics_checkMoveRestriction",
     "Confirmed by decoding its own message text (GATESTR.DAT 0x800: "
     "\"You can't move while you're wearing the collar.\"; 0x801: "
     "\"[You get o%sf%s first.]\", i.e. a dismount/disembark-first "
     "message): a shared movement-precondition gate checked before "
     "letting the player move, gating on several hardcoded plot-item "
     "logicNums (0xD3/211 = the collar; others, e.g. 0xA2/0xA8/0x9D, "
     "not individually identified -- plausibly mount/vehicle-related "
     "given the dismount message) via Logics_prehandlerChainReaches/"
     "Logics_IsPrehandler1/Logics_prehandlerHasMode, plus a "
     "Logic_call(_roomLogicNum, action=0xF) room-override hook. Returns "
     "nonzero (with the blocking message already printed) if movement "
     "is currently disallowed."),
    (0x14ED6, "Logics_tryMoveDirection",
     "sub_14ED6(directionChar): the core room-exit resolution function. "
     "arg_0 is the parsed direction character ('n'/'s'/'e'/'w'/etc, "
     "confirmed by direct comparison against a per-room exit-table "
     "entry field and by setting Parser_val21 to the same byte on a "
     "match). Walks the current room's exit table (var_6 entries, "
     "count from sub_12445(_roomLogicNum)), each entry a small variant "
     "record (direction char + a 1-5 type tag + type-specific data) "
     "dispatching to 5 different exit-resolution shapes -- a direct "
     "room link, a Logics_getBit-gated door (bits 0xC/0x10, printing "
     "the locally-embedded string \"%sn't open.\\n\" when closed -- not "
     "a GATESTR.DAT message, a literal string constant in this "
     "executable's own data segment), a fixed blocked-message table "
     "lookup, a sub_14742-computed dynamic "
     "destination, and one more table-based lookup not fully "
     "distinguished from the others. Calls the already-named "
     "Logics_checkMoveRestriction before actually moving, and falls "
     "back to \"You can't go that way.\\n\" when no exit matches. Not "
     "fully traced: sub_123F3/sub_12445/sub_14742's individual roles, "
     "and the exact semantics of each of the 5 exit-type branches."),

    # -- eighth pass, same session: the three Logics_tryMoveDirection
    # helpers, per Paul's direction. See
    # docs/overview.md#logics_getroommoveenabled-logics_getroomexitcount-logics_callspecialexit-named. --

    (0x123F3, "Logics_getRoomMoveEnabled",
     "sub_123F3(logicNum): validates range and proc_table.type==1 "
     "(must be a Room), then returns the low byte of Room.field_16 "
     "(0 for anything else). Logics_tryMoveDirection treats this as an "
     "overall gate -- if zero, movement processing is skipped entirely "
     "for the room regardless of its exit table -- distinct from (and "
     "checked independently of) Logics_checkMoveRestriction's "
     "item-based gating. Exact semantics of Room.field_16 itself not "
     "confirmed beyond this usage."),
    (0x12445, "Logics_getRoomExitCount",
     "sub_12445(logicNum): identical shape/guard to "
     "Logics_getRoomMoveEnabled, but returns the full word at "
     "Room.field_18 -- used directly as the exit-table entry count in "
     "Logics_tryMoveDirection (decremented once there for 0-based "
     "indexing, then drives the direction-matching loop)."),
    (0x14742, "Logics_callSpecialExit",
     "sub_14742(exitId, action): a function-pointer dispatch table "
     "(off_3C862[exitId*4], bounds-checked exitId < 0x2C/44), calling "
     "whichever handler is selected with a single action-code "
     "argument. This is Logics_tryMoveDirection's exit-type-4 branch "
     "(a per-exit \"special\" handler rather than a plain room link or "
     "gated door) -- called there as "
     "Logics_callSpecialExit(exitTableEntryId, 0xF), matching the "
     "action-code convention already seen elsewhere (e.g. "
     "Logic_call/room_load's action arguments). Individual handler "
     "routines in the dispatch table not traced."),

    # -- ninth pass, same session: back to rank_unnamed_functions.py's
    # top of the list -- turned out to be the game's actual win/lose
    # ending. See docs/overview.md#game_showendingmessage-and-game_endgamemenu-named. --

    (0x312D1, "Game_showEndingMessage",
     "Confirmed by decoding its own message text: msgId 0x7816 = "
     "\"You have failed.\", 0x7817 = \"You have won the game, scoring "
     "%d out of 1500 points.\" (using _score as the %d). Branches on "
     "_hasWonGame (was byte_CBB6E), prints the appropriate ending "
     "message, resets that flag, then calls Game_endGameMenu."),
    (0xCBB6E, "_hasWonGame",
     "Gates which of the two ending messages Game_showEndingMessage "
     "prints -- confirmed by the message text itself (fail vs. win)."),
    (0xC4AE6, "Game_endGameMenu",
     "The classic post-ending prompt loop: calls sub_C48E4 repeatedly "
     "for a choice, dispatching 1=restart (sub_1057E), "
     "2=restore-a-save (j_load_game(2)), 3=undo "
     "(Parser_performUndo), 4=quit (j_shutdown) -- the textbook "
     "Infocom-style \"RESTART, RESTORE, UNDO, or QUIT?\" menu shown "
     "after winning or losing. sub_C48E4 (the actual prompt/choice "
     "getter) not traced."),

    # -- tenth pass, same session: the container/surface "you see"
    # description cluster. See
    # docs/overview.md#logics_describecontents-logics_countvisiblecontents-logics_listcontents-named. --

    (0x149D8, "Logics_describeContents",
     "sub_149D8(logicNum, prepositionType): confirmed by its own "
     "message text, \"\\t%cn%s you see\" -- %c is 'O' (prepositionType "
     "== 1, \"On\") or 'I' (otherwise, \"In\"), %s is the far-pointer "
     "string j_printObj(logicNum, 2) returns (the object's own printed "
     "name). First calls thunk_sub_66DFD (Logics_countVisibleContents) "
     "as a gate -- prints nothing at all if it returns 0 -- then builds "
     "\"On/In <object> you see\", calls thunk_sub_667D0 "
     "(Logics_listContents) to print the actual comma-separated "
     "content list, and closes with \".\\n\". The classic container/"
     "surface description sentence."),
    (0x66DFD, "Logics_countVisibleContents",
     "sub_66DFD(logicNum, handlerId): walks a linked list of contained "
     "items (Logics_getUnkHandler for the first, then Logics_getVal1 "
     "repeatedly for each next item, terminated at 0), counting only "
     "those where Logics_getBit(item, 8) is false (bit 8 -- not "
     "individually confirmed, but consistently used as a visibility/"
     "hidden flag across this cluster). Used by "
     "Logics_describeContents purely as a \"is there anything to show\" "
     "gate."),
    (0x667D0, "Logics_listContents",
     "sub_667D0(logicNum, handlerId): the same linked-list walk as "
     "Logics_countVisibleContents, but instead of counting, prints each "
     "visible item's name (via Logics_getVal1-chased entries), "
     "inserting \",\" (TextWindow_addChar) between entries -- the "
     "actual comma-separated content listing "
     "Logics_describeContents prints after \"On/In <object> you "
     "see\"."),

    # -- eleventh pass, same session: the in-game clock/status-line
    # builder. See docs/overview.md#game_updatestatusline-named. --

    (0x136AF, "Game_updateStatusLine",
     "Confirmed by reading its two literal (non-GATESTR.DAT) format "
     "strings at seg067+0x227/+0x238: \"%s %d, %02d:%02d\" (24-hour) "
     "and \"%s %d, %d:%02d%c\" (12-hour, %c = 'a'/'p' for am/pm). "
     "Computes hour-of-day/minute from _gameMinutes (was "
     "Persisted_val4: /60 then %24 for hour, %60 for minute) and a day "
     "number from _gameDayNumber+0x10 (was Persisted_val5), formats "
     "\"<name> <day>, <time>\" (name via a 12-entry lookup table at "
     "seg067_0+0x1FEh, not itself decoded), appends the current room's "
     "name (Logics_getObjectString(_roomLogicNum)), and passes the "
     "combined string through sub_160E1 (a bounded string-copy utility, "
     "not confidently a \"set title\" function on its own -- not "
     "renamed). Blanked entirely (spaces instead of a real time) when "
     "_statusTimeHidden (was Persisted_val7) is set. Persisted_val194's "
     "role in selecting the 24-hour vs. 12-hour format (compared "
     "against the literal value 8) wasn't confirmed beyond that "
     "comparison -- left unrenamed."),
    (0xcb800, "_gameMinutes",
     "Elapsed in-game minutes -- confirmed via Game_updateStatusLine's "
     "hour/minute-of-day math (/60 then %24 for hour, %60 for minute). "
     "A separate, more granular clock from _gameTicks (480 ticks/day, "
     "used for the 'Dorman day %d' message) -- both track real-time "
     "progression but at different resolutions for different displays."),
    (0xcb802, "_gameDayNumber",
     "Elapsed in-game day count -- confirmed via Game_updateStatusLine "
     "(added to a fixed +0x10 offset, plausibly a calendar-epoch shift, "
     "before being formatted as the day number)."),
    (0xcbb6f, "_statusTimeHidden",
     "Nonzero blanks Game_updateStatusLine's time portion (spaces "
     "instead of a real \"day, time\" string) -- confirmed directly by "
     "that branch's code."),

    # -- twelfth pass: the "-- MORE --" pagination prompt, picked up as
    # rank_unnamed_functions.py's new top target (24 callers) after
    # Game_updateStatusLine was named. Confirmed by direct read plus the
    # two literal strings it displays. See
    # docs/overview.md#textwindow_showmoreprompt-named. --

    (0x1496B, "TextWindow_showMorePrompt",
     "sub_1496B(): the classic \"-- MORE --\" screen-pagination prompt. "
     "Flushes an (always-empty here) buffer via TextWindow_addDirect, "
     "saves the cursor position (get_text_cursorPos), writes the "
     "literal string \"- MORE -\" (aMore_1 at 0xCBB7C, immediately "
     "after the already-named _scoreNotifyTipShown/unk_CBB7B bytes), "
     "polls j_Events_waitForPress/Events_checkKeypress until a key is "
     "pressed, restores the cursor position, overwrites the prompt "
     "with 8 literal spaces (0xCBB85), then calls "
     "TextWindow_resetFontLinesRemaining to reset the page-line "
     "counter so the next screenful can accumulate before prompting "
     "again. 24 callers, scattered across many different logic/method "
     "routines -- i.e. invoked wherever paginated text output fills "
     "the window, not owned by one subsystem."),

    # -- thirteenth pass: the TAKE-command mechanics, confirmed by a real
    # call site printing the literal string "You take%s" right before
    # calling sub_153B6. See
    # docs/overview.md#logics_takeobject-named--the-take-command-mechanics. --

    (0x153B6, "Logics_takeObject",
     "sub_153B6(logicNum): confirmed by its one real call site "
     "(sub_6AD19, the TAKE verb's dispatcher), which prints the "
     "literal message \"You take%s\" (aYouTakeS) immediately before "
     "calling this. Bails out (returns 0) if "
     "thunk_sub_67662(logicNum, Logics_logicNum211, 0) returns 2 "
     "(object can't be taken, e.g. fixed/scenery). Otherwise: "
     "reassigns the object's handler to Logics_logicNum211 "
     "(Logics_updateHandler), clears bit 8 (the same 'hidden/visible' "
     "bit seen in the Logics_*Contents cluster), awards a one-time "
     "pickup score bonus via Logics_getTakeScore/Score_add/"
     "Logics_setTakeScore(logicNum, 0) (zeroing it so it isn't paid "
     "out again), then sets bit 2 ('taken'), clears bit 0xA, sets bit "
     "0x1D, clears bit 8 again, and returns 1."),
    (0x12109, "Logics_getTakeScore",
     "sub_12109(logicNum): bounds-checked (against METHODS_COUNT) "
     "proc_table lookup returning the type-tagged struct's "
     "field_E/field_20/field_14 (Room/LogicSection2/LogicSection8 -- "
     "same field slot across all three type shapes, same dispatch "
     "idiom as the already-named Logics_getVal2_2 family). Confirmed "
     "as a one-time take-score value: every call site immediately "
     "Score_add()s a nonzero result and then zeroes it via "
     "Logics_setTakeScore, both in Logics_takeObject and in the "
     "similarly-shaped take logic inside sub_143F3 (not itself "
     "renamed this pass)."),
    (0x12179, "Logics_setTakeScore",
     "sub_12179(logicNum, value): setter half of Logics_getTakeScore "
     "-- same bounds-checked proc_table field_E/field_20/field_14 "
     "dispatch, just storing instead of loading."),

    # -- fourteenth pass: sub_24FFB, confirmed as the low-level mouse/
    # keyboard-emulated pointer-polling primitive that the already-named
    # get_mouse_input wraps (a real call site at get_mouse_input+0xB6
    # passes &x, &y straight through to it). sub_26F2A (19 callers, the
    # actual top of this pass's re-ranked list) was investigated but
    # left unnamed -- see docs/overview.md for why. See
    # docs/overview.md#mouse_pollposition-named. --

    (0x24FFB, "Mouse_pollPosition",
     "sub_24FFB(xPtr, yPtr): confirmed via its one real caller, "
     "get_mouse_input, which passes &x/&y straight through. If both "
     "pointers are null, just forwards to the already-named "
     "get_mouse_buttons(). Otherwise: if mouseState's keyboard-cursor-"
     "emulation bits are set, reads one key via sub_25216 (not "
     "renamed -- single caller, private helper) to move a "
     "keyboard-driven cursor and feeds Enter/Space through "
     "addCharacter as a click; else reads the real mouse position/"
     "buttons via INT 33h (AH=3), waiting out an already-held button "
     "via sub_24FAE (not renamed -- single caller, private helper) "
     "first. Writes the resulting x/y through the two far-pointer "
     "arguments and returns a button/click code."),

    # -- fifteenth pass: sub_148E8, a generic keyed-message lookup+print
    # utility reused by many different compiled logic routines with
    # their own local static tables. Confirmed by reading two different
    # call sites (sub_76A79/method158, and sub_87302 -- a thin
    # single-arg wrapper passing its own arg straight through with a
    # different table/count), which rules out any one-call-site-specific
    # meaning for the key argument. See
    # docs/overview.md#logics_printkeyedmessage-named. --

    (0x148E8, "Logics_printKeyedMessage",
     "sub_148E8(key, table, count): table is a far pointer to `count` "
     "6-byte records (word key, dword far-pointer-to-message-string). "
     "Scans for a record whose key equals `key` OR is 0 (a wildcard/"
     "default entry); once found, if that record's message pointer is "
     "non-null, prints it via TextWindow_add and returns 1; if the "
     "message is null, keeps scanning forward through SUBSEQUENT "
     "records (regardless of their own key) for the first one with a "
     "non-null message and prints that instead -- i.e. a match with an "
     "empty message is a placeholder pointing at a shared fallback "
     "message a little further down the same table. Returns 0 if no "
     "matching record is ever found. Confirmed generic (not tied to "
     "one particular key source) via two call sites: sub_76A79 "
     "(a compiled logic method) passes vocab_list_0._altVocabId "
     "against a 6-entry table; sub_87302 is a thin wrapper passing its "
     "own single argument straight through against an unrelated "
     "55-entry table. This is the same 'per-object special-response "
     "override table, falling back to generic text' pattern seen "
     "elsewhere in the compiled room/object logic."),

    # -- sixteenth pass: sub_27134, confirming and CORRECTING last
    # pass's guess about it. It's not a per-tick animation driver --
    # it's the animated-picture-overlay subsystem's teardown/free-all
    # routine. Confirmed mechanically via sub_26D08 (itself confirmed
    # as a trivial "Image_Free N consecutive Image records" helper) and
    # kill_handle. See
    # docs/overview.md#animpics_freeall-named--correcting-last-passs-guess-about-sub_27134. --

    (0x27134, "AnimPics_freeAll",
     "sub_27134(): for each active animated-picture slot (0.."
     "_animPicsSlotCount, was word_C9658), frees its 20 Image records "
     "via the new Image_freeFrames(handle, 20) and then kill_handle()s "
     "the slot's own handle (animPicsHandles[i], was byte_D22DA -- a "
     "5-slot array of dword handles), zeroing it. Finally resets "
     "_animPicsSlotCount to 0. CORRECTS last pass's overview.md guess "
     "that this was 'the already-initialized per-tick driver' -- it's "
     "the teardown/free-all routine, called from AnimPics_resetForRoom "
     "(was sub_26D50) specifically when slots are already active."),
    (0x26D08, "Image_freeFrames",
     "sub_26D08(imgFarPtr, frameCount): trivial helper -- calls "
     "Image_Free on frameCount consecutive Image-sized records "
     "starting at imgFarPtr. Confirmed directly by its body; used by "
     "AnimPics_freeAll to release a slot's up-to-20 loaded frames."),
    (0x26D50, "AnimPics_resetForRoom",
     "sub_26D50(): if _animPicsSlotCount (was word_C9658) is already "
     "nonzero, tears down every active slot via AnimPics_freeAll; "
     "otherwise (first-ever call) just zeroes the 10-word per-slot "
     "frame-duration/timing table at 0xA3BA that AnimPics_freeAll's "
     "sibling slot-register/draw functions (sub_26D7E/sub_26F74, not "
     "renamed this pass) maintain. Called from graphics_init -- the "
     "room-transition reset point for the animated-picture-overlay "
     "subsystem sighted last pass."),
    (0xC9658, "_animPicsSlotCount",
     "Was word_C9658: count of active animated-picture-overlay slots "
     "(0-5), shared by AnimPics_freeAll/AnimPics_resetForRoom and the "
     "not-yet-renamed sub_26D7E (registers a new slot, capped at 5)/"
     "sub_26F74 (per-slot timing/draw loop)."),
    (0xD22DA, "animPicsHandles",
     "Was byte_D22DA: 5-slot array of dword memory handles, one per "
     "active animated-picture-overlay slot -- allocated in sub_26D7E "
     "(not renamed this pass) via new_handle, freed in AnimPics_freeAll "
     "via kill_handle."),

    # -- seventeenth pass: investigated sub_15DB2 (15 callers, top of
    # the re-ranked list), which turns out to be a "change the current
    # sound/music track" function tangled up with the already-documented
    # Stream_*/digitized-sound engine (same word_C8582 config-flags
    # global). Confidently confirmed exactly one piece of it:
    # get_buff_size? really is what its tentative name says. Everything
    # else -- including the discovery that the existing tentative name
    # "startGame?" is almost certainly WRONG (it's actually a
    # stop-current-sound-before-switching function, not anything to do
    # with starting a game) -- is left unrenamed pending a dedicated
    # future pass. See
    # docs/overview.md#sound-track-selection-subsystem-sighted--get_buffer_size-confirmed-startgame-flagged-as-mislabeled. --

    (0x1052c, "get_buffer_size",
     "get_buff_size?(): confirmed exactly as its tentative name says -- "
     "finds the largest free memory block (get_largest_free_block_2), "
     "reserves a fixed amount depending on video mode (halved if "
     "img._active), and returns whatever's left (0 if not enough). "
     "Simply dropping the uncertainty-marking '?' now that it's "
     "directly confirmed; not part of the broader rename-worthy finding "
     "below."),

    # -- eighteenth pass: sub_1D896, the top confidently-traceable
    # target after sub_26F2A (still unnamed) and sub_4A69F (skipped --
    # landed in a corrupted-looking cluster with "sp-analysis failed"
    # functions and bad far-call targets, a suspected new instance of
    # the known RTLink-flattening segment-word bug, not renamed). This
    # is the long-sought hardware-output-side entry into the .MUS/MIDI
    # playback engine flagged in docs/overview.md's earlier "honestly
    # murky" .MUS writeup. See
    # docs/overview.md#midi_sendbyte-named--the-mus-engines-hardware-output-side-finally-traced. --

    (0x1D896, "Midi_sendByte",
     "sub_1D896(byte): polls the status port (_midiStatusPort, was "
     "word_C83AC) for bit 0x40 clear (MPU-401's 'output not ready/"
     "busy' bit) with a 0xFFFF-iteration timeout; once clear, writes "
     "`byte` to the data port (_midiDataPort, was word_C83AA) and "
     "returns 1, or 0 on timeout. Confirmed as MPU-401 UART-mode MIDI "
     "output (not e.g. a printer, an earlier hypothesis) via the "
     "surrounding cluster: sub_1D966 (not renamed) configures these "
     "same two ports from a caller-supplied base port and installs a "
     "DOS interrupt vector at IRQ+8 with 8259 unmasking -- classic "
     "MPU-401 IRQ-driven setup; sub_1EE70 (not renamed) reads a 3-byte "
     "big-endian value via sub_1ECB6 (a generic per-track "
     "stream-offset byte reader) at offsets 2/3/4 -- the exact shape "
     "of a Standard MIDI File tempo meta-event (FF 51 03 tt tt tt) -- "
     "and its arithmetic involves the literal constant 500000, MIDI's "
     "default microseconds-per-quarter-note unit, before calling this "
     "function to actually transmit the resulting byte(s). This is the "
     "hardware-output-side entry into the same `.MUS` background-music "
     "engine already reached from the memory-management side via the "
     "previously-flagged sub_1FE5C (see file-formats.md's 'honestly "
     "murky' .MUS section) -- not fully unified into one confirmed "
     "picture yet, but a real breakthrough on the piece that was "
     "explicitly flagged as needing exactly this angle."),
    (0xC83AA, "_midiDataPort",
     "Was word_C83AA: MPU-401 data port, set from a caller-supplied "
     "base port in sub_1D966 (not renamed), read by Midi_sendByte."),
    (0xC83AC, "_midiStatusPort",
     "Was word_C83AC: MPU-401 status port (base+1), set alongside "
     "_midiDataPort in sub_1D966 (not renamed); Midi_sendByte polls "
     "its bit 0x40 (output-not-ready/busy) before writing."),

    # -- nineteenth pass: sub_288F4, a simple clock-based busy-wait
    # delay utility. Confirmed directly by its body. See
    # docs/overview.md#clock_delayticks-named. --

    (0x288F4, "Clock_delayTicks",
     "sub_288F4(loTicks, hiTicks): records the current _clock() value, "
     "then busy-loops calling _clock() again until the elapsed 32-bit "
     "tick count (current - start) is >= the (loTicks, hiTicks) 32-bit "
     "target. A generic timing/delay primitive -- used by the "
     "already-named Screen_fadeOut to pace fade steps, and by "
     "sub_26C0C (part of the Image_Init/AnimPics-adjacent cluster, not "
     "renamed) for similar pacing."),

    # -- twentieth pass: sub_2899D, confirmed as the "invalid mouse
    # click target" error-beep UI feedback sound, and its underlying
    # tone-generation primitive sub_28920. Confirmed via a real call
    # site (sub_28595, itself called from the already-named
    # get_mouse_input) that plays it specifically on an out-of-range/
    # empty clickable-region selection. See
    # docs/overview.md#speaker_playerrorbeep-named--pc-speaker-tone-generation-confirmed. --

    (0x28920, "Speaker_playTone",
     "sub_28920(freqLo, freqHi, durationLo, durationHi): the classic "
     "PC-speaker square-wave tone sequence -- enable the speaker (in "
     "al,61h; or al,3; out al,61h), program PIT counter 2 for square-"
     "wave mode (out 43h,0B6h), write the 16-bit frequency divisor as "
     "two sequential bytes to the counter's data port (out 42h, "
     "freqLo/freqHi), hold for (durationLo, durationHi) ticks via the "
     "already-named Clock_delayTicks, then disable the speaker again "
     "(in al,61h; and al,0FCh; out al,61h). A separate, simpler "
     "primitive from the digitized-sample PC-speaker playback engine "
     "documented earlier -- this one generates a plain tone, no "
     "sample data involved."),
    (0x2899D, "Speaker_playErrorBeep",
     "sub_2899D(): plays a short ~4004Hz tone (divisor 298) for 50 "
     "ticks via Speaker_playTone, waits 50 ticks (Clock_delayTicks), "
     "then plays the same tone again -- a double-beep. Confirmed as "
     "specifically an 'invalid selection' error sound via its real "
     "call site in sub_28595 (itself reached from the already-named "
     "get_mouse_input): played and returns 0 when the clicked region "
     "index is out of range or maps to an empty/invalid table entry, "
     "vs. the normal path which inserts the selected character into "
     "the input line and returns 1."),

    # -- twenty-first pass: sub_1CAF6, confirmed as the fundamental
    # AdLib/OPL2 FM-synthesis chip register-write primitive -- a whole
    # new sound-hardware subsystem (distinct from the PC-speaker beep/
    # digitized-sample engines already documented) sighted via this
    # single function. Confirmed conclusively: the port variable it
    # writes through is assigned the literal 0x388 elsewhere -- the
    # standard AdLib/OPL2 base I/O address, not a coincidental DMA-
    # controller address as IDA's stale auto-comment on the same "out"
    # instruction suggested. See
    # docs/overview.md#opl2_writeregister-named--an-adlibopl2-fm-synthesis-subsystem-sighted. --

    (0x1CAF6, "Opl2_writeRegister",
     "sub_1CAF6(reg, value): writes `reg` to the OPL2 address port "
     "(_opl2BasePort, was word_D3BD0 -- confirmed assigned the literal "
     "0x388, the standard AdLib/OPL2 base address, elsewhere in this "
     "same overlay), does several dummy port reads (the chip's "
     "required inter-write settling delay), writes `value` to the "
     "data port (_opl2BasePort+1), then several more dummy reads (a "
     "longer delay, matching the OPL2's well-documented longer "
     "settling time after a data write than after an address write). "
     "IDA's inline comment on this exact 'out dx,al' ('DMA controller, "
     "8237A-5, channel 0 base address and word count') is a stale "
     "auto-annotation matching the literal port VALUE 0 in isolation -- "
     "misleading here, since the actual port comes from a variable "
     "confirmed set to 0x388, not 0. Its only caller, sub_1CB32 (not "
     "renamed), computes and writes OPL2 registers 0xA0+channel/"
     "0xB0+channel -- the per-channel frequency-LSB and octave/key-on/"
     "frequency-MSB registers -- strongly suggesting an "
     "'Opl2_setChannelFrequency'-shaped function, left for a future "
     "pass."),
    (0xD3BD0, "_opl2BasePort",
     "Was word_D3BD0: confirmed assigned the literal 0x388 elsewhere "
     "in this overlay -- the standard AdLib/OPL2 FM synthesizer base "
     "I/O address. Read by Opl2_writeRegister."),

    # -- twenty-second pass: sub_143F3, called directly from main().
    # Confirmed via real decoded GATESTR.DAT text (msgId 0xC406:
    # "[Taking%s first.]\n") as the "automatically take a prerequisite
    # object" mechanic -- the classic parser-adventure idiom where e.g.
    # "unlock door" with the key on the floor first prints
    # "[Taking the key first.]" and picks it up before the real action
    # proceeds. Reuses the same take-mechanics tail as the already-named
    # Logics_takeObject (Logics_updateHandler/Logics_getTakeScore/
    # Score_add/Logics_setTakeScore/bit twiddling), gated behind several
    # preconditions this pass didn't fully unpick. See
    # docs/overview.md#logics_autotakeobject-named--the-take-the-key-first-mechanic. --

    (0x143F3, "Logics_autoTakeObject",
     "sub_143F3(logicNum): only proceeds if logicNum matches the "
     "current parser subject (Logics_logicNum211 == Parser_val2), the "
     "object's prehandler type (proc_table-shaped lookup via "
     "Logics_getPrehandler) is 7, bit 0x1D is set (a flag "
     "Logics_takeObject itself sets on a normal take -- plausibly "
     "'portable'/'auto-takeable'), bit 0xA is clear, "
     "Logics_prehandlerChainReaches(logicNum, Logics_logicNum211) is "
     "FALSE, and thunk_sub_67662(logicNum, Logics_logicNum211, 0) "
     "returns 0. If all hold: prints the object's name then the real, "
     "decoded GATESTR.DAT message 0xC406 -- '[Taking%s first.]' -- "
     "then performs the exact same take-mechanics tail as "
     "Logics_takeObject (Logics_updateHandler, clear bit 8, one-time "
     "Logics_getTakeScore/Score_add/Logics_setTakeScore gated on bit "
     "2, then set bit 2). The classic parser-adventure 'auto-take a "
     "needed object before performing the real command' idiom. Not all "
     "of the gating preconditions' exact meanings were nailed down "
     "this pass (particularly prehandler type 7 and bit 0xA) -- "
     "flagged for a future pass rather than guessed."),

    # -- twenty-third pass: sub_26EDC, another AnimPics-cluster function
    # (uses the already-named _animPicsSlotCount and the same 0xA3BA-ish
    # per-slot table sub_26D7E/sub_26F74 maintain). Confirmed via a
    # real caller, room_load, as a timer-resync routine. See
    # docs/overview.md#animpics_resyncslots-named. --

    (0x26EDC, "AnimPics_resyncSlots",
     "sub_26EDC(): for each active animated-picture slot (0.."
     "_animPicsSlotCount), resets the per-slot frame-index byte (same "
     "field sub_26F74 reads as its frame position) to either 0, or "
     "frameCount-1 if the slot's loop-direction byte reads 0xFF "
     "(playing in reverse) -- then adds the current _clock() value "
     "into a per-slot 32-bit accumulator at 0xA3E2+slot*4, re-basing "
     "each slot's animation-timing clock to now. Confirmed called from "
     "room_load: this is the 'resync all active animation timers to "
     "the current clock' step run after a room transition, preventing "
     "animations from jumping forward using stale elapsed time "
     "accumulated while the game was paused/loading."),

    # -- twenty-fourth pass: skipped sub_4A616 (12 callers) -- another
    # "sp-analysis failed" tiny stub in the same suspicious 0x4A6xx
    # neighborhood as the sub_4A69F cluster flagged two passes ago,
    # reached both by normal `call` and by a cross-segment `jmp` from a
    # different overlay (seg101:0029) straight into its body; it just
    # sets a return value of 0 or 1 with no arguments, so its semantic
    # role is entirely caller-dependent and wasn't pursued further. See
    # docs/overview.md#queue_find-named--a-companion-to-queue_remove. --

    (0x12F81, "Queue_find",
     "sub_12F81(key): read-only companion to the already-named "
     "Queue_remove -- confirmed via a direct structural match against "
     "its body (same _queueCount-bounded scan over the same 4-byte-"
     "entry table at the same seg126_93-relative offsets, matching on "
     "the same key byte at entry+0). Returns the entry's stored word "
     "(entry+2) if found, or the sentinel 0x7FFF if not -- a find/peek "
     "operation where Queue_remove additionally deletes the match."),

    # -- twenty-fifth pass: sub_265B0, confirmed via its two helpers'
    # OWN caller list -- both are also called directly by the
    # already-named scale_pic (itself reached from Image_load), so this
    # function is doing the same load-and-scale-to-fit sequence as
    # scale_pic, just starting from a picture number instead of an
    # already-loaded picture. See
    # docs/overview.md#load_and_scale_pic-named. --

    (0x265B0, "load_and_scale_pic",
     "sub_265B0(picNumber): calls the already-named load_picture"
     "(picNumber, frameNumber=0), then sub_25B90() and "
     "sub_25BCE(result) (not renamed) -- the exact same two-helper "
     "sequence the already-named scale_pic (reached from Image_load) "
     "uses to scale a picture to fit its target area. Named to match "
     "that existing lowercase-underscore convention rather than "
     "invent a new one for this small, closely-related cluster. Many "
     "unrelated callers (Commset_show, sub_7179E, sub_74149, and 8 "
     "more) confirm this is a generic 'load and display picture N, "
     "scaled' entry point, not owned by one subsystem."),

    # -- twenty-sixth pass: sub_26D7E, the AnimPics slot-registration
    # function already fully characterized (but not renamed) when
    # AnimPics_freeAll/AnimPics_resetForRoom were named. Finalizing it
    # now, plus its own helper sub_26C88. See
    # docs/overview.md#animpics_registerslot-named--the-last-piece-of-the-animpics-cluster. --

    (0x26D7E, "AnimPics_registerSlot",
     "sub_26D7E(picNumber, frameCount, loopDirection, duration): "
     "registers a new active animated-picture slot (capped at 5 via "
     "_animPicsSlotCount, frameCount capped at 20) -- allocates a "
     "handle (new_handle), clears frameCount Image records via the "
     "new Image_clearFrames, loads each frame via the already-named "
     "Image_load (bailing out and freeing everything if any frame "
     "fails to load), then stores frameCount/loopDirection/duration "
     "into the same per-slot table AnimPics_resetForRoom/sub_26F74 "
     "maintain and increments _animPicsSlotCount. Already fully "
     "characterized in the AnimPics_freeAll writeup; finalizing the "
     "name now."),
    (0x26C88, "Image_clearFrames",
     "sub_26C88(imgFarPtr, count): zeroes the first byte (the active/"
     "loaded flag, per the already-referenced img._active) of each of "
     "`count` consecutive Image-sized records -- the initialization "
     "counterpart to the already-named Image_freeFrames, used by "
     "AnimPics_registerSlot to blank a slot's frame table before "
     "loading real frames into it."),

    # -- twenty-seventh pass: sub_28BB7, confirmed directly by its body
    # as a full window-teardown function, distinct from the already-
    # named (lighter-weight) Window_close. See
    # docs/overview.md#window_destroy-named. --

    (0x28BB7, "Window_destroy",
     "sub_28BB7(windowNum): validates windowNum in [0,6) (no-op "
     "otherwise), then does full teardown: the already-named "
     "Window_close, releases any reserved regions "
     "(Windows_ReserveRegions(windowNum, 0)), zeroes "
     "Windows_x2[windowNum] (the per-slot 'in use' flag), rescans all "
     "6 slots to recompute Wndows_numWindows (highest active index + "
     "1), and clears Windows_activeWindow to -1 if this was the "
     "active window. A superset of Window_close, not a synonym for "
     "it."),

    # -- twenty-eighth pass: skipped sub_474F8 (10 callers, unresolved
    # far calls to segment 0x802, a jump into the middle of a local
    # label -- likely another corrupted-looking area near the
    # already-flagged sub_4A69F cluster) and sub_4A722 (9 callers, a
    # bare mid-function fragment with no prologue, same neighborhood).
    # Moved to sub_9E8DF (9 callers, 1007 bytes, reached via a real
    # thunk). Confirmed via TWO separate real call sites, each printing
    # a decoded GATESTR.DAT death message right before calling it (msgId
    # 0x41E: "you leap from the cliff walkway into the abyss" after a
    # JUMP-off-a-cliff easter egg; msgId 0x4008: killed by an axe blow
    # to the neck) as the player-death handler: shows a "you have died"
    # picture/message, then resets a large swath of Persisted_valNNN
    # globals and per-object handlers back to their initial values --
    # effectively restarting the game's state after death. See
    # docs/overview.md#game_restartafterdeath-named--the-player-death-handler. --

    (0x9E8DF, "Game_restartAfterDeath",
     "sub_9E8DF(): confirmed via two real call sites, each printing a "
     "decoded GATESTR.DAT death message immediately before calling it "
     "(msgId 0x41E, 'you leap from the cliff walkway into the abyss' "
     "-- a JUMP-off-a-cliff easter egg; msgId 0x4008, killed by an "
     "axe blow). Calls the still-unconfirmed sub_26F2A and the "
     "already-named AnimPics_freeAll, then -- gated on whether this is "
     "the player's first death or a repeat one (_deathCount, was "
     "word_CE8A8) -- either shows a 'you have died' picture directly "
     "or pauses (TextWindow_showMorePrompt) and routes through "
     "sub_15674 (a major hub function, not renamed) with a death "
     "picture/message pair. Afterward resets _roomLogicNum's handler, "
     "object 0x28's handler, and a large swath of Persisted_valNNN "
     "globals (95 through at least 124) plus roughly a dozen "
     "individual objects' handlers (logicNums 0x21-0x40) back to "
     "their initial values -- effectively restarting the game's state "
     "in place after the player dies, without a separate confirmation "
     "prompt visible in this function itself."),
    (0xCE8A8, "_deathCount",
     "Was word_CE8A8: incremented once per call to "
     "Game_restartAfterDeath; gates whether that function shows the "
     "death picture directly (repeat death) or pauses with a message "
     "first (first death) -- effectively a 'has the player died "
     "before' counter."),

    # -- twenty-ninth pass: sub_1019C, a trivial far-function-pointer
    # call trampoline. Confirmed directly by its body (8 bytes: push
    # bp, call [far ptr arg], pop bp, retf). See
    # docs/overview.md#invoke_callback-named. --

    (0x1019C, "invoke_callback",
     "sub_1019C(fnPtr): calls the far function pointer passed as its "
     "only argument, with no further arguments of its own -- a "
     "minimal indirect-call trampoline. 8 callers across unrelated "
     "functions confirm it's generic plumbing, not tied to one "
     "callback's specific signature beyond 'takes no arguments'."),

    # -- thirtieth pass: sub_1DD41, a trivial "append byte to buffer"
    # primitive -- but its caller context is a major new clue for the
    # still-open .MUS/MIDI unification thread. The un-named "seg024"
    # state machine that calls it (data-driven compiled code, not a
    # normal function boundary -- not itself renamed) branches on
    # incoming byte values 0xF0, 0xF2, 0xF3, 0xFA-0xFD, 0xFF -- exactly
    # the Standard MIDI status-byte ranges (SysEx start, Song Position
    # Pointer, Song Select, System Realtime Start/Continue/Stop,
    # Meta-event/Reset). See
    # docs/overview.md#midi_bufferbyte-named--a-midi-status-byte-state-machine-sighted. --

    (0x1DD41, "Midi_bufferByte",
     "sub_1DD41(): implicit-AL-argument primitive -- writes AL to "
     "*_midiBufferPos (was word_C8445) and advances the pointer. "
     "Trivial in isolation, but its only callers are all within one "
     "un-named data-driven state machine (labeled only as "
     "'seg024:XXXX' locations, not a normal function -- the same "
     "'compiled logic, not a clean function' shape seen elsewhere in "
     "this codebase) that branches on incoming byte values matching "
     "Standard MIDI status bytes almost exactly: 0xF0 (SysEx start), "
     "0xF2 (Song Position Pointer), 0xF3 (Song Select), a checked "
     "range 0xFA-0xFD (System Realtime Start/Continue/Stop), and 0xFF "
     "(Meta-event/Reset). Strongly suggests this state machine is the "
     ".MUS format's MIDI event-stream parser, buffering bytes via this "
     "function before eventual dispatch -- not fully unified with the "
     "Midi_sendByte/sub_1D966/sub_1EE70 cluster yet, but a significant "
     "new clue for that still-open thread."),
    (0xC8445, "_midiBufferPos",
     "Was word_C8445: a growing buffer-position pointer, reset to a "
     "fixed offset (0x527) elsewhere in the same un-named MIDI-status-"
     "byte state machine; advanced one byte at a time by "
     "Midi_bufferByte."),

    # -- thirty-first pass: sub_1ECDE, confirmed as a Standard MIDI
    # File variable-length-quantity (VLQ) decoder -- textbook shape
    # (accumulate 7 bits per byte, continue while the high bit is set).
    # This also finally confirms and names its helper sub_1ECB6
    # (already characterized, not renamed, in the Midi_sendByte pass).
    # See docs/overview.md#midi_readvarlengthvalue-named--a-midi-vlq-decoder-confirmed. --

    (0x1ECDE, "Midi_readVarLengthValue",
     "sub_1ECDE(trackIndex): decodes a Standard MIDI File "
     "variable-length quantity from track `trackIndex`'s current read "
     "position -- reads a byte via Midi_peekTrackByte(trackIndex, 0), "
     "accumulates its low 7 bits into a growing 32-bit value (shifting "
     "the accumulator left 7 bits each iteration), increments the "
     "track's position counter (word ptr [0xA1A4+trackIndex*2]) once "
     "per byte consumed, and continues while the byte's high bit "
     "(0x80) is set -- the textbook MIDI VLQ decode loop. Returns the "
     "low 16 bits of the decoded value in ax (word_D20F8); the high "
     "16 bits (word_D20FA) are available to callers that need the "
     "full 32-bit value."),
    (0x1ECB6, "Midi_peekTrackByte",
     "sub_1ECB6(trackIndex, byteOffset): reads a single byte at "
     "trackBase(trackIndex) + byteOffset, where trackBase is looked up "
     "from a small per-track table and added to a far pointer "
     "(_tmpSub._val10) to the loaded track data. Callers pass "
     "byteOffset=0 to read at the track's current position (advanced "
     "separately by Midi_readVarLengthValue's position counter) or a "
     "small positive offset to peek ahead at a just-identified fixed-"
     "format payload (e.g. the Midi_sendByte pass's sub_1EE70, reading "
     "a 3-byte tempo meta-event at offsets 2-4 without advancing). The "
     "per-track base-offset table itself wasn't "
     "renamed -- it's referenced via two different segment-relative "
     "names in different callers (-0x5E5Ch here vs. 0xA1A4 in "
     "Midi_readVarLengthValue) that may or may not be the same "
     "physical array; not confirmed either way."),
]


def resolve(ref):
    if isinstance(ref, str):
        ea = idc.get_name_ea_simple(ref)
        assert ea != idc.BADADDR, f"name not found: {ref!r}"
        return ea
    return ref


def main():
    print(f"DRY_RUN = {DRY_RUN}")
    for ref, new_name, note in RENAMES:
        ea = resolve(ref)
        old_name = idc.get_name(ea)
        if DRY_RUN:
            print(f"{ea:#x}: {old_name!r} -> {new_name!r}   ({note})")
        else:
            ok = idc.set_name(ea, new_name, idc.SN_NOCHECK)
            print(f"{ea:#x}: {old_name!r} -> {new_name!r}   ok={ok}")


main()
