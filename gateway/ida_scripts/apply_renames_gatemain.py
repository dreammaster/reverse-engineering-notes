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
