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

    # -- thirty-second pass: skipped sub_2609A (8 callers, a graphics-
    # mode-only per-slot color/position setter with no message/string
    # confirmation available -- args plausible but not confident enough
    # to name). Moved to sub_2A933 (8 callers), called from the
    # already-named Surface_draw/Surface_draw2. Confirmed directly by
    # its body as a bounds-checked pixel-address computation. See
    # docs/overview.md#surface_getpixeloffset-named. --

    (0x2A933, "Surface_getPixelOffset",
     "sub_2A933(surface, x, y): returns a small negative error code "
     "(0xFFE4) if surface.field_0 is the 0xCA00 sentinel (a special/"
     "null surface), or (0xFFE5) if y or x are out of bounds against "
     "surface.field_A/field_C (height/width). Otherwise computes a "
     "byte offset from surface.field_12 (a packed bytes-per-line-"
     "shaped field, split into two bytes) and field_14, combining the "
     "x/y contributions into a 32-bit value, dividing by 16 (repacking "
     "into a paragraph-relative far offset), and adding "
     "surface._image (the surface's base far pointer) -- the core "
     "bounds-checked pixel/byte-address primitive underlying the "
     "already-named Surface_draw/Surface_draw2 drawing routines."),

    # -- thirty-third pass: sub_1D808/sub_1D84A, confirmed as the
    # textbook MPU-401 UART-mode command-and-acknowledge protocol --
    # another clean piece of the MIDI cluster, called (among others)
    # from sub_1EE70, already familiar from the Midi_sendByte/
    # Midi_readVarLengthValue passes. See
    # docs/overview.md#midi_sendcommand-named--the-mpu-401-commandack-protocol. --

    (0x1D808, "Midi_sendCommand_raw",
     "sub_1D808(): implicit-AH-argument primitive. Polls "
     "_midiStatusPort for 'not busy' (bit 0x40 clear) with a timeout "
     "(returns 0 on timeout); if ready, disables interrupts (cli), "
     "writes AH to _midiCommandPort (was word_C83AE -- confirmed the "
     "SAME physical port as _midiStatusPort, set alongside it in "
     "sub_1D966: MPU-401's status register doubles as the command "
     "register on write), then polls for a response: if a byte "
     "arrives and it's 0xFE (MPU-401's standard command-ACK byte), "
     "returns 1 (success); if it's anything else (real incoming MIDI "
     "data arriving mid-command), dispatches it through "
     "_midiDataCallback (was off_C83BD, a callback pointer configured "
     "elsewhere) and keeps waiting for the real ACK. Returns 0 if the "
     "ACK never arrives before timeout. Interrupts are restored via "
     "pushf/popf bracketing regardless of outcome."),
    (0x1D84A, "Midi_sendCommand",
     "sub_1D84A(command): normal-calling-convention wrapper -- loads "
     "`command` into AH and calls Midi_sendCommand_raw. Called from "
     "sub_1EE70 (the tempo-processing function from the Midi_sendByte "
     "pass) among others."),
    (0xC83AE, "_midiCommandPort",
     "Was word_C83AE: the MPU-401 command register -- confirmed the "
     "same physical port as _midiStatusPort (both set from the same "
     "base+1 value in sub_1D966), just used for writes (send a "
     "command) instead of reads (poll status)."),
    (0xC83BD, "_midiDataCallback",
     "Was off_C83BD: a far function pointer, configured elsewhere "
     "(sub_1D953) and invoked by Midi_sendCommand_raw whenever a "
     "non-ACK byte arrives while waiting for a command's "
     "acknowledgment -- the handler for real incoming MIDI data "
     "arriving out-of-band during command handshaking."),

    # -- thirty-fourth pass: sub_AB180, confirmed by decoding its real
    # GATESTR.DAT message (msgId 0x3C00, a Gateway-station corridor
    # description) as a "show this description only the first time"
    # helper, reused by several different rooms' compiled logic
    # (confirmed one real caller, sub_ABA21, reached via j_method233)
    # that apparently share this identical corridor flavor text. See
    # docs/overview.md#logics_describecorridoronce-named. --

    (0xAB180, "Logics_describeCorridorOnce",
     "sub_AB180(): if byte_CF114 is set, clears it and prints the "
     "decoded GATESTR.DAT message 0x3C00 -- 'This is one of the many "
     "corridors that wind their way through the occupied portions of "
     "Gateway...' -- via TextWindow_add; otherwise does nothing. A "
     "one-time-description gate shared by multiple different rooms' "
     "compiled logic (7 callers, including sub_ABA21 via the "
     "already-named j_method233), consistent with several physically "
     "distinct maze-like corridor rooms reusing identical flavor "
     "text and one shared first-visit flag."),

    # -- thirty-fifth pass: investigated but skipped sub_14A5F (6
    # callers, called from Logics_checkMoveRestriction -- a generic
    # "print object header, invoke its logic for an action" dispatcher
    # with several unclear pieces: j_scene_update? and thunk_sub_669E3
    # weren't traced, and the exact verb/action semantics stayed
    # ambiguous; not renamed) and sub_1A0FC (6 callers, another
    # "sp-analysis failed" mid-function fragment, a separate instance
    # of the same disassembly-boundary problem seen elsewhere in this
    # binary). Moved to sub_1D732, which resolved into a clean
    # continuation of the already-sighted OPL2/AdLib subsystem: it
    # writes the OPL2's real hardware register 0xBD (AM depth/Vibrato
    # depth/Rhythm mode enable/rhythm instrument bits), built from four
    # flag globals. See
    # docs/overview.md#opl2_writerhythmregister-named--the-opl2-subsystem-grows. --

    (0x1D732, "Opl2_writeRhythmRegister",
     "sub_1D732(): builds a byte from four flag globals -- bit 0x80 "
     "from _opl2TremoloDepth (was byte_D1C52), bit 0x40 from "
     "_opl2VibratoDepth (was byte_D1C58), bit 0x20 from "
     "_opl2RhythmEnabled (was byte_D1C53), and the low bits directly "
     "from _opl2RhythmInstruments (was byte_D1C59) -- then writes it "
     "via the already-named Opl2_writeRegister(0xBD, value). OPL2's "
     "real hardware register 0xBD is exactly this: bit 7 tremolo (AM) "
     "depth, bit 6 vibrato depth, bit 5 rhythm-mode enable, and bits "
     "4-0 individual rhythm-instrument enables (bass drum/snare/tom-"
     "tom/cymbal/hi-hat) -- an exact match confirming this is real "
     "OPL2 hardware programming, not a coincidental register number."),
    (0xD1C52, "_opl2TremoloDepth",
     "Was byte_D1C52: boolean flag contributing OPL2 register 0xBD's "
     "bit 0x80 (tremolo/AM depth) via Opl2_writeRhythmRegister."),
    (0xD1C58, "_opl2VibratoDepth",
     "Was byte_D1C58: boolean flag contributing OPL2 register 0xBD's "
     "bit 0x40 (vibrato depth) via Opl2_writeRhythmRegister."),
    (0xD1C53, "_opl2RhythmEnabled",
     "Was byte_D1C53: boolean flag contributing OPL2 register 0xBD's "
     "bit 0x20 (rhythm mode enable) via Opl2_writeRhythmRegister."),
    (0xD1C59, "_opl2RhythmInstruments",
     "Was byte_D1C59: bitmask contributing OPL2 register 0xBD's low 5 "
     "bits directly (individual rhythm-instrument enables: bass drum/"
     "snare/tom-tom/cymbal/hi-hat) via Opl2_writeRhythmRegister."),

    # -- thirty-sixth pass: skipped sub_1E2E3 (single NOP byte, a
    # tail-merge artifact, not a real function) and sub_1E315 (another
    # "sp-analysis failed" chunked fragment in the same seg098
    # neighborhood). Moved to sub_255A8, confirmed as a case-
    # insensitive prefix-match string comparison -- the classic parser
    # "does this abbreviation match this vocabulary word" check. This
    # also confirms its character-normalization helper sub_1AECE. See
    # docs/overview.md#vocab_matchesabbreviation-named--a-parser-abbreviation-matcher. --

    (0x255A8, "Vocab_matchesAbbreviation",
     "sub_255A8(word, abbrev): walks `abbrev` while it's non-null, "
     "comparing each character (case-normalized via the new "
     "Char_toLower) against the corresponding character of `word`; "
     "advances both pointers on a match, stops on a mismatch or when "
     "`abbrev` is exhausted. Returns 1 only if every character of "
     "`abbrev` matched -- i.e. `word` starts with `abbrev` -- 0 "
     "otherwise. The classic parser 'does the player's typed "
     "abbreviation match this longer vocabulary word' check (e.g. "
     "'n' matching 'north')."),
    (0x1AECE, "Char_toLower",
     "sub_1AECE(char): looks up a per-character-code table (a "
     "ctype-style flag table at char+0x3707) checking bit 0x1 ('is "
     "uppercase'); if set, adds 0x20 to lowercase it (standard ASCII "
     "case-fold). Returns the character unchanged otherwise. A "
     "generic character-normalization utility, also used directly by "
     "sub_204CE (not renamed)."),

    # -- thirty-seventh pass: sub_27C31, confirmed via the already-
    # named global `button_strings` array as the on-screen clickable-
    # icon-button drawing function -- draws a 3D-beveled button (light/
    # dark gray fillRect + drawLine border) with an icon image inside,
    # positioned from a per-state rectangle-bounds table, and returns
    # the button's associated vocabulary/label string. Called from
    # Dialog_prompt/get_mouse_input/prompt_for_filename/Commnet_proc1
    # -- the mouse-driven icon toolbar (e.g. a compass rose) used
    # alongside the parser's text input. See
    # docs/overview.md#icon_drawbutton-named. --

    (0x27C31, "Icon_drawButton",
     "sub_27C31(buttonIndex, stateArray): looks up buttonIndex's "
     "current state byte from stateArray (a per-icon status/highlight "
     "value), uses it to index a 4-parallel-word rectangle-bounds "
     "table (x1/y1/x2/y2-shaped) for this button's screen position, "
     "loads and draws the button's icon image (Image_Init/"
     "Image_drawAt), then -- in graphics mode -- draws a 3D-beveled "
     "border around it (light-gray/dark-gray fillRect and "
     "Screen_drawLine calls, the classic raised-button look). Returns "
     "the button's associated string from the already-named "
     "button_strings[stateByte] -- the vocabulary word or label "
     "corresponding to this icon/state, handed back to the caller "
     "(e.g. to feed into the parser as if typed). Confirmed generic "
     "via its callers: Dialog_prompt, get_mouse_input, "
     "prompt_for_filename, Commnet_proc1 -- all part of the mouse-"
     "driven icon toolbar (e.g. a compass rose) used alongside the "
     "text parser."),

    # -- thirty-eighth pass: sub_30D4F, a genuine RTLink-thunk-shaped
    # function ("call rtlink_thunk; jmp <target>") that the earlier
    # batch rtlink-thunk rename passed over -- IDA merged its jmp
    # target's code back in as a "FUNCTION CHUNK" of this same
    # function, which made the is_rtlink_thunk() same-function check in
    # find_rtlink_thunks.py/rank_unnamed_functions.py treat it as a
    # split-body false negative rather than a real thunk. Renamed by
    # hand following the established thunk_<target> convention rather
    # than re-running the whole batch script for one straggler. See
    # docs/overview.md#thunk_sub_5d9f3-named--a-straggler-rtlink-thunk-caught. --

    (0x30D4F, "thunk_sub_5D9F3",
     "sub_30D4F: call rtlink_thunk; jmp loc_5DFE6 (inside sub_5D9F3, "
     "itself not renamed). A genuine RTLink call-site thunk, missed by "
     "the earlier batch pass because IDA's own function-chunk merging "
     "made it look like a same-function tail chunk to the automated "
     "same-function-start check."),
    (0x3119B, "thunk_sub_5D9F3_2",
     "sub_3119B: same shape and same target (sub_5D9F3, via "
     "loc_5E049) as thunk_sub_5D9F3 above -- a second call-site thunk "
     "to the same target from a different overlay segment (the usual "
     "one-thunk-per-caller-segment pattern already documented for "
     "this codebase's RTLink thunks). Suffixed _2 since IDA requires "
     "unique names and thunk_sub_5D9F3 is already taken."),

    # -- fortieth pass: sub_2A90E, a low-level far-pointer segment-
    # overflow fixup utility. Confirmed by its unusual implicit
    # calling convention (no formal stack args -- takes an ax:dx far
    # pointer and tests the CPU carry flag as set by the caller's
    # preceding arithmetic) and its callers, all in the same graphics/
    # Surface-drawing neighborhood as the already-named
    # Surface_getPixelOffset. See
    # docs/overview.md#surface_advancesegmentoncarry-named. --

    (0x2A90E, "Surface_advanceSegmentOnCarry",
     "sub_2A90E(ax=offset, dx=segment, implicit CF from caller's "
     "preceding add): the classic 8086 far-pointer-offset-overflow "
     "fixup -- if the caller's arithmetic on the offset carried "
     "(CF set), adds 0x1000 to ES (the segment half of the ax:dx "
     "pair as loaded into es:di); otherwise adds 0x1000 to DS "
     "instead. This is the standard 'advance to the next 64KB bank' "
     "segment adjustment needed after adding to a raw video-memory/"
     "picture-buffer offset that might cross a 64KB boundary. Callers "
     "(sub_17D6A/sub_17E31/sub_17FB8/sub_2AA24/sub_2B6D4/sub_2C42A, "
     "none renamed) sit in the same graphics/Surface-drawing "
     "neighborhood as the already-named Surface_getPixelOffset."),

    # -- forty-first pass: sub_A8577, confirmed by decoding all six of
    # its real GATESTR.DAT messages as the "consequences of firing a
    # weapon" handler -- Gateway station bans weapons, so shooting one
    # (successfully or not) triggers arrest, a fine, or death depending
    # on the outcome. This also confirms the 32-bit player-money field
    # it reads/writes. See
    # docs/overview.md#game_handleweapondischarge-named--the-consequences-of-firing-a-gun. --

    (0xA8577, "Game_handleWeaponDischarge",
     "sub_A8577(outcomeType): dispatches on outcomeType (0-4), each "
     "branch loading a distinct .RS sound file (Stream_loadFile) and "
     "printing a distinct decoded GATESTR.DAT message: 1 = a fatal "
     "hit ('...you are riddled with energy bolts and bullets', msgId "
     "0x1CCF) leading straight into Game_showEndingMessage; 3/4 = a "
     "miss that ricochets dangerously vs. safely (0x1CD0/0x1CD1); "
     "0/2 = a miss witnessed by soldiers, leading to arrest -- if the "
     "player can't afford the 1000-credit fine (checked against the "
     "32-bit _playerCreditsLo/_playerCreditsHi pair), they're executed "
     "by expulsion into vacuum (0x1CD3 + Game_showEndingMessage); "
     "otherwise they pay the fine (0x1CD4, credits -= 1000), have "
     "their weapon confiscated (a handler update on logicNum 0x14C, "
     "word_CB808 set to 0x1Eh -- not itself renamed), and a follow-up "
     "event is queued via the already-named Queue_exists/Queue_find/"
     "Queue_add. The classic 'weapons are illegal on Gateway station' "
     "consequence path for the FIRE/SHOOT command."),
    (0xCF34C, "_playerCreditsHi",
     "Was word_CF34C: high word of the 32-bit player-credits/money "
     "field (SaveField <Persisted_val213, 4> confirms the pair is "
     "saved as one 4-byte field). Added to/subtracted from at several "
     "points in the game (earning and spending money); checked in "
     "Game_handleWeaponDischarge against the 1000-credit weapons "
     "fine."),
    (0xCF34A, "_playerCreditsLo",
     "Was Persisted_val213: low word of the 32-bit player-credits/"
     "money field, paired with _playerCreditsHi."),

    # -- forty-second pass: sub_12FC3, called directly from main()'s
    # main game loop right before _turnCount is incremented. Its body
    # shares function chunks with sub_130D6 (itself "sp-analysis
    # failed") and both interleave with the already-named _queueCount/
    # Queue_add/Queue_remove/Queue_find machinery and the already-named
    # waitMsg/j_continue_waiting ("Do you want to continue waiting?")
    # -- confirming this whole cluster is the turn-advance / scheduled-
    # event-queue-processing loop, reused both once per normal turn and
    # repeatedly as the inner loop of the WAIT command. Only sub_12FC3
    # itself renamed this pass; sub_130D6 and the deeper queue-item
    # countdown mechanics (including word_CB808's exact relationship to
    # the earlier-named weapon-confiscation countdown) are left for a
    # future pass rather than guessed under this much interlocking
    # complexity. See
    # docs/overview.md#queue_processturn-named--the-turnwait-event-queue-loop-sighted. --

    (0x12FC3, "Queue_processTurn",
     "sub_12FC3(prevValue, currentValue): called from main() right "
     "before _turnCount is incremented -- i.e. once per game turn. "
     "Gates on a one-shot skip flag (byte_CB7F2, not renamed), then "
     "either fires action 0x1B on the current room (Logic_call) or "
     "falls through into a shared chunk (also reached from sub_130D6) "
     "that walks the same 4-byte-entry scheduled-event table the "
     "already-named Queue_add/Queue_remove/Queue_find operate on, "
     "bounded by the already-named _queueCount. That shared code also "
     "references the already-named waitMsg ('Do you want to continue "
     "waiting?') and j_continue_waiting, confirming this is the "
     "per-turn queued-event-processing loop reused as the WAIT "
     "command's inner loop. word_CB7F6/word_CB808 (not renamed) look "
     "like a queue-walk index and a countdown value respectively, but "
     "their precise roles -- and word_CB808's relationship to the "
     "weapon-confiscation countdown from the "
     "Game_handleWeaponDischarge pass -- weren't nailed down this "
     "pass."),

    # -- forty-third pass: sub_186F0/sub_186B2/sub_186D4/sub_18682, a
    # complete and textbook-exact Sound Blaster DSP detection sequence
    # -- a fourth sound-hardware backend (alongside the already-named
    # PC-speaker, MPU-401/MIDI, and OPL2/AdLib clusters), reached from
    # the already-named Stream_selectHandler. See
    # docs/overview.md#sb_detectdsp-named--a-fourth-sound-backend-sound-blaster. --

    (0x186F0, "Sb_writeByte",
     "sub_186F0(AL=byte): polls the DSP write-status port (base+0xC) "
     "for bit 7 clear (not busy) with a timeout, does the standard "
     "short I/O delay (four `jmp $+2`), then writes the byte to that "
     "same port -- textbook Sound Blaster DSP write protocol (the "
     "write-status and write-command registers share one port "
     "address). Returns via carry flag (clear = success, set = "
     "timeout)."),
    (0x186D4, "Sb_readByte",
     "sub_186D4(): polls the DSP read-buffer-status port (base+0xE) "
     "for bit 7 set (data available) with a timeout, then reads the "
     "byte from the DSP read-data port (base+0xA) -- textbook Sound "
     "Blaster DSP read protocol. Returns via carry flag."),
    (0x186B2, "Sb_resetDsp",
     "sub_186B2(): the exact standard Sound Blaster DSP reset "
     "sequence -- writes 1 to the reset port (base+6), busy-waits "
     "~256 iterations, writes 0 to release reset, then polls (via "
     "Sb_readByte, up to 32 tries) for the DSP to respond with the "
     "magic byte 0xAA confirming it's alive. An exact match for the "
     "well-documented SB reset protocol -- conclusive proof this "
     "cluster is real Sound Blaster (or 100%-compatible) hardware "
     "detection, not a coincidental register shape."),
    (0x18682, "Sb_detectDsp",
     "sub_18682(): calls Sb_resetDsp, then performs the standard SB "
     "'DSP Identification' compatibility test: writes command 0xE0 "
     "then test byte 0xC6 via Sb_writeByte, reads back via "
     "Sb_readByte, and checks the result equals 0x39 -- the exact "
     "bitwise complement of 0xC6, confirming a genuine/compatible DSP "
     "chip. On success calls sub_18963(1) (not renamed, plausibly "
     "'mark Sound Blaster detected'). Reached from the already-named "
     "Stream_selectHandler -- Sound Blaster digital-audio output is a "
     "fourth sound-hardware backend this engine supports, alongside "
     "the PC-speaker, MPU-401/MIDI, and OPL2/AdLib clusters already "
     "documented."),
    (0xC84F3, "_sbBasePort",
     "Was word_C84F3: the Sound Blaster DSP base I/O port (typically "
     "0x220 on real hardware), used as the base for all of "
     "Sb_resetDsp/Sb_readByte/Sb_writeByte/Sb_detectDsp's fixed port "
     "offsets (+6 reset, +0xA read-data, +0xC write-status/command, "
     "+0xE read-status)."),

    # -- forty-fourth pass: sub_1D8CB, the MPU-401 shutdown/uninstall
    # function -- the exact reverse of sub_1D966's IRQ-driven install
    # (restores the original interrupt vector sub_1D966 saved, remasks
    # the IRQ, sends MPU-401's standard 0xFF reset command via the
    # already-named Midi_sendCommand_raw). This finally lets the
    # long-flagged "startGame?" mislabeling (see the sound-track-
    # selection-subsystem-sighted section several passes back) be
    # corrected for real: its full body is now unmistakably "stop the
    # currently playing track", dispatching to whichever backend is
    # active and resetting exactly the same buffer-bookkeeping tail
    # already seen in sub_15DB2 -- which can now also be confidently
    # renamed as the track-selection function that calls it. See
    # docs/overview.md#sound_stoptrack-named--the-startgame-mislabeling-finally-corrected. --

    (0x1D8CB, "Midi_shutdown",
     "sub_1D8CB(): the exact reverse of sub_1D966's install -- if "
     "byte_C83B6 (the 'initialized' flag sub_1D966 sets) is 1: "
     "restores the original 8259 IRQ mask, sends MPU-401's standard "
     "reset command 0xFF via Midi_sendCommand_raw, and restores the "
     "original interrupt vector (dword_C83B9, saved by sub_1D966 "
     "before installing its own handler) via DOS INT 21h AH=25h. "
     "Always clears byte_C83B6 to 0 at the end."),
    (0x1D966, "Midi_initDevice",
     "sub_1D966(basePort, irqLine): sets up _midiDataPort/"
     "_midiCommandPort/_midiStatusPort from basePort, installs a DOS "
     "interrupt vector at IRQ+8 with 8259 unmasking, and sets "
     "byte_C83B6=1 on success. The install-side counterpart to the "
     "new Midi_shutdown; already characterized (but not renamed) in "
     "the Midi_sendByte pass -- finalizing the name now that its "
     "shutdown counterpart makes the whole picture unambiguous."),
    (0x20220, "Sound_stopTrack",
     "Renaming the long-flagged mislabeled tentative name for real. "
     "Full body confirms: only proceeds if word_C8582's streaming-"
     "active bits are set AND the given trackId (arg_0) matches the "
     "currently-playing track (word_C8580) or is 0xFFFF ('stop "
     "whatever's playing'). Waits for the active buffer to drain, "
     "then dispatches a backend-specific stop based on word_C8582's "
     "mode bits -- MIDI via the new Midi_shutdown, or one of two other "
     "not-yet-renamed backend stop routines (sub_1E974/sub_1F910) -- "
     "clears the backend-selector bits, resets the current-track "
     "globals, then falls into an UNCONDITIONAL tail (reached even if "
     "the initial guards failed) that frees 4 buffer handles plus one "
     "more and resets every Stream-buffer bookkeeping global to its "
     "idle state -- the exact same reset sequence already observed at "
     "the tail of sub_15DB2 (now renamed Sound_selectTrack below). "
     "This is 'stop the currently playing sound/music track', not "
     "anything related to starting a new game."),
    (0x15DB2, "Sound_selectTrack",
     "Finalizing the name floated (but not applied) several passes "
     "ago in the sound-track-selection-subsystem writeup, now that "
     "Sound_stopTrack's real role is confirmed: sub_15DB2 calls "
     "Sound_stopTrack(0xFFFF) (unconditional stop) before loading and "
     "starting a new track -- 'change the currently playing sound/"
     "music track'. sub_15F35 (the two-resource-variant lookup "
     "helper) still not renamed."),

    # -- forty-fifth pass: sub_1FDB8, called from gatemain_start --
    # the sound-device SELECTION dispatcher (matching the soundMode
    # command-line argument from the cross-IDB argv-mapping finding).
    # Confirmed via its two callees, sub_1CD54 (sets the already-named
    # _opl2BasePort to the real 0x388 before probing) and sub_1FA5E
    # (calls the already-named Midi_initDevice then immediately
    # Midi_shutdown -- a detect-only probe pattern). See
    # docs/overview.md#sound_selectdevice-named--the-sound-mode-dispatcher-confirmed. --

    (0x1FDB8, "Sound_selectDevice",
     "sub_1FDB8(mode, midiBasePort, midiIrq): stores midiBasePort/"
     "midiIrq into _midiBasePortConfig/_midiIrqConfig, then dispatches "
     "on `mode` (masked to drop bit 3, a separate flag): mode 1 or 2 "
     "probes for OPL2/AdLib via the new Opl2_detectAndInit; mode 4 "
     "probes for MPU-401/MIDI via the new Midi_detectDevice, and on "
     "success also prints the game's title ('       Gateway      ') "
     "and calls two more setup routines (sub_1FB56/sub_1FC70, not "
     "renamed). Sets word_C8582 flag bits recording which backend "
     "probe succeeded (value 4 = MIDI, matching Sound_stopTrack's own "
     "test of that same bit; value 2 = OPL2/AdLib). This is the "
     "command-line soundMode argument's device-selection entry "
     "point."),
    (0x1CD54, "Opl2_detectAndInit",
     "sub_1CD54(): sets _opl2BasePort to the real 0x388, then calls "
     "sub_1CD72 (the actual presence-detection read, not renamed) "
     "followed by sub_1CE20 (plausibly a channel-silencing init, not "
     "renamed). Called from Sound_selectDevice for mode 1/2."),
    (0x1FA5E, "Midi_detectDevice",
     "sub_1FA5E(): calls the already-named Midi_initDevice(basePort, "
     "irqLine) then immediately Midi_shutdown() again -- a detect-"
     "only probe: initialize just long enough to see if the hardware "
     "responds, capture the result, then tear it back down rather "
     "than leaving it running. Called from Sound_selectDevice for "
     "mode 4, using the config values it just stored."),
    (0xC8586, "_midiBasePortConfig",
     "Was word_C8586: the configured MPU-401 base port (e.g. from a "
     "command-line argument or config value), passed to "
     "Midi_initDevice by Midi_detectDevice -- distinct from the "
     "already-named _midiDataPort, which is the port actually in use "
     "once initialized."),
    (0xC8588, "_midiIrqConfig",
     "Was word_C8588: the configured MPU-401 IRQ line, passed to "
     "Midi_initDevice by Midi_detectDevice alongside "
     "_midiBasePortConfig."),

    # -- forty-sixth pass: sub_5C91C, confirmed via the already-named
    # aaInputPrompt/get_input_line_ptr/Commset_winContent as the input-
    # line redraw routine. See
    # docs/overview.md#inputwindow_redrawpromptline-named. --

    (0x5C91C, "InputWindow_redrawPromptLine",
     "sub_5C91C(): writes the already-named aaInputPrompt string (the "
     "parser's prompt, e.g. a leading character) followed by the "
     "current input buffer's text (get_input_line_ptr) into the "
     "content window (TextWindow_addDirect, Commset_winContent) -- "
     "redrawing the prompt-plus-typed-so-far line. Then, if mouse "
     "input mode is off, hides the mouse cursor; otherwise shows it "
     "(waiting for button release first if a button was already "
     "held)."),

    # -- forty-seventh pass: sub_204CE, byte-for-byte the same
    # algorithm as the already-named Vocab_matchesAbbreviation, but
    # compiled separately in the startup/config-parsing area (seg029,
    # right next to Sound_selectDevice/Opl2_detectAndInit/
    # Midi_detectDevice, and immediately preceded by sub_20448, a
    # hex-digit-string parser -- consistent with parsing a BLASTER-
    # style environment/command-line config string, e.g. sound card
    # base address/IRQ/DMA settings). Named generically rather than
    # with the game-vocabulary-specific "Vocab_" prefix since this
    # instance operates on config text, not parser vocabulary. See
    # docs/overview.md#string_matchesprefixci-named. --

    (0x204CE, "String_matchesPrefixCI",
     "sub_204CE(word, abbrev): identical algorithm to the already-"
     "named Vocab_matchesAbbreviation (same case-insensitive prefix "
     "check via Char_toLower, same 0/1 sentinel-byte return "
     "convention) but a separate compiled copy used in the startup/"
     "sound-config-parsing area, immediately after sub_20448 (a hex-"
     "digit-string parser, not renamed) -- consistent with parsing a "
     "BLASTER-style config string rather than in-game parser "
     "vocabulary."),

    # -- forty-eighth pass: sub_249FF, confirmed directly by its body
    # (including IDA's own "Reset mouse driver" comment on the INT 33h
    # call) as the mouse subsystem's shutdown/reset routine, using
    # several already-named Mouse_* helpers. See
    # docs/overview.md#mouse_shutdown-named. --

    (0x249FF, "Mouse_shutdown",
     "sub_249FF(): if mouseState bit 0x38 is set, hides the mouse "
     "(Mouse_Hide), resets the mouse driver (INT 33h AX=0 -- IDA's "
     "own comment already reads 'Reset mouse driver'), and restores "
     "the cursor range (set_mouse_range). If mouseState bit 0xC is "
     "set, frees mouse resources (Mouse_free) and calls sub_24A42 "
     "(not renamed, also called from the already-named Mouse_init -- "
     "a shared init/shutdown helper)."),

    # -- forty-ninth pass: sub_2661C, a one-shot "load a picture frame,
    # draw it, then free it" utility -- confirmed directly by its body
    # (Image_load -> draw helper -> Image_Free), matching the
    # established load_and_scale_pic naming convention for this small
    # cluster of picture-loading entry points. See
    # docs/overview.md#load_and_draw_pic-named. --

    (0x2661C, "load_and_draw_pic",
     "sub_2661C(x, y, picNumber, frameNumber): loads the given "
     "picture frame into a temporary local Image (the already-named "
     "Image_load), returning 0 immediately if that fails; otherwise "
     "calls sub_2666E(x, y, img) (not renamed -- its own body's exact "
     "coordinate/offset handling wasn't fully unpicked, but it "
     "eventually calls sub_2B6D4, a drawing/blit routine) to draw it "
     "at the given position, frees the temporary Image (Image_Free), "
     "and returns 1. Named to match the established "
     "load_and_scale_pic convention for this small cluster of one-"
     "shot picture-loading entry points."),

    # -- fiftieth pass: sub_791E2, confirmed via its real GATESTR.DAT
    # message and the already-recognized lowercase direction-name
    # string constants (aNorth/aSouth/aEast/aWest etc., at addresses in
    # the same seg086) as the pond room's detailed reflection/shore
    # description generator. See
    # docs/overview.md#logics_describepondview-named. --

    (0x791E2, "Logics_describePondView",
     "sub_791E2(directionIndex): looks up directionIndex (0-4) in a "
     "5-entry table to find matching shore/direction data, then "
     "prints the decoded GATESTR.DAT message 0x4824 -- \"You're "
     "standing on the %sern shore of the pond. %slight gently "
     "reflects off the calm %s surface of the pond. A leaf "
     "occasionally falls into the pond to the %s and causes a small "
     "ripple, distorting the %s reflection.\" -- filling its five %s "
     "placeholders from: a direction name (the already-recognized "
     "aNorth/aSouth/aEast/aWest/aNortheast/etc. constants, confirming "
     "the table holds direction-string references), and, gated on "
     "Persisted_val183 (a day/night flag), either 'Sun'/sun-flavored "
     "text or a 'Moon'/moon-flavored equivalent (including the "
     "literal string aSunS = 'sun's' for the final possessive "
     "reflection phrase when it's daytime and the direction matches). "
     "A single room's detailed environmental description generator, "
     "not a generic utility."),

    # -- fifty-first pass: sub_80894, confirmed via three real decoded
    # GATESTR.DAT messages as a "hostile beast notices you" encounter
    # handler shared by a cluster of 4 related rooms (_roomLogicNum
    # 0x98-0x9B), with a shard-based distraction mechanic. See
    # docs/overview.md#logics_describebeastapproach-named. --

    (0x80894, "Logics_describeBeastApproach",
     "sub_80894(arg_0): the beast-encounter turn handler for a cluster "
     "of 4 related rooms (_roomLogicNum 0x98-0x9B). Prints a room-"
     "specific 'beast notices you' message (msgId 0x508C). Then, "
     "gated on Persisted_val177: if SET, prints msgId 0x5091 ('He "
     "immediately becomes transfixed by the light reflecting off the "
     "...crystal shard and stands motionless'), sets Persisted_val178 "
     "= 1, clears a bit on object 0x9D, and Queue_removes item 3 -- "
     "the shard is holding the beast at bay. If CLEAR, prints msgId "
     "0x508D ('You freeze...') and (except in room 0x98) 0x508E ('a "
     "glint from the...shard...crosses his face. He stops dead...then "
     "resumes walking toward you.') followed by 0x508F ('He grabs "
     "you...and slams you head first into the %s wall.') and "
     "Queue_adds item 0x15 -- the beast attacks; OR, specifically in "
     "room 0x98, prints the successful-escape resolution msgId 0x5090 "
     "('...he becomes momentarily motionless...lets out a deafening "
     "shriek...quickly exits the clearing') and updates object 0x9D's "
     "handler, clears Persisted_val182, and Queue_adds item 3. "
     "Confirms a full crystal-shard-based deterrence puzzle against a "
     "hostile creature, with distinct held/clean-shard vs. mud-"
     "covered-shard vs. no-shard outcomes."),

    # -- fifty-second pass: sub_130D6, the companion function flagged
    # for follow-up when Queue_processTurn was named -- a multi-chunk,
    # "sp-analysis failed" function, but its mechanism is now clear
    # from reading both chunks: it's the actual countdown-queue tick
    # that Queue_processTurn falls into. See
    # docs/overview.md#queue_tickcountdowns-named--closing-the-loop-on-queue_processturn. --

    (0x130D6, "Queue_tickCountdowns",
     "sub_130D6(): walks the same _queueCount-bounded, 4-byte-entry "
     "scheduled-event table Queue_add/Queue_remove/Queue_find/"
     "Queue_processTurn operate on, decrementing each entry's "
     "countdown value; when an entry's countdown reaches zero, "
     "memmove-compacts it out of the table (the same removal shape as "
     "Queue_remove, just triggered by expiry rather than an explicit "
     "call) and applies its associated logicNum-8/index-3 update via "
     "the already-named Logics_updateHandler-adjacent machinery. After "
     "a full pass, if a caller-supplied flag indicates this is running "
     "as part of the WAIT command (and other conditions on the "
     "current wait count are met), checks byte_CC530 and -- if set -- "
     "calls the already-named j_scene_update? then "
     "j_continue_waiting(waitMsg) to ask 'Do you want to continue "
     "waiting?'. This confirms and closes the loop on the mechanism "
     "flagged (but not fully named) when Queue_processTurn was named: "
     "the turn-advance and WAIT-command loops share this exact "
     "countdown-tick code."),

    # -- fifty-third pass: sub_15470, a thin wrapper always invoking
    # the still-unnamed sub_14A5F's action 8 on the current room,
    # called directly from main() and the already-named show_startup.
    # See docs/overview.md#logics_lookatcurrentroom-named. --

    (0x15470, "Logics_lookAtCurrentRoom",
     "sub_15470(): calls sub_14A5F(_roomLogicNum, action=8) (sub_14A5F "
     "itself not renamed -- a generic 'print object header, invoke "
     "its logic for an action' dispatcher whose exact verb semantics "
     "vary per call site) and always returns 1. Called directly from "
     "main() and the already-named show_startup -- consistent with "
     "action 8 being 'describe/look at this room', invoked once at "
     "game startup and once per the main loop's LOOK-equivalent "
     "point."),

    # -- fifty-fourth pass: sub_15AD8, a small save/restore mechanism
    # for one specific object's handler, keyed by a caller-supplied
    # context value against a 4-slot table. Confirmed mechanically by
    # direct read; the exact significance of the hardcoded object
    # (logicNum 0x71) and handler index (1) wasn't determined (no
    # message/string anchor available, single caller sub_9478A via
    # j_method074, not itself investigated). See
    # docs/overview.md#logics_saveorrestorehandler-named. --

    (0x15AD8, "Logics_saveOrRestoreHandler",
     "sub_15AD8(key, mode): finds (or allocates, up to 4 slots) a "
     "slot matching `key` in a small table. If mode==0 (save): reads "
     "object 0x71's handler-index-1 value (Logics_getUnkHandler) and "
     "stores it into the slot. If mode!=0 (restore): reads the "
     "previously-stored value back out of the slot and writes it back "
     "via Logics_setUnkHandler -- a push/pop-style save-and-restore of "
     "one object's handler state, keyed by caller context. The "
     "specific significance of logicNum 0x71/handler index 1 wasn't "
     "determined -- no message-string anchor was available, and its "
     "one caller (sub_9478A, reached via j_method074) wasn't itself "
     "investigated."),

    # -- fifty-fifth pass: sub_15BDA, called directly from main() and
    # show_startup -- confirmed as the room-to-background-music mapping
    # entry point, via a real call to the already-named Sound_stopTrack
    # and a body that otherwise duplicates Sound_selectTrack's own tail
    # logic (same word_C856E/word_C8580/word_C857A/word_C857E globals,
    # same +0x3D8E table, same sub_15F35 helper calls). See
    # docs/overview.md#sound_selecttrackforroom-named. --

    (0x15BDA, "Sound_selectTrackForRoom",
     "sub_15BDA(roomNum): looks up roomNum in a 106-entry per-room "
     "table to find its music config (section id, track id, a "
     "duration/volume-ish byte, and a flag byte); if the found "
     "section/track already match what's currently playing "
     "(word_C856E/word_C8580), does nothing. Otherwise calls the "
     "already-named Sound_stopTrack(0xFFFF) and then runs essentially "
     "the same track-loading tail already documented in "
     "Sound_selectTrack (same globals, same sub_15F35 resource-variant "
     "lookup) to start the new room's music. The room-to-music "
     "mapping entry point, called directly from main() and the "
     "already-named show_startup."),

    # -- fifty-sixth pass: sub_16978, confirmed directly by its body
    # (using the already-named Windows_currentWindow/Listbox_draw) as
    # the "switch active window" primitive, returning the previous
    # window for save/restore use. See
    # docs/overview.md#windows_setcurrentwindow-named. --

    (0x16978, "Windows_setCurrentWindow",
     "sub_16978(windowNum): saves the current Windows_currentWindow "
     "to return later. If windowNum is valid (>=0) and different from "
     "the current window, redraws the current listbox as deselected "
     "(Listbox_draw(0)) and sets Windows_currentWindow = windowNum. "
     "Always returns the PREVIOUS current window number, letting "
     "callers temporarily switch windows and restore the old one "
     "afterward."),

    # -- fifty-seventh pass: sub_17A12/sub_17A19, a listbox nested-
    # state stack -- confirmed via the immediately-following function's
    # body, which pushes the current listbox's items/divider-index onto
    # a 20-deep stack (indexed by the same word_D0766 this function
    # resets to 0) before calling the already-named Listbox_reset with
    # new items. See docs/overview.md#listbox_resetstatestack-named. --

    (0x17A12, "Listbox_resetStateStack",
     "sub_17A12(): sets word_D0766 (the listbox nested-state stack's "
     "depth counter) to 0. Called from several already-named entry "
     "points (Events_waitForPress, InputWindow_getLine, Scene_draw) -- "
     "consistent with clearing any nested-dialog stack state at the "
     "start of a fresh top-level input/display session."),
    (0x17A19, "Listbox_pushState",
     "sub_17A19(winNumber, items, dividerIndex): saves winNumber's "
     "current items (Listbox_getItems), divider index "
     "(Listbox_getDividerIndex), and a value from sub_16B01 (not "
     "renamed) into a 20-deep stack indexed by word_D0766, increments "
     "the stack depth, then calls the already-named Listbox_reset "
     "with the new items/dividerIndex -- pushing the current listbox "
     "state before replacing it, i.e. opening a nested listbox/menu "
     "on top of the current one. Bails out early (no-op) if the "
     "stack is already full (word_D0766 == 0x14)."),

    # -- fifty-eighth pass: sub_18842, confirmed directly by its body
    # (PIC end-of-interrupt out 20h,20h + iret, full register
    # save/restore) as a real hardware interrupt service routine --
    # continuing the "digitized PC-speaker sound-effect engine" thread
    # from several passes ago, whose self-modifying ISR body was
    # explicitly flagged then as "not traced further". See
    # docs/overview.md#speaker_sampleisr-named--the-digitized-sample-isr-body-traced. --

    (0x18842, "Speaker_sampleIsr",
     "sub_18842(): a real hardware ISR -- initializes several "
     "playback-state globals (byte_C84F6/word_C84F7/word_C84FC/"
     "word_C84FE/word_C8500/byte_C84FB, not renamed: buffer length/"
     "position/end-marker-shaped values, matching the already-"
     "documented 'digitized PC-speaker sound-effect engine'), then "
     "dispatches to one of two continuation routines (sub_18905 or "
     "sub_18883, not renamed -- plausibly double-buffered 'next "
     "sample byte' handlers), sends the PIC end-of-interrupt (out "
     "20h,20h), restores all registers, and returns via iret. This is "
     "the actual timer-ISR body flagged as 'not traced further' when "
     "the digitized PC-speaker engine was first documented -- not "
     "fully unpicked here either (the two continuation routines and "
     "exact sample-decoding scheme remain open), but confirms the ISR "
     "is a real, complete interrupt handler rather than a plain "
     "subroutine."),

    # -- fifty-ninth pass: sub_1D05A/sub_1D1A4, confirmed via the
    # already-named _opl2RhythmEnabled/Opl2_writeRegister/
    # Opl2_writeRhythmRegister as the OPL2 FM-synth note-on/note-off
    # primitives -- part of a MIDI-to-OPL2 translation layer (callers
    # sub_1E21B/sub_1E45C/sub_1E4C4, not renamed). See
    # docs/overview.md#opl2_noteon--opl2_noteoff-named. --

    (0x1D05A, "Opl2_noteOn",
     "sub_1D05A(channel, velocity): clamps velocity to 0x7F (the "
     "MIDI velocity range) and stores it per-channel; looks up that "
     "channel's operator-register offsets from one of two tables "
     "depending on _opl2RhythmEnabled (melodic vs. rhythm/percussion "
     "channel-to-operator mapping, since OPL2's 2-operator FM voices "
     "are wired differently for the 5 fixed rhythm instruments), then "
     "calls sub_1D492 (not renamed) for each of up to 2 operators "
     "(carrier + modulator) to apply the volume. The 'start playing a "
     "note' half of an OPL2 FM-synth voice primitive pair."),
    (0x1D1A4, "Opl2_noteOff",
     "sub_1D1A4(channel): for melodic channels, clears the key-on bit "
     "(0x20) in OPL2 register 0xB0+channel via the already-named "
     "Opl2_writeRegister -- the standard OPL2 note-off. For rhythm-"
     "mode channels (when _opl2RhythmEnabled is set and channel <= "
     "0xA), instead clears the corresponding bit in "
     "_opl2RhythmInstruments and calls the already-named "
     "Opl2_writeRhythmRegister. The 'stop playing a note' counterpart "
     "to Opl2_noteOn."),

    # -- sixtieth pass: sub_1D85B, confirmed via the already-named
    # Midi_sendCommand_raw/_midiDataPort as an MPU-401 reset-and-flush
    # helper, called from the already-named Midi_initDevice. See
    # docs/overview.md#midi_resetdevice-named. --

    (0x1D85B, "Midi_resetDevice",
     "sub_1D85B(): sends MPU-401's standard reset command (0xFF) via "
     "the already-named Midi_sendCommand_raw, then reads and discards "
     "one byte directly from _midiDataPort -- flushing any stray "
     "leftover byte from the data buffer right after a reset. Called "
     "from the already-named Midi_initDevice during setup, plus three "
     "other MIDI-adjacent call sites (sub_1F552/sub_1F93E/sub_1FA8E, "
     "not renamed)."),

    # -- sixty-first pass: sub_1E148/sub_1E168, a second compiled copy
    # of the Midi_peekTrackByte/Midi_readVarLengthValue pair, operating
    # on a different base pointer (_tmpSub._sub._set1) and position
    # counter (word_D20A0) -- same shape as the earlier Vocab_
    # matchesAbbreviation/String_matchesPrefixCI duplicate-copy finding.
    # See docs/overview.md#midi_peekbyte--midi_readvarlengthvalue2-named. --

    (0x1E148, "Midi_peekByte",
     "sub_1E148(byteOffset): reads a byte at "
     "*(_tmpSub._sub._set1) + byteOffset + word_D20A0 -- the same "
     "shape as the already-named Midi_peekTrackByte (base pointer + "
     "offset + running position counter), just for a single implicit "
     "stream rather than an array of tracks indexed by trackIndex."),
    (0x1E168, "Midi_readVarLengthValue2",
     "sub_1E168(): byte-for-byte the same Standard MIDI File "
     "variable-length-quantity decode loop as the already-named "
     "Midi_readVarLengthValue -- reads a byte via the new "
     "Midi_peekByte(0), accumulates 7 bits per byte, increments "
     "word_D20A0 (this copy's position counter) once per byte, and "
     "continues while the high bit is set. A second compiled copy for "
     "this stream, the same duplication pattern already seen with "
     "Vocab_matchesAbbreviation/String_matchesPrefixCI."),

    # -- sixty-second pass: sub_1FE30, called from the already-named
    # Sound_selectDevice, finish, and shutdown -- confirmed as the
    # sound subsystem's own full teardown function, the counterpart to
    # Sound_selectDevice's device-selection init. See
    # docs/overview.md#sound_shutdown-named. --

    (0x1FE30, "Sound_shutdown",
     "sub_1FE30(): unconditionally stops the current track (the "
     "already-named Sound_stopTrack(0xFFFF)); if word_C8582's MIDI-"
     "active bit (value 4, the same bit Sound_selectDevice sets on a "
     "successful MPU-401 probe) is set, calls sub_1FCAA and prints a "
     "short literal string via sub_1FB56 (both not renamed -- "
     "plausibly clearing an on-screen device indicator); finally "
     "masks word_C8582 down to just bit 8, clearing all backend-"
     "selection state. Called from the already-named `finish` and "
     "`shutdown` top-level exit routines -- the sound subsystem's own "
     "full teardown, counterpart to Sound_selectDevice's init."),

    # -- sixty-third pass: sub_22954, confirmed via the already-named
    # video_status_reg as the classic "wait for vertical retrace"
    # synchronization primitive, used before palette changes and mouse
    # cursor show/hide to avoid tearing/flicker. See
    # docs/overview.md#screen_waitforverticalretrace-named. --

    (0x22954, "Screen_waitForVerticalRetrace",
     "sub_22954(): reads the already-named video_status_reg and "
     "busy-waits for bit 3 (the EGA/VGA vertical-retrace bit) to "
     "clear, then busy-waits again for it to become set -- the "
     "standard technique to synchronize to the very start of a new "
     "vertical retrace period. Called from Screen_setEGAPalette "
     "(avoiding palette-change tearing/snow) and Mouse_Hide/"
     "Mouse_show (avoiding cursor-draw flicker)."),

    # -- sixty-fourth pass: sub_2384F, confirmed directly by its body
    # (using the already-named get_keypress) as an uppercase-
    # normalizing keypress reader -- likely the single-key menu-choice
    # reader hinted at for sub_C48E4 (Game_endGameMenu's own prompt
    # function, still not renamed). See
    # docs/overview.md#getuppercasekeypress-named. --

    (0x2384F, "getUppercaseKeypress",
     "sub_2384F(): calls the already-named get_keypress(), and if the "
     "result is a lowercase letter ('a'-'z'), converts it to "
     "uppercase (subtracting 0x20). Returns the result unchanged "
     "otherwise. A single-key menu-choice reader, named to match the "
     "existing lowercase-underscore get_keypress/get_input_line_ptr "
     "convention."),

    # -- sixty-fifth pass: sub_26228, confirmed directly by its body
    # (a gradual palette-brightness ramp via repeated
    # Screen_setEGAPalette calls) as the fade-IN counterpart to the
    # already-named Screen_fadeOut. See
    # docs/overview.md#screen_fadein-named. --

    (0x26228, "Screen_fadeIn",
     "sub_26228(): in text mode, does nothing. In basic EGA mode "
     "(_videoIndex==1), just sets a fixed reference palette directly "
     "(no gradual ramp). Otherwise (VGA/other), builds a scaled-down "
     "copy of the reference palette in a local buffer -- each color "
     "component multiplied by a growing fraction (0 up to 64/64, "
     "stepping by 4) -- calling the already-named Screen_setEGAPalette "
     "after each step, ramping the screen up from black to full "
     "brightness. The fade-IN counterpart to the already-named "
     "Screen_fadeOut."),

    # -- sixty-sixth pass: sub_26F74, finalizing the name for the
    # AnimPics per-slot animation-timing/draw loop already fully
    # characterized (but left unrenamed) back when AnimPics_freeAll was
    # named several passes ago -- the last unnamed piece of that
    # cluster. See docs/overview.md#animpics_tick-named--the-animpics-clusters-last-piece. --

    (0x26F74, "AnimPics_tick",
     "sub_26F74(mode): if _animPicsSlotCount is 0, returns "
     "immediately. Otherwise walks every active animated-picture slot, "
     "comparing the current _clock() against a per-slot randomized "
     "deadline (computed from _rand() scaled by the slot's registered "
     "duration parameters, from AnimPics_registerSlot); when a slot's "
     "deadline passes, advances its frame index (forward or backward "
     "per the slot's loop-direction byte) and draws the new frame via "
     "the already-named Image_draw, then computes the next randomized "
     "deadline. `mode` affects what gets recorded as a slot's "
     "completion state (0 vs -1) when it finishes a non-looping "
     "animation. Called from input-polling loops (Events_waitForPress, "
     "get_input_character, get_mouse_input) so animations keep "
     "advancing while the game waits for the player. Already "
     "characterized in detail in the AnimPics_freeAll writeup several "
     "passes ago; finalizing the name now as the last unnamed piece of "
     "that cluster."),

    # -- sixty-seventh pass: sub_28231, a trivial two-global setter.
    # Confirmed mechanically by direct read; the already-named globals
    # it writes (winNumber, word_D2A96) don't have enough other context
    # to pin down a more specific role beyond "window tracking", so
    # named conservatively. See
    # docs/overview.md#windows_setcontentwindow-named. --

    (0x28231, "Windows_setContentWindow",
     "sub_28231(winNumber, contentWinNum): sets the global "
     "`winNumber` (already named, but otherwise only ever written "
     "here) and `word_D2A96` (not renamed, but already known from the "
     "Icon_drawButton-adjacent code to be read as a TextWindow_"
     "addDirect target window number) from its two arguments. Called "
     "from the already-named room_load plus three other room/UI-"
     "adjacent functions -- consistent with selecting which window "
     "receives a room's text output, but not confirmed beyond the "
     "mechanical read/write shape."),

    # -- sixty-eighth pass: sub_2881D, confirmed directly by its body
    # (using the already-named LogFile_windowNum/LogFile_handle/
    # LogFile_disabled globals) as the transcript/log-file close
    # function, called from the already-named finish/shutdown exit
    # routines. See docs/overview.md#logfile_close-named. --

    (0x2881D, "LogFile_close",
     "sub_2881D(): if LogFile_windowNum is >= 0 (a transcript log "
     "file is currently open), closes it (_fclose(LogFile_handle)), "
     "resets LogFile_windowNum to -1, and clears LogFile_disabled. "
     "Called from the already-named finish/shutdown exit routines "
     "plus sub_1057E/sub_2E8F1 (not renamed) -- the transcript/log-"
     "file close counterpart to whatever opens it."),

    # -- sixty-ninth pass: sub_2A597, confirmed via the already-named
    # videoIndex/video_set_videoIndex as a validated video-mode-index
    # getter, called from several already-named drawing entry points.
    # See docs/overview.md#video_getvalidindex-named. --

    (0x2A597, "Video_getValidIndex",
     "sub_2A597(): reads the already-named videoIndex global (set by "
     "the already-named video_set_videoIndex; distinct from the "
     "similarly-named but separate _videoIndex global used elsewhere) "
     "and returns it if in range 0-7, or the sentinel -6 otherwise. "
     "Called from Surface_draw/Surface_draw2/Video_ClearScreen/"
     "sub_17B8E (not renamed) -- a validated video-mode-index getter "
     "used at the start of drawing operations."),

    # -- seventieth pass: sub_1063F, confirmed via the repeated
    # divide-by-1000 calls (through the MS Quick C runtime's 32-bit
    # long-division helper) as a signed 32-bit integer to decimal
    # string formatter. See
    # docs/overview.md#format_long_decimal-named. --

    (0x1063F, "format_long_decimal",
     "sub_1063F(loWord, hiWord): formats the signed 32-bit value "
     "(loWord:hiWord) as a decimal string into a local buffer -- "
     "special-cases exactly zero ('0'), negates and prefixes '-' for "
     "negative values, then repeatedly divides by progressively "
     "smaller powers of ten (via the MS Quick C/MSC runtime's 32-bit "
     "long-division helper, `unknown_libname_5`) to extract each "
     "decimal digit. A generic number-formatting utility (plausibly "
     "used for score/turn-count/credits display), named to match the "
     "project's lowercase-underscore convention for custom C-runtime-"
     "adjacent utilities."),

    # -- seventy-first pass: sub_2A163, confirmed via a real call to
    # the C runtime's _vsprintf as the core "format a message and show
    # it in an auto-sized dialog box" implementation that Dialog_prompt
    # wraps. See docs/overview.md#dialog_showformattedprompt-named. --

    (0x2A163, "Dialog_showFormattedPrompt",
     "sub_2A163(x1, y1, format, args, ...): formats its printf-style "
     "arguments into a local message buffer via the real C runtime "
     "_vsprintf, then walks the resulting text measuring line count "
     "(capped at 0x18=24) and max line width (capped at 0x4F=79, i.e. "
     "an 80x25 text screen) to auto-size a dialog box for the "
     "message. Called directly from the already-named Dialog_prompt -- "
     "this is the core formatted-message-dialog implementation "
     "Dialog_prompt wraps. The remainder of the function (actually "
     "creating/positioning the window and displaying the text) wasn't "
     "traced in full this pass."),

    # -- seventy-second pass: sub_2A41D, confirmed via direct read as
    # the nested-dialog "pop/restore previous" counterpart -- called
    # conditionally at the top of the just-named
    # Dialog_showFormattedPrompt, and from Dialog_prompt/printf. See
    # docs/overview.md#dialog_restoreprevious-named. --

    (0x2A41D, "Dialog_restorePrevious",
     "sub_2A41D(): no-ops if word_C97AA (a nested-dialog depth "
     "counter) is 0. Otherwise decrements it, destroys the "
     "just-closed dialog's window (the already-named Window_destroy), "
     "restores the saved Image_OffsetPos, redraws the previous "
     "dialog's background image from a per-depth array (Image_draw + "
     "Image_Free), redraws its saved text (TextWindow_addDirect), "
     "optionally shows the text cursor, and shows the mouse again. "
     "The pop/restore half of a nested-dialog stack (the push half "
     "isn't identified yet), mirroring the same nested-state-stack "
     "pattern already seen in Listbox_pushState."),

    # -- seventy-third pass: sub_13CC7, confirmed via its literal
    # strings (already recognized by IDA: "[Please be more specific.",
    # " I'm not sure wh[at]", " you mean by ") as the parser's
    # ambiguous-preposition clarification-request handler. See
    # docs/overview.md#parser_askforclarification-named. --

    (0x13CC7, "Parser_askForClarification",
     "sub_13CC7(data): if a flag byte in the parse-result struct "
     "(data+4) is set, does nothing. Otherwise, if the struct's vocab "
     "ID field (data+0x26) matches one of 8 specific values (0x491/"
     "0x492/0x42E/0x42F/0x420/0x424/0x928/0x929 -- plausibly a set of "
     "ambiguous prepositions), prints '[Please be more specific. I'm "
     "not sure what you mean by <word> you mean by...' and continues "
     "building a clarification prompt from the referenced word. The "
     "parser's 'ambiguous preposition/reference' disambiguation-"
     "request handler, called from the already-named "
     "GatewayParser_speakHandler/Parser_proc6."),

    # -- seventy-fourth pass: sub_1452B, confirmed via a real decoded
    # GATESTR.DAT message ("[%sn't holding%s.]") as the "is this object
    # currently holding the referenced object?" implicit-precondition
    # check, called directly from main(). See
    # docs/overview.md#logics_checkisholding-named. --

    (0x1452B, "Logics_checkIsHolding",
     "sub_1452B(logicNum): if logicNum is one of two special-cased "
     "values (0xE3/0xE4), returns 0 (check passes) unconditionally. "
     "Otherwise prints logicNum's name, then decoded GATESTR.DAT "
     "message 0xC407 -- '[%sn't holding%s.]' -- filling in "
     "logicNum's name and the referenced object's name "
     "(Logics_logicNum211), and returns 1. This reads as an implicit "
     "'is <subject> holding <object>?' precondition check used "
     "before some action, printing a failure message and signaling "
     "failure (1) when it doesn't hold; the two special-cased values "
     "presumably bypass the check for specific NPCs/contexts where it "
     "doesn't apply."),

    # -- seventy-fifth pass: sub_15674, the long-referenced "major hub
    # function" mentioned in passing throughout many earlier passes
    # (AnimPics_freeAll, Sound_stopTrack, Screen_fadeIn, Game_
    # restartAfterDeath) -- finally traced and named directly. See
    # docs/overview.md#game_showillustration-named--the-cutsceneillustration-display-sequence. --

    (0x15674, "Game_showIllustration",
     "sub_15674(picNumber, arg2, arg4, arg6, arg8): if a 'picture currently "
     "shown' flag (word_C8EF0) is set, tears down any active animated-"
     "picture overlays (sub_26F2A, the already-named AnimPics_freeAll). "
     "Checks whether graphics display is available (thunk_sub_5D9F3, "
     "forced true if _videoIndex==3); if not, falls back to a text-"
     "only path (sub_158C3, not renamed). Otherwise, if picNumber != 0, "
     "loads and draws the picture (stopping the current sound track "
     "first if a specific mode bit is set, to avoid audio glitching "
     "during picture display), or fills the screen black if the "
     "picture fails to load; either way calls the already-named "
     "Screen_fadeIn, delays 3 seconds, then calls sub_157A9 (not "
     "renamed -- plausibly the caption-text display) with its message "
     "arguments, and frees the loaded image. This is the game's "
     "full-screen illustration/cutscene display sequence -- confirmed "
     "as the hub Game_restartAfterDeath and others route through for "
     "'show a picture with fade-in, delay, and caption text'."),

    # -- seventy-sixth pass: sub_158C3, Game_showIllustration's own
    # text-only fallback path, traced directly. See
    # docs/overview.md#textwindow_addmessagelist-named. --

    (0x158C3, "TextWindow_addMessageList",
     "sub_158C3(msgArray): walks a far-pointer array of dword message-"
     "string pointers (msgArray), grouped by null (0:0) separator "
     "entries. For each group: prints a tab character, then each "
     "non-null message in the group via TextWindow_add back-to-back, "
     "then a newline; stops entirely at the first group that starts "
     "with a null entry. The already-named Game_showIllustration calls "
     "this as its text-only fallback (when graphics display isn't "
     "available) to print the same caption content a picture-mode call "
     "would otherwise show alongside the image."),

    (0x26F2A, "AnimPics_finishPlayback",
     "sub_26F2A (19 callers, sitting physically right between the named "
     "AnimPics_resyncSlots and AnimPics_tick in sg1692): if any AnimPics "
     "slots are registered, clears a private scratch buffer (unk_D2302) "
     "and latches each per-slot 'shown' byte in byte_D22EE to 1 (these "
     "arrays are private to this function only, not the slot handle/"
     "frame tables AnimPics_registerSlot writes). Always finishes by "
     "tail-calling Events_checkKeypress, which consumes a pending Space "
     "or Enter keypress (returning it) while requeuing any other pending "
     "key into injectCharacter for later. At 6+ call sites (e.g. "
     "sub_9B5F9, sub_B1730, sub_BA9A5, the death-sequence handler) it is "
     "called immediately before AnimPics_freeAll, and an anonymous inline "
     "loop in sg1692 (between AnimPics_resyncSlots and AnimPics_tick's "
     "definitions) calls it once after its tick/wait-for-press loop "
     "exits -- both patterns match 'settle the currently-displayed "
     "frames and swallow a skip keypress' as the standard finishing step "
     "before an anim-pics playback sequence tears down its slots."),

    (0x13629, "GameDate_format",
     "sub_13629(dayCount): adds 16 to dayCount, then walks a 12-entry "
     "table at seg067+0x1FE holding the standard (non-leap) Gregorian "
     "month lengths (31,28,31,30,31,30,31,31,30,31,30,31), starting "
     "from table index 4 (May, 0-based), subtracting whole months off "
     "dayCount and cycling the index mod 12 until what's left fits "
     "within the current month. If the table index wrapped back below "
     "4 (i.e. past December into Jan-Apr), the year-suffix is 3, "
     "otherwise 2; it then calls _sprintf with the format string at "
     "seg067+0x216 ('%02d-%02d-21%02d') to render "
     "'<month>-<day>-21<yearSuffix>' into the static buffer unk_D2E3A, "
     "and returns a far pointer to that buffer. Confirms the game's "
     "internal day-counter epoch (dayCount=0) is May 17, 2102 -- the "
     "in-universe date a dayCount of 0 formats to. Called from four "
     "room/logic-adjacent functions (sub_72260, sub_A60CE, sub_AA9E3, "
     "sub_B8235), consistent with a journal/diary-entry or event-log "
     "timestamp formatter."),

    (0x21B15, "RawFile_write",
     "sub_21B15(handle, buffer, bufferSeg, count): a thin wrapper "
     "around DOS INT 21h/AH=40h (write to file with handle) -- bx="
     "handle, cx=count, ds:dx=buffer, returning 0 on carry-set (error) "
     "or DOS's returned byte count otherwise. NOT the C runtime's own "
     "_write (already named at 0x1A89C, a much larger function with "
     "real FILE-handle-table bookkeeping and append-mode handling) -- "
     "this is a separate, much more primitive raw-handle writer, sitting "
     "in the small sg12EE segment alongside the already-named `fseek`, "
     "`fsetpos`, and `set_filename_prefix` -- a distinct, lightweight "
     "custom file-I/O layer the game uses directly (bypassing the C "
     "runtime's buffered stdio), most plausibly for the save-game "
     "system given fsetpos's presence in the same small group."),

    (0x1B81C, "_tzset",
     "sub_1B81C(): confirmed as the standard MSC runtime _tzset(). "
     "Calls _getenv('TZ') (the literal string 'TZ' verified directly "
     "at its argument address); if unset or empty, leaves defaults "
     "alone. Otherwise parses the classic POSIX TZ format (e.g. "
     "'PST8PDT'): _strncpy's the first 3 chars into off_CB770 (the "
     "standard-timezone name), atoi's the numeric UTC-offset digits "
     "and multiplies by 3600 (__aFlmul) into word_CB76A:word_CB76C (a "
     "32-bit seconds-west-of-UTC value), then _strncpy's any trailing "
     "DST-abbreviation chars into off_CB774, setting word_CB76E to 1 "
     "if that DST name is non-empty or 0 otherwise. The default data "
     "(before any TZ env var is parsed) is 'PST'/'PDT'/28800 seconds/"
     "daylight=1 -- the standard MSC runtime default."),

    (0x1B80C, "_tzsetOnce",
     "sub_1B80C(): a one-time-init guard around the just-named _tzset "
     "-- checks word_D2E00, calls _tzset and increments it only the "
     "first time through. Called from the already-named _ftime and "
     "__dtoxtime before they read the timezone globals, so those "
     "runtime functions don't re-parse the TZ environment variable on "
     "every call."),

    (0x0CB76A, "_timezoneLo",
     "Low word of the 32-bit _timezone MSC runtime global (seconds "
     "west of UTC) that _tzset (sub_1B81C) computes from the TZ "
     "environment variable, and _ftime divides by 60 to fill in a "
     "struct timeb's timezone-in-minutes field. Defaults to 0x7080 "
     "(28800 = 8 hours), matching the default 'PST8PDT' TZ data. "
     "Split Lo/Hi to match this project's existing convention for "
     "32-bit globals IDA represents as two word_ symbols (see "
     "_playerCreditsLo/_playerCreditsHi)."),

    (0x0CB76C, "_timezoneHi",
     "High word of the 32-bit _timezone global; see _timezoneLo "
     "(sub_1B81C/_tzset's companion global, defaults to 0)."),

    (0x0CB76E, "_daylight",
     "The MSC runtime _daylight global: 1 if the parsed TZ "
     "environment variable includes a DST-abbreviation suffix, 0 "
     "otherwise. Set by the just-named _tzset; defaults to 1 "
     "(matching the default 'PST8PDT' TZ data, which does specify a "
     "DST abbreviation)."),

    (0x0CB770, "_tzname",
     "The MSC runtime _tzname[0] global: the 3-letter standard-"
     "timezone abbreviation (e.g. 'PST'), written by _tzset "
     "(sub_1B81C) from the TZ environment variable's leading letters. "
     "Defaults to the literal string 'PST' (aPst)."),

    (0x0CB774, "_tznameDst",
     "The MSC runtime _tzname[1] global: the 3-letter DST-timezone "
     "abbreviation (e.g. 'PDT'), written by _tzset (sub_1B81C) from "
     "the TZ environment variable's trailing letters, or cleared to an "
     "empty string if the TZ value has no DST suffix. Defaults to the "
     "literal string 'PDT' (aPdt)."),

    (0x1B8F0, "_isindst",
     "sub_1B8F0(tm): another sibling of the just-named _tzset cluster "
     "(called from the already-named _ftime and __dtoxtime). Reads a "
     "0-based month field at tm+8: month<3 (before April) or month>9 "
     "(after October) returns 0 immediately (never DST); month strictly "
     "between 3 and 9 (May-September inclusive) returns 1 immediately "
     "(always DST) -- exactly the pre-2007 US DST rule (first Sunday "
     "of April through last Sunday of October). For the two boundary "
     "months (3=April, 9=October) it computes the weekday of a "
     "reference date via a classic day-of-week formula (year field at "
     "tm+0Ah, *365 plus leap-day correction, /7) to find that month's "
     "first (April) or last (October) Sunday, then compares tm's day-"
     "of-month (tm+0Eh) and what's plausibly an hour field (tm+4, "
     "checked against the 2am DST-transition hour) against it to "
     "decide which side of the transition the given date/time falls "
     "on. This is the standard MSC runtime `_isindst()`."),

    (0x18F54, "Dos_setErrnoFromCode",
     "sub_18F54(al=code, ah=flag): the real worker behind the "
     "already-named __maperror (which just zeroes ah and tail-calls "
     "this). If ah is already non-zero on entry, uses it directly as "
     "the result (callers _close/_dos_findfirst call this directly, "
     "presumably passing a pre-known errno value straight through via "
     "ah). Otherwise clamps al to a max index (byte_CAE19, with a "
     "special case for al in 0x20-0x21 forcing index 5), looks it up "
     "via xlat against a translation table at segment offset 0x2F3A "
     "(DOS extended-error-code -> errno mapping), and stores the "
     "sign-extended result into the just-identified errno global "
     "(word_CAE11, confirmed via the already-named fread's read of "
     "it, plus two direct 0x16/EINVAL literal stores elsewhere)."),

    (0x0CAE11, "errno",
     "The MSC runtime errno global. Read by the already-named fread; "
     "written directly to 0x16 (EINVAL) at two call sites, and by the "
     "just-named Dos_setErrnoFromCode (sub_18F54, the worker behind "
     "__maperror) via a DOS-error-code translation table."),

    (0x25B90, "Picture_checkFormatMatch",
     "sub_25B90(): reads the global pic_header._flags byte, masks it "
     "to the low nibble (a format-code sub-field distinct from the "
     "individual flag bits tested elsewhere in this struct, e.g. the "
     "already-named PICFLAG_HAS_PALETTE and the separately-tested 0x10/"
     "0x40 bits), and uses it to index byte_C96B0 (a per-format table). "
     "Compares that against byte_C96B6 indexed by the current "
     "_videoIndex (a per-video-mode table); if they match, returns the "
     "format-table value (the picture's required video mode), "
     "otherwise returns 0. Called from Picture_Load, load_and_scale_pic, "
     "and scale_pic -- the latter prints ' scale_pic : EGA -> VGA "
     "disabled ' (a literal string sitting right after these two "
     "tables in memory) when a picture's format doesn't match the "
     "active video mode, consistent with this function being the "
     "format/video-mode compatibility check that decision is based on."),

    (0x2BCA5, "Image_allocateSurface",
     "sub_2BCA5(height, width, videoIndex, surface, arg_A): calls "
     "sub_2A9C7(height, width, videoIndex) to compute a required "
     "buffer size; if that reports an error (dx!=0), returns the "
     "error code 0xFFE6 (-26). Otherwise allocates a handle of that "
     "size via the already-named new_handle; if allocation fails "
     "(dx==0, a null handle), also returns 0xFFE6. Otherwise calls "
     "sub_2BBBA(height, width, videoIndex, handle, surface, arg_A) to "
     "build/decode the actual image data into the new buffer, and "
     "returns 0 (success). Called from sub_24A42, itself called from "
     "the already-named Image_load -- the size-computation-plus-"
     "allocation-plus-build step of loading an image into a surface."),

    (0x5CD81, "InputWindow_setDisplayMode",
     "sub_5CD81(mode): remaps mode 2 to 5, then compares against a "
     "cached current mode (word_CBCFE); no-ops if unchanged. Otherwise "
     "resets word_CBD94 to 0, caches the new mode, and checks a second "
     "mode-like global (word_CBCFC) against 2: if it's 2, calls the "
     "already-named Scene_draw(0) then InputWindow_redrawPromptLine "
     "(a full scene + prompt-line redraw). Otherwise hides the mouse, "
     "clears the already-named Input_window_mb via WindowText_clear, "
     "calls the tentatively-named scene_update?, then shows the mouse "
     "again. Called (both directly and via thunk_sub_5CD81 from other "
     "overlays) with small literal mode values (1, 3, 4, 5 observed at "
     "different call sites) from InputWindow_getLine, get_mouse_input, "
     "and several room-logic overlays -- a shared display/input-mode "
     "switch that only runs its transition side effects when the mode "
     "actually changes."),

    (0x62AB0, "Undo_resetSnapshotBuffer",
     "sub_62AB0(): if the global `handle` (a memory handle, dword) is "
     "non-null, frees it via the already-named kill_handle and zeroes "
     "it; always resets a size accumulator (word_CBFE8) and two flags "
     "(Parser_val6, Parser_val7) to 0. Called from the already-named "
     "save_game and from sub_62AE2 (traced alongside this one) before "
     "(re)allocating a fresh undo snapshot buffer -- see the renames "
     "below."),

    (0x62AE2, "Undo_allocateSnapshotBuffer",
     "sub_62AE2(): calls the just-named Undo_resetSnapshotBuffer, then "
     "computes a required buffer size by summing per-entry "
     "contributions across the object method table (indexed 0..the "
     "already-named METHODS_COUNT) and the save-field table (indexed "
     "0..the already-named SAVE_FIELDS_COUNT) into word_CBFE8, plus a "
     "constant 0x42. Compares that (rounded up) against "
     "get_buffer_size()'s available space; if it fits, allocates a new "
     "handle of that size via new_handle into the global `handle`, and "
     "if the allocation succeeded, sets Parser_val7=1. Called from the "
     "already-named save_game (in its mode-3/quicksave path) whenever "
     "no undo buffer is currently allocated."),

    (0x0CBFE8, "_undoSnapshotSize",
     "The required-size accumulator Undo_allocateSnapshotBuffer "
     "(sub_62AE2) builds up from the method and save-field tables "
     "before allocating the undo snapshot handle; reset to 0 by "
     "Undo_resetSnapshotBuffer (sub_62AB0)."),

    (0x0CBFE4, "_undoSnapshotHandle",
     "The dword memory handle Undo_allocateSnapshotBuffer (sub_62AE2) "
     "allocates via new_handle to hold the in-memory undo snapshot; "
     "freed and zeroed by Undo_resetSnapshotBuffer (sub_62AB0). Locked/"
     "written by the already-named save_game (mode-3/quicksave path) "
     "via synchronize_save, treating the handle's memory as a virtual "
     "file."),

    (0x0CBFEB, "Parser_undoSnapshotValid",
     "Set to 1 by the already-named save_game's mode-3/quicksave path "
     "only after synchronize_save successfully writes the current game "
     "state into the undo snapshot handle; cleared to 0 up front each "
     "time that path runs. The already-named Parser_performUndo "
     "requires this (alongside Parser_val7, renamed below) before "
     "actually loading the undo slot -- 'a valid snapshot was taken "
     "this turn', as opposed to just 'the buffer exists'."),

    (0x0CBFEA, "Parser_undoBufferAllocated",
     "Set to 1 by Undo_allocateSnapshotBuffer (sub_62AE2) once the "
     "undo snapshot handle is successfully allocated; cleared by "
     "Undo_resetSnapshotBuffer (sub_62AB0). The already-named "
     "Parser_performUndo also uses this alone to decide which of two "
     "messages to print ('undone' vs 'nothing to undo' -- messages at "
     "off_CB926/off_CB92A, not yet decoded)."),

    (0x1D492, "Opl2_setOperatorVolume",
     "sub_1D492(operatorIndex): the volume half of an OPL2 operator "
     "update, called twice per note from the already-named Opl2_noteOn "
     "(once per operator -- carrier and modulator). Reads a cached "
     "per-channel velocity value (stored earlier by Opl2_noteOn at "
     "offset 0x1CA for rhythm channels or 0x1B8 for melodic ones), "
     "then reads the operator's current output-level register byte "
     "(masked to the low 6 bits, the OPL2 level field) and inverts it "
     "(0x3F - level) so higher = louder. For operators flagged to "
     "track velocity (gated by a per-operator byte at +0x1A6 and a "
     "table byte at +0xC), scales that inverted level by the velocity "
     "through a lookup table (indexed by the cached velocity, at "
     "bx-0x62C6) and rescales back down (>>7). Re-inverts to "
     "attenuation scale, ORs in the operator's key-scale-level bits "
     "(from +0x0, shifted into bits 6-7), and writes the result to "
     "OPL2 register 0x40+operatorRegisterOffset (the standard OPL2 "
     "Level/KSL register) via the already-named Opl2_writeRegister."),

    (0x1D570, "Opl2_setNoteSelect",
     "sub_1D570(): writes OPL2 register 8 (the chip-wide CSM-select/"
     "Note-Select register) with 0x40 if the global byte_D1C54 is "
     "non-zero, or 0 otherwise -- the standard OPL2 NTS (bit 6) "
     "keyboard-split-mode bit, which changes how key-scale frequency "
     "splits are computed chip-wide. byte_D1C54 is set by this "
     "function's only caller, sub_1CF90, immediately before calling "
     "this to commit the setting to hardware."),

    (0x20390, "Sound_initPlaybackTiming",
     "sub_20390(arg_0, arg_2, arg_4): no-ops unless bit 0x10 of the "
     "sound-engine state word (word_C8582, a widely-shared bitmask "
     "used across the sound backend selection/dispatch code -- which "
     "exact backend bit 0x10 marks wasn't pinned down this pass) is "
     "set. When set, computes arg_2 * 100000 (0x186A0, via __aFlmul) "
     "into a 32-bit pair (word_C858E:word_C8590 -- plausibly a "
     "duration or sample-count scaled into a fixed-point/microsecond "
     "unit), stores arg_4 into word_C858C, copies an existing 32-bit "
     "value (word_C8596:word_C8598) into word_C8592:word_C8594 "
     "(plausibly resetting an elapsed/position counter from a total-"
     "duration value), and stores arg_0 into word_C858A. Called from "
     "sub_1E7D4 and sub_1F1DE (both unnamed, themselves called from "
     "the already-named Sound_stopTrack area) -- consistent with "
     "initializing per-track playback-timing state for one specific "
     "sound backend before starting or resuming a track."),

    (0x203D6, "Sound_getElapsedPlaybackTime",
     "sub_203D6(): computes (word_C8596:word_C8598 minus a snapshot "
     "pair, sg3EDC:_tmpSub._val7/_val8) via unsigned 32-bit subtract, "
     "then unsigned-divides that by 1000 (__aFuldiv) and returns the "
     "result in dx:ax. word_C8596:word_C8598 is a running clock/tick "
     "counter; _tmpSub._val7/_val8 is a snapshot of that same counter "
     "taken elsewhere (in sub_1EB9E, unnamed) at track-start time -- so "
     "this computes elapsed playback time since that snapshot, scaled "
     "down by 1000 (plausibly ms from an underlying microsecond-ish "
     "tick, matching the just-named Sound_initPlaybackTiming's "
     "*100000 scaling). Called from sub_1E7D4 and sub_1F1DE, the same "
     "two unnamed callers as Sound_initPlaybackTiming -- the elapsed-"
     "time query half of that same per-track timing mechanism."),

    (0x157A9, "Game_showCaptionText",
     "sub_157A9(msgArray, xp, yp, redrawPicture): the long-flagged "
     "'Game_showIllustration's caption-text helper' from several "
     "passes ago, finally traced directly. msgArray is a far-pointer "
     "array of dword message pointers grouped by null (0:0) separator "
     "entries, the same shape TextWindow_addMessageList walks for its "
     "text-only fallback. For each group: resets the y position to "
     "yp, then for every message in the group draws it TWICE via "
     "Font_setColor/Font_setPosition/Font_writeString -- once in black "
     "(fg=0) at (xp, y), then again in white (fg=0xFF) at (xp-1, y-1) "
     "-- a drop-shadow/embossed caption style, advancing y by 0xC (12) "
     "per line. After a group finishes, delays ~15 (ticks or seconds, "
     "via j_delay); if the delay is interrupted (skip keypress), "
     "returns immediately. Otherwise, if redrawPicture is set, redraws "
     "the already-loaded illustration (Image_draw) before continuing "
     "to the next caption group over the same picture; if not set, "
     "fills the screen black (Screen_setPenColor(BLACK) + fillRect) "
     "before the next group instead -- supporting both 'captions over "
     "a static picture' and 'sequential black-background caption "
     "pages' modes. Returns once the message array is exhausted (a "
     "null entry with no more groups) or a caption is skipped."),

    (0x15F35, "Sound_lookupTrackVariant",
     "sub_15F35(trackId): the long-flagged 'sound resource-variant "
     "lookup' from several passes ago, finally traced. Walks a 6-"
     "bytes-per-entry table (up to 0x25=37 entries) whose first word "
     "field is a key; if trackId matches an entry's key, returns "
     "either that entry's second word field (offset +2) when the "
     "MIDI-active bit (bit 4) of word_C8582 is set, or its third word "
     "field (offset +4) otherwise. Returns 0 if trackId isn't found. "
     "Called twice from the already-named Sound_selectTrackForRoom -- "
     "a per-room/track table mapping a logical track ID to the "
     "specific sound-resource number to use for whichever backend "
     "(MIDI vs. other) is currently active."),

    (0x1057E, "Game_refuseRestart",
     "sub_1057E(): calls the already-named LogFile_close, frees the "
     "currently-loaded illustration (Image_Free(img)), invokes "
     "Logic_call(_roomLogicNum, action=24) (a pre-something room-logic "
     "hook), reloads game state via j_load_game(1), calls "
     "j_scene_update?, invokes Logic_call(_roomLogicNum, action=25) "
     "(a matching post-hook), then prints the decoded literal message "
     "'[Sorry, you can't use \"restart\" right now.]' (aSorryYouCanTUs, "
     "referenced via aaCantRestart) and returns -1. Despite the "
     "load_game/Logic_call bracketing looking restart-shaped, the "
     "printed message is unambiguous: this is the handler for a "
     "restart request the engine declines (called from the already-"
     "named Game_endGameMenu and from sub_69EDA), most plausibly "
     "reloading/resuming the current session rather than actually "
     "restarting -- distinct from the already-named "
     "Game_restartAfterDeath, which performs a real restart."),

    (0x18842, "SoundBlaster_dmaIsr",
     "CORRECTION: this ISR (previously named as part of a "
     "'digitized PC-speaker sound-effect engine' several sessions "
     "ago) is actually the Sound Blaster DSP's auto-init-DMA sample-"
     "playback interrupt handler, not PC-speaker code. Confirmed this "
     "pass by tracing its two dispatch targets (sub_18883/sub_18905, "
     "renamed below): both directly program the ISA DMA controller "
     "(8237A-5, ports 0x0A-0x0C/0x02-0x03/0x83, channel 1) and the "
     "already-named _sbBasePort/Sb_writeByte Sound Blaster DSP "
     "interface -- PC-speaker playback never touches DMA or the SB "
     "base port at all. The installer code just above this ISR (at "
     "sg09a4, around loc_1875C) confirms it further: it saves the "
     "current IRQ vector for byte_C84F5 (default 3, a classic SB IRQ "
     "line) into word_C8502:word_C8504 before installing this ISR at "
     "cs:0x7C1, using the already-named Sb_writeByte to program the "
     "card first. The buffer-position/length globals this ISR sets up "
     "(byte_C84F6/word_C84F7/word_C84FC/word_C84FE/word_C8500/"
     "byte_C84FB) are real, just belonging to the Sound Blaster DMA "
     "backend instead of PC-speaker -- that part of the original "
     "characterization stands."),

    (0x18883, "SoundBlaster_startNextDmaBlock",
     "sub_18883(): reprograms ISA DMA channel 1 for the next block of "
     "digitized-sample playback -- masks the channel, clears the byte-"
     "pointer flip-flop, sets auto-init/write/increment/single mode "
     "(0x49), writes the base address (from word_C84F7 and the block-"
     "index byte_C84F6 as the DMA page register), writes the new "
     "count, then re-enables the channel. Advances the internal block "
     "counter/index (byte_C84FB/byte_C84F6), resets word_C84F7, then "
     "sends Sound Blaster DSP command 0x14 (8-bit single-cycle DMA "
     "DAC output) followed by the 16-bit sample count via sub_18950 "
     "(unnamed, a DSP command-byte sender). One of the just-corrected "
     "SoundBlaster_dmaIsr's two dispatch targets -- the 'more data "
     "queued, start the next DMA block' path."),

    (0x18905, "SoundBlaster_uninstallDmaIsr",
     "sub_18905(): masks DMA channel 1, then restores the original "
     "interrupt vector for IRQ byte_C84F5 (writing word_C8502:"
     "word_C8504 -- the vector saved by this ISR's installer -- back "
     "into the real-mode IVT), temporarily masking/unmasking that IRQ "
     "line on the 8259A PIC around the restore. Clears two state "
     "flags (byte_C84CA, cs:byte_1803B), then reads the Sound Blaster "
     "DSP's IRQ-acknowledge port (_sbBasePort+0xE) to clear the "
     "pending hardware interrupt. The just-corrected "
     "SoundBlaster_dmaIsr's other dispatch target -- the 'playback "
     "finished, uninstall this ISR and acknowledge the last IRQ' "
     "path."),

    (0x18950, "SoundBlaster_writeByteFromIsr",
     "sub_18950(al=byte, dx=port): byte-for-byte the same DSP-write "
     "handshake as the already-named Sb_writeByte (poll the status "
     "port until bit 7 clears, a few I/O-delay jumps, then write the "
     "byte) -- another instance of this project's duplicate-compiled-"
     "copy pattern. The one difference: this copy polls unboundedly "
     "(no cx-based retry limit, no carry-flag timeout signal), fitting "
     "its use inside SoundBlaster_startNextDmaBlock, itself called "
     "from interrupt context (SoundBlaster_dmaIsr) where a private, "
     "self-contained near-call duplicate avoids relying on the far-"
     "callable Sb_writeByte's calling convention/timeout plumbing."),

    (0x1E974, "Opl2_stopTrack",
     "sub_1E974(): one of the long-flagged 'other Sound_stopTrack "
     "backend routines' -- the OPL2/AdLib backend's stop-track "
     "handler, called directly from the already-named Sound_stopTrack "
     "(and from sub_1E950). Resets byte_D20A2, calls sub_1CC34(0) "
     "(unnamed, plausibly resetting some MIDI-file-position parse "
     "state shared with the MIDI backend), sets a _tmpSub scratch "
     "value, and conditionally calls sub_1E136 (unnamed) if a track-"
     "loaded flag (byte_C850C) is set. Then loops over all 11 OPL2 "
     "logical channels (0-10, matching 9 melodic voices or 6 melodic + "
     "5 rhythm-mode percussion voices), zeroing a per-channel state "
     "word and calling the already-named Opl2_noteOn(channel, "
     "velocity=0) then Opl2_noteOff(channel) to silence each one. "
     "Finally, if bit 0x10 of word_C8582 is set, clears bits 4-6 of "
     "that shared state word and resets byte_D20A2 again -- tearing "
     "down whatever per-track state that bit range represents for "
     "this backend."),

    (0x13CB1, "Parser_printBeMoreSpecific",
     "sub_13CB1(): prints the literal message '[Please be more "
     "specific.%s]\\n' (aPleaseBeMoreSp + asc_CB97E) via "
     "TextWindow_add. Called from the already-named "
     "GatewayParser_speakHandler and Parser_proc6 -- a simpler, "
     "generic sibling of the already-named Parser_askForClarification "
     "(which fills in a specific ambiguous word), used when the "
     "parser needs to ask for clarification without a particular word "
     "to reference."),

    (0x13F85, "Parser_printTalkingIsStrange",
     "sub_13F85(logicNum): if logicNum is 0xD3 (a special-cased ID, "
     "plausibly the player character), uses the literal string ' "
     "yourself' as the referenced name; otherwise calls j_printObj"
     "(logicNum, 3) to get the target's descriptive name/pronoun. "
     "Either way, prints decoded GATESTR.DAT message 0xC404 -- "
     "'Doesn't it strike you that talking to%s is just a little "
     "strange?' -- filling in the name. Then sets Persisted_val6 to "
     "'e' and queues logic/event 0x2B via Queue_add(0x2B, 1) "
     "(plausibly scheduling a delayed follow-up reaction). Called "
     "from the already-named GatewayParser_speakHandler -- the "
     "parser's response to trying to talk to a non-conversational "
     "target (an object, or oneself)."),

    (0x147A6, "Parser_callActionHandler",
     "sub_147A6(actionId): bounds-checks actionId to 1-195 (returns 0 "
     "if 0 or >195), then indexes a 6-bytes-per-entry function-pointer "
     "table (off_3C978) by actionId*6 and calls that far function "
     "pointer, returning its result (0 if out of range). Called from "
     "the already-named Parser_perform -- the core 'dispatch to the "
     "handler for this action/verb ID' primitive of the parser "
     "execution engine, analogous in shape to the object-method-table "
     "dispatch pattern seen elsewhere (sub_1234F/sub_123A1, not "
     "renamed there due to corrupted disassembly) but using its own "
     "separate table."),

    (0x169A6, "Windows_switchListboxWindow",
     "sub_169A6(direction): no-ops if Windows_currentWindow < 0. "
     "Otherwise redraws the current listbox deselected "
     "(Listbox_draw(0)), then steps Windows_currentWindow forward "
     "(direction>0, wrapping 5->0) or backward (direction<=0, "
     "wrapping 0->5) through the 6 window slots (0-5) until landing "
     "on one whose Windows_listboxIndex[] entry is >=0 (i.e. actually "
     "has a listbox), then redraws that window's listbox selected "
     "(Listbox_draw(1)). Called from the already-named get_mouse_input "
     "and from sub_1796D -- the 'switch focus to the next/previous "
     "listbox window' navigation primitive (e.g. Tab/Shift-Tab-style "
     "cycling between listbox windows)."),

    (0x16A89, "Listbox_getSelectedIndexForWindow",
     "sub_16A89(winNumber): calls the already-named "
     "Windows_getListboxIndex(winNumber); if that's negative (the "
     "window has no listbox), returns -1. Otherwise returns "
     "Listbox_selectedIndex[] at that listbox index -- the currently-"
     "selected item index for the listbox in the given window. Called "
     "twice from the already-named Listbox_mouseButtonDown."),

    (0x1CC15, "Pit_setReloadCount",
     "sub_1CC15(ax=count): programs the 8253/8254 PIT's channel 0 "
     "(the system timer, normally IRQ0) via out 0x43 with command "
     "byte 0x36 (channel 0, mode 3 square wave, 16-bit binary, LSB "
     "then MSB access), then writes ax's low byte then high byte to "
     "port 0x40 -- the standard sequence to reprogram the system "
     "timer to a custom tick rate. Called from sub_1CC34 (renamed "
     "below) and sub_1CC58 -- the low-level hardware step of setting "
     "a custom sound/timing tick rate."),

    (0x1CC34, "Sound_setTimerRate",
     "sub_1CC34(rate): stores rate into a code-segment-resident word "
     "(cs:word_1CC04, consistent with being read by a timer ISR "
     "living in the same segment), sets a 'stopped' flag (cs:"
     "word_1CC02) to 1 if rate<1 or 0 otherwise (via the carry-to-"
     "boolean idiom), then calls the just-named Pit_setReloadCount(rate) "
     "to actually reprogram the hardware timer. Runs with interrupts "
     "disabled around the whole sequence. Called from the already-"
     "named Opl2_stopTrack (with rate=0, resetting/stopping the "
     "timer) and from sub_1E329 (presumably setting a real tempo-"
     "derived rate) -- the shared master timer-rate control for the "
     "sound engine's custom tick clock."),

    (0x15932, "Logics_collectPlayerItemLists",
     "sub_15932(): walks two separate contained-items linked lists off "
     "logicNum 0xD3 (the player, per this session's earlier finding in "
     "Parser_printTalkingIsStrange) using the already-documented "
     "Logics_getUnkHandler(0xD3, handlerIndex)/Logics_getVal1 "
     "traversal pattern (same shape as Logics_describeContents' "
     "container-contents walk): first with handlerIndex=1, snapshotting "
     "each visited logicNum into a flat array (seg3EDC-based buffer 1), "
     "null-terminated; then again with handlerIndex=0 into a second "
     "flat array (buffer 2), also null-terminated. Called from "
     "sub_A2D8D and sub_3141B (both unnamed) -- plausibly separating "
     "the player's worn vs. carried items (or two similarly-split "
     "inventory categories) into two ready-to-iterate arrays, though "
     "which handler index maps to which category wasn't independently "
     "confirmed this pass."),

    (0x16B53, "Listbox_getSelectedItemText",
     "sub_16B53(): returns a far pointer to a static buffer "
     "(byte_D312C) holding the text of the current window's currently-"
     "selected listbox item, or an empty string if there's no listbox "
     "or no items. Two source formats are handled: if "
     "Listbox_items[listboxIdx] is a raw-text blob (sentinel word "
     "0xFFFF at its start), the selected line is directly _strcpy'd "
     "out and trailing spaces trimmed. Otherwise the listbox stores "
     "each line as a list of vocab-word indices, so each word's text "
     "is looked up via vocab_list._textP and _strcat'd together with "
     "spaces, up to Listbox_lineSize words or a terminating 0 index. "
     "Finally, if Listbox_argE has bit 1 set, the first character is "
     "capitalized (via ascii_table_flags's lowercase-letter bit). "
     "Called from prompt_for_filename and sub_5D9F3 (the RTLink-"
     "thunked function behind thunk_sub_5D9F3, seen many times this "
     "session) -- the general 'read the highlighted listbox entry as "
     "a string' primitive, e.g. for reading a selected filename out "
     "of a file-picker listbox."),

    (0x1796D, "Listbox_handleNavigationKey",
     "sub_1796D(c): the listbox keyboard-navigation dispatcher. Maps "
     "c (an extended key code, c-327 indexing an 11-entry jump table) "
     "to: Home (-9999) / End (9999) / PageUp / PageDown (both via the "
     "already-named Listbox_getNumLines, signed) / Up / Down (delta "
     "+-1, via the already-named Listbox_deltaChange) / Left / Right "
     "(switch listbox window, via the already-named "
     "Windows_switchListboxWindow). For any other character, calls "
     "sub_238C1 (unnamed) as a gate, and if it passes and c is a "
     "printable/alpha character (per ascii_table_flags), calls the "
     "already-named Listbox_findLineStartingWith(c) -- a type-ahead-"
     "jump-to-item feature. Returns 1 if the keypress was consumed by "
     "the listbox, 0 otherwise (letting the caller process it as a "
     "normal character). Called from get_mouse_input and "
     "prompt_for_filename."),

    (0x1D58C, "Opl2_setChannelFeedback",
     "sub_1D58C(channel): no-ops if the per-channel flag byte at "
     "+0x1A6 is set (the same flag Opl2_setOperatorVolume gates its "
     "velocity-tracking on) -- i.e. this only applies to normal "
     "(non-rhythm-only) channels. Otherwise indexes the same 7-byte-"
     "stride per-channel table Opl2_setOperatorVolume uses (channel*7) "
     "to read a feedback-amount byte (+2, doubled into bits 1-3) and "
     "an algorithm/connection-type byte (+0xC, compared >=1 to set bit "
     "0), OR's them together, and writes the result to OPL2 register "
     "0xC0+channelRegisterOffset (the standard OPL2 Feedback/"
     "Connection-Type register) via the already-named "
     "Opl2_writeRegister -- using the same per-channel register-offset "
     "field (+0x1B8) as Opl2_setOperatorVolume. Called from sub_1D3C4 "
     "and sub_1D448 (both unnamed, plausibly the note-on/instrument-"
     "setup routines for this backend)."),

    (0x1D5E8, "Opl2_setOperatorAttackDecay",
     "sub_1D5E8(operatorIndex): indexes the same 7-byte-stride table "
     "(operatorIndex*7) as the OPL2 cluster's other per-operator "
     "setters, reads a byte at +3 (shifted into bits 4-7) and a byte "
     "at +6 (masked into bits 0-3), ORs them together, and writes the "
     "result to OPL2 register 0x60+operatorRegisterOffset (using the "
     "same +0x194 per-operator register-offset field as "
     "Opl2_setOperatorVolume) via the already-named "
     "Opl2_writeRegister. Register 0x60-0x75 is the standard OPL2 "
     "Attack-Rate/Decay-Rate register (AR in the high nibble, DR in "
     "the low nibble). Called from sub_1D3C4 and sub_1D448."),

    (0x1D63E, "Opl2_setOperatorSustainRelease",
     "sub_1D63E(operatorIndex): byte-for-byte the same shape as the "
     "just-named Opl2_setOperatorAttackDecay, but reading fields +4 "
     "(high nibble) and +7 (low nibble) instead of +3/+6, and writing "
     "to OPL2 register 0x80+operatorRegisterOffset -- the standard "
     "OPL2 Sustain-Level/Release-Rate register (SL in the high "
     "nibble, RR in the low nibble). Called from sub_1D3C4 and "
     "sub_1D448, completing the four standard per-operator OPL2 "
     "envelope/level registers this cluster now covers (0x40 level/"
     "KSL, 0x60 attack/decay, 0x80 sustain/release, plus the per-"
     "channel 0xC0 feedback/connection)."),

    (0x1D694, "Opl2_setOperatorModulationFlags",
     "sub_1D694(operatorIndex): indexes the same 7-byte-stride table "
     "as the rest of this cluster and builds a byte from four boolean "
     "flag fields (+9 -> bit 7, +0xA -> bit 6, +5 -> bit 5, +0xB -> "
     "bit 4) plus a 4-bit value (+1, masked 0xF, bits 0-3), writing "
     "the result to OPL2 register 0x20+operatorRegisterOffset via the "
     "already-named Opl2_writeRegister. Register 0x20-0x35 is the "
     "standard OPL2 AM/Vibrato/Envelope-Type/KSR/Multiple register "
     "(Tremolo, Vibrato, sustain EG type, key-scale rate, and "
     "frequency multiplier). Called from sub_1D3C4 and sub_1D448, "
     "completing all 5 standard OPL2 per-operator/channel registers "
     "this cluster now covers (0x20 AM/VIB/EG/KSR/Mult, 0x40 Level/"
     "KSL, 0x60 Attack/Decay, 0x80 Sustain/Release, 0xC0 Feedback/"
     "Connection)."),

    (0x1D786, "Opl2_setOperatorWaveform",
     "sub_1D786(operatorIndex): if the global word_D1C56 (a 'waveform "
     "select enabled' flag, matching OPL2's WSE bit) is nonzero, reads "
     "a 2-bit waveform value from the same 7-byte-stride per-operator "
     "table (+0xD, masked to 2 bits); otherwise uses 0 (sine, the "
     "fixed default when waveform select is disabled). Writes the "
     "result to OPL2 register 0xE0+operatorRegisterOffset -- the "
     "standard OPL2 Waveform-Select register -- via the already-named "
     "Opl2_writeRegister. Called from sub_1D3C4 and sub_1D448, and "
     "with this the entire standard OPL2 per-operator/channel register "
     "set is now covered by this cluster (0x20/0x40/0x60/0x80/0xC0/"
     "0xE0)."),

    (0x1D3C4, "Opl2_setOperatorProperty",
     "sub_1D3C4(operatorIndex, propertyId): a property-ID dispatcher "
     "(propertyId 0-0x11, 18 values, via a jump table) that calls "
     "exactly one of this session's just-named OPL2 register setters "
     "per property -- Opl2_setOperatorVolume, Opl2_setChannelFeedback, "
     "Opl2_setOperatorAttackDecay, Opl2_setOperatorSustainRelease, "
     "Opl2_setOperatorModulationFlags, Opl2_setOperatorWaveform, "
     "Opl2_setNoteSelect, or the already-named "
     "Opl2_writeRhythmRegister -- letting a caller update a single "
     "named instrument/operator parameter without touching the "
     "others. Ties together this entire OPL2 register cluster as a "
     "single property-set entry point."),

    (0x1D448, "Opl2_applyOperatorSettings",
     "sub_1D448(operatorIndex): unconditionally calls every OPL2 "
     "register setter this cluster covers, in sequence, for one "
     "operator -- Opl2_writeRhythmRegister, Opl2_setNoteSelect, "
     "Opl2_setOperatorVolume, Opl2_setChannelFeedback, "
     "Opl2_setOperatorAttackDecay, Opl2_setOperatorSustainRelease, "
     "Opl2_setOperatorModulationFlags, and Opl2_setOperatorWaveform. "
     "The 'load/commit a full instrument definition' counterpart to "
     "the just-named Opl2_setOperatorProperty's 'update one property' "
     "role -- called from sub_1D2FC when first setting up an "
     "operator's complete OPL2 register state."),

    (0x1D2FC, "Opl2_loadOperatorPatch",
     "sub_1D2FC(operatorIndex, patchData, waveform): copies 13 bytes "
     "from a source patch structure (patchData, read every other byte "
     "-- a 26-byte source, matching a duplicated-field or word-sized-"
     "field-per-byte encoding) into the 7-byte-stride per-operator "
     "table this whole OPL2 cluster shares (at operatorIndex*7, "
     "offsets 0-12), then writes waveform (masked to 2 bits) into "
     "offset+13 of that same table -- exactly the field the just-"
     "named Opl2_setOperatorWaveform reads. Finally calls the just-"
     "named Opl2_applyOperatorSettings(operatorIndex) to push the "
     "whole loaded patch out to OPL2 hardware in one go. This is the "
     "'load a MIDI-instrument-patch definition into an OPL2 operator' "
     "entry point, called from sub_1CFB0."),

    (0x1CEC0, "Opl2_setRhythmMode",
     "sub_1CEC0(enable): the master OPL2 rhythm-mode toggle. When "
     "enabling, configures two extra rhythm-mode channels (7 and 8 -- "
     "OPL2's hi-hat/cymbal-style extra voices) via sub_1D7DA (a "
     "per-channel setter in this same cluster, not yet renamed) with "
     "specific hardcoded parameters. Always sets the already-named "
     "_opl2RhythmEnabled global from the argument, sets the channel "
     "count (now _opl2ChannelCount) to 11 (6 melodic + 5 rhythm-mode "
     "percussion voices) if enabling or 9 (melodic-only) otherwise -- "
     "matching the channel-loop bounds already seen in the already-"
     "named Opl2_stopTrack -- clears the already-named "
     "_opl2RhythmInstruments bitmask, then calls sub_1D1FE (unnamed) "
     "and the already-named Opl2_writeRhythmRegister to commit the "
     "mode switch to hardware."),

    (0x0D1C82, "_opl2ChannelCount",
     "The OPL2 backend's active logical-channel count: 11 (6 melodic "
     "+ 5 rhythm-mode percussion voices) when rhythm mode is enabled, "
     "or 9 (melodic-only) otherwise. Set by the just-named "
     "Opl2_setRhythmMode; read as a channel-loop upper bound "
     "elsewhere in the OPL2 backend."),

    (0x1CF6E, "Opl2_setMasterVolume",
     "sub_1CF6E(level): clamps level to the range 1-12 and stores it "
     "into word_D3BD2 (now _opl2MasterVolume). That global's only "
     "reader is sub_1CB32 (unnamed, called from sub_1D7DA -- part of "
     "this same OPL2 per-channel cluster, itself called from the "
     "already-named Opl2_setRhythmMode), consistent with a 1-12 "
     "master-volume-style scaling factor feeding into per-channel "
     "volume/instrument calculations. Called from sub_1CE20 and "
     "sub_1E60C."),

    (0x0D3BD2, "_opl2MasterVolume",
     "A clamped 1-12 master-volume-style value set by the just-named "
     "Opl2_setMasterVolume; read only by sub_1CB32 (unnamed), part of "
     "the OPL2 per-channel instrument-setup cluster."),

    (0x1861F, "Opl2_writeDetectRegister",
     "sub_1861F(ah=reg, al=value): byte-for-byte the same OPL2 "
     "register-write sequence as the already-named Opl2_writeRegister "
     "-- write the register number to port 0x388, delay via 4 IN "
     "reads (the calibrated ~3.3us address-write delay from the "
     "Adlib Programming Guide), write the value to port 0x389, then "
     "delay via 23 IN reads of port 0x388 (~23us data-write delay) -- "
     "but as a private, near-callable duplicate using raw port I/O "
     "directly instead of calling Opl2_writeRegister. Called "
     "repeatedly from sub_18432 (itself called from the already-named "
     "Stream_selectHandler), consistent with the classic AdLib/OPL2 "
     "hardware-presence detection sequence (reset/mask the timers, "
     "start timer 1, check status) rather than the main note-playing "
     "engine -- another instance of this project's duplicate-"
     "compiled-copy pattern."),

    (0x1DD8E, "Sound_takeTrackFlag",
     "sub_1DD8E(trackIndex): with interrupts disabled, atomically "
     "reads then zeroes a per-track word at a table indexed by "
     "trackIndex*2 (offset 0x557) -- a classic 'take and clear a "
     "pending-event flag shared with an ISR' pattern (cli/sti "
     "bracketing implies a timer ISR writes this same table). Called "
     "from sub_1F1DE and sub_1F93E, both already seen as callers "
     "within the Sound_initPlaybackTiming/Sound_getElapsedPlaybackTime "
     "timing cluster from earlier this session -- plausibly consuming "
     "a per-track 'segment/loop completed' notification set by the "
     "sound engine's timer ISR, though the exact event this flag "
     "represents wasn't independently confirmed."),

    (0x1D953, "Midi_setDataCallback",
     "sub_1D953(callback): with interrupts disabled, atomically sets "
     "the already-named far-pointer global _midiDataCallback from its "
     "two word arguments (offset:segment). Called (twice) from "
     "sub_1F552 -- a simple atomic setter for the MIDI data callback "
     "pointer, matching the cli/sti pattern used elsewhere in this "
     "session's sound-timing cluster for state shared with an ISR."),

    (0x1F910, "Midi_stopTrack",
     "sub_1F910(): another of the long-flagged 'other Sound_stopTrack "
     "backend routines' -- called directly from the already-named "
     "Sound_stopTrack (and from sub_1F692). Resets a scratch value, "
     "sets word_C8532 to 1, then busy-loops calling sub_1F93E "
     "(unnamed, tied to the just-named Midi_setDataCallback's caller "
     "sub_1F552's neighborhood) until word_C8532 is cleared back to 0 "
     "-- a blocking 'drain the pending MIDI queue until finished' "
     "loop. Afterward clears word_C852E if set, and returns 1. The "
     "MIDI/MPU-401 backend's stop-track handler, paralleling the "
     "already-named Opl2_stopTrack for the OPL2 backend."),

    (0x1F93E, "Midi_stopTrackStep",
     "sub_1F93E(): the per-call state-machine step Midi_stopTrack's "
     "busy-loop repeatedly invokes, advancing a shared scratch state "
     "counter (_tmpSub._val9, reused here purely as a 0-19 step index) "
     "by one on every call via a 20-entry jump table. Step 1: if a "
     "flush-needed flag (word_C8530) is set, calls the already-named "
     "Sound_takeTrackFlag per track and sends MIDI byte 0xFC for any "
     "flagged one, then calls Midi_resetDevice. Step 2: calls "
     "Midi_resetDevice, installs a fixed completion routine via the "
     "just-named Midi_setDataCallback, then spins on Midi_sendCommand"
     "(0x3F) until it succeeds. Steps 3-18 (16 steps, one per MIDI "
     "channel): send Control Change 123 ('All Notes Off') and 121 "
     "('Reset All Controllers') on that channel via Midi_sendByte. "
     "Step 19: clears word_C8532 -- the flag Midi_stopTrack's busy-"
     "loop is waiting on -- signaling the whole shutdown sequence is "
     "complete. This one function makes Midi_stopTrack's entire "
     "multi-call drain loop concrete: a proper MIDI 'all channels "
     "silent, device reset' teardown sequence, spread one step per "
     "call so it doesn't block for too long in any single call."),

    (0x1FB56, "Midi_sendDisplayText",
     "sub_1FB56(str): CORRECTS an earlier guess (in Sound_shutdown's "
     "writeup) that this 'clears an on-screen device indicator' -- it "
     "doesn't touch the screen at all. Sends a Roland-style MIDI "
     "SysEx 'Display Data' message: three header bytes (0x20, 0, 0 -- "
     "matching Roland's MT-32/Sound-Canvas Display-Data SysEx address "
     "0x20 0x00 0x00) via Midi_sendByte, then 20 (0x14) bytes read "
     "from str, accumulating a running byte sum, then a standard "
     "Roland 7-bit two's-complement checksum byte "
     "(-(sum & 0x7F) mod 128), via sub_1FA8E/sub_1FAFE/sub_1FC4E "
     "(unnamed, plausibly the SysEx start-of-message/end-of-message "
     "framing). Called from the already-named Sound_selectDevice (on "
     "successful MPU-401 detection, to show a device/game identifier) "
     "and Sound_shutdown (presumably to clear/blank the display on "
     "exit) -- writes text to a General-MIDI-module's own onboard LCD, "
     "not the game's screen."),

    (0x1FA8E, "Midi_beginRolandSysEx",
     "sub_1FA8E(): calls the already-named Midi_initDevice; on "
     "success, calls Midi_resetDevice, sends MPU-401 UART-mode "
     "command 0x3F (spinning on Midi_sendCommand until it succeeds), "
     "then sends five bytes via Midi_sendByte: 0xF0 (SysEx start), "
     "0x41 (Roland manufacturer ID), 0x10 (device ID), 0x16 (Roland "
     "MT-32/Sound Canvas model ID), 0x12 (DT1 'Data Set 1' command) "
     "-- the exact standard Roland SysEx header preceding an address+"
     "data+checksum+terminator message. Called from the just-named "
     "Midi_sendDisplayText and sub_1FB10 -- the shared 'init the MIDI "
     "device and begin a Roland Data Set SysEx message' preamble."),

    (0x1FAFE, "Midi_endSysEx",
     "sub_1FAFE(): sends 0xF7 (MIDI SysEx 'End of Exclusive') via "
     "Midi_sendByte, then calls the already-named Midi_shutdown. "
     "Called from the just-named Midi_sendDisplayText and sub_1FB10 "
     "-- the closing half of the Midi_beginRolandSysEx/Midi_endSysEx "
     "pair framing a Roland SysEx message."),

    (0x1FC4E, "Midi_busyWaitDelay",
     "sub_1FC4E(): a calibrated busy-wait -- loops a counter 0 to "
     "0x800 computing counter*counter each iteration (result unused), "
     "purely to burn CPU cycles. Called from the just-named "
     "Midi_sendDisplayText and sub_1FB10, immediately after a Roland "
     "SysEx message, presumably giving the MIDI device time to "
     "process it before the caller continues."),

    (0x1FE5C, "Sound_loadAndStartTrack",
     "sub_1FE5C(): the shared track-loading worker called from both "
     "the already-named Sound_selectTrack and Sound_selectTrackForRoom "
     "(after they've picked a track/room number). No-ops unless a "
     "relevant word_C8582 backend bit is set and a track/room "
     "selector global (word_C8580) is nonzero. Opens the selected "
     ".MUS resource via open_file2(fileNumber, FILETYPE_MUS); when "
     "coming from the room-based path, additionally walks up to 4 "
     "header-described sub-chunks, allocating a handle and reading "
     "each in via new_handle/fsetpos/file_read2/close_file2 (skipping "
     "chunks below a size threshold). Either way ends up with the "
     "main track payload in a shared `ptr` global and copies two "
     "header words into word_C8572/word_C8574. Then dispatches on "
     "backend: if the MIDI bit (word_C8582 bit 2) is set, calls "
     "sub_1F63A (traced earlier this session -- parses the loaded MIDI "
     "data) and, on success, loops calling sub_1F692 up to 256 times "
     "to prime the MIDI event queue, snapshots the clock into the "
     "_tmpSub._val7/_val8 fields Sound_getElapsedPlaybackTime reads, "
     "sets the 'playing' bits, and optionally spins on a ready-"
     "handshake pair (word_C85A0/word_C859E) if a further bit is set. "
     "Otherwise (bit 1 set instead), calls sub_1E1BC(ptr) -- "
     "presumably the OPL2/other-backend equivalent preparation step. "
     "Several inner helpers (sub_1F63A's own callees, sub_1F692, "
     "sub_1E1BC) remain unnamed, but this function's own role -- load "
     "a .MUS track's data and kick off playback on whichever backend "
     "is active -- is clear from its structure and already-named "
     "callers/callees."),

    (0x1F63A, "Midi_prepareTrackData",
     "sub_1F63A(dataPtr, dataSeg, header1, header2): stores dataPtr:"
     "dataSeg into the shared _tmpSub._val10/unk_4DA68 fields, "
     "returning 0 immediately if that pointer is null. Otherwise "
     "calls sub_1FBCE(header1, header2) (unnamed -- presumably "
     "records the two header words the just-named "
     "Sound_loadAndStartTrack read from the MUS file), then calls "
     "sub_1EE06(dataPtr:dataSeg) (unnamed); if that returns 0, bails "
     "out. Otherwise calls sub_1F552 (unnamed; if it returns 0, bails "
     "out) then sub_1F4A0 (unnamed), and returns 1. Called from "
     "Sound_loadAndStartTrack (with the just-loaded MIDI track's data "
     "pointer and the two header words read from the MUS file) and "
     "from sub_1F7D6 -- the MIDI backend's 'parse/validate the loaded "
     "track data and prepare it for playback' step."),

    (0x1F692, "Midi_serviceTick",
     "sub_1F692(): the MIDI backend's per-tick service routine. If "
     "the already-named Midi_stopTrack's busy-loop flag (word_C8532) "
     "is set, delegates straight to the already-named "
     "Midi_stopTrackStep and returns. Otherwise, if a 'playing' flag "
     "(word_C852E) is clear, no-ops. Otherwise services each active "
     "track (calling sub_1F1DE, unnamed, once per track index up to "
     "word_D20F6), then calls sub_1DDA4 (unnamed); if that returns 0, "
     "returns. If a further flag (word_C857E) is set, reconfigures "
     "the MPU-401's active-track set: sends command 5 (spinning until "
     "it succeeds) and command 0xEC with a computed track-count "
     "bitmask, swaps a pair of per-track arrays, then sends further "
     "MPU-401 commands (0xB8, ...; the rest of this ~320-byte function "
     "wasn't traced instruction-by-instruction). Called (up to 256 "
     "times per call) from the already-named Sound_loadAndStartTrack "
     "to prime the MIDI event queue, and repeatedly from sub_201C0 -- "
     "consistent with this being the regular per-frame/per-tick MIDI "
     "service routine, not a one-shot setup step."),

    (0x201C0, "Sound_serviceTick",
     "sub_201C0(): the top-level sound-engine tick dispatcher, guarded "
     "against re-entrancy (word_C85A2: if already nonzero, returns 1 "
     "immediately without doing any work). Otherwise, if a 'sound "
     "active' bit (word_C8582 bit 4) is clear, skips straight to "
     "cleanup. If set, dispatches by backend: MIDI (bit 2) calls the "
     "just-named Midi_serviceTick; else OPL2/other (bit 1) calls "
     "sub_1E950 (unnamed, itself calling the already-named "
     "Opl2_stopTrack elsewhere -- plausibly the OPL2 backend's own "
     "per-tick service routine); else defaults to 0. Based on that "
     "result, sets or clears a bit in word_C8582 (matching the same "
     "bit patterns Opl2_stopTrack manipulates at its own end), then "
     "releases the re-entrancy guard and returns the result. Called "
     "from the already-named room_load and via a data-driven call site "
     "elsewhere -- the shared per-tick service point for whichever "
     "sound backend is currently active."),

    (0x24A42, "Mouse_initCursorSurfaces",
     "sub_24A42(): allocates the two mouse-cursor image surfaces "
     "(the already-declared mouse_surface2, 24x16, and mouse_surface, "
     "24x16) via the already-named Image_allocateSurface, storing "
     "each one's resulting image handle into a scratch dword global "
     "(dword_D21B0/dword_D2234). Then computes the initial mouse "
     "position: x is hardcoded to 319 in video mode 3, or otherwise "
     "centered as (_screenRight - width)/2; y is always centered as "
     "(_ScreenBottom - height)/2. Called (twice) from the already-"
     "named Mouse_init -- the mouse cursor's own surface-allocation-"
     "plus-initial-centering setup step."),

    (0x24FAE, "Mouse_pollDriverState",
     "sub_24FAE(): if bit 3 of the already-named mouseState is set "
     "(the DOS mouse driver is present), calls INT 33h AH=3 (the "
     "standard 'get mouse position and button status' mouse-driver "
     "call) via _int86, then halves the returned cx (x position) if "
     "in video mode 3 (that mode's coordinate space is twice as wide "
     "as the internal one), and passes the resulting (x, y) to the "
     "already-named Commset_btn_setMouse. Called from the already-"
     "named Mouse_pollPosition and get_mouse_buttons -- the shared "
     "low-level 'read the real DOS mouse driver and feed its position "
     "into the game's own mouse state' primitive."),

    (0x27582, "Region_setValueAndStyle",
     "sub_27582(windowNum, regionIndex, val1, style): no-ops if "
     "either windowNum or regionIndex is negative. Otherwise looks up "
     "the actual region slot via the already-named "
     "Windows_regionIndexes[windowNum*35+regionIndex], sets the "
     "already-named Regions_val1[slot]=val1 and "
     "Regions_style[slot]=style, then calls the already-named "
     "Region_fill(windowNum, regionIndex) to redraw it. Called from "
     "the already-named Listbox_add and Listbox_reset -- sets a "
     "listbox item's region value/style (plausibly selected vs. "
     "unselected appearance) and immediately refills it."),

    (0x25C52, "ScalePic_scaleCoordinate",
     "sub_25C52(direction, value): if direction==0xFFFF (-1), returns "
     "3*(value/4) + value%4 -- i.e. value scaled by 3/4 (using two "
     "different truncating-division computations of the same "
     "quotient, redundantly). If direction==1, returns (value*4)/3 -- "
     "the reciprocal 4/3 scale. Any other direction returns value "
     "unchanged. Called from the already-named scale_pic (which "
     "prints ' scale_pic : EGA -> VGA disabled ' when refusing to "
     "scale) -- the coordinate/dimension scaling primitive behind "
     "its EGA<->VGA picture-scaling conversion."),
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
