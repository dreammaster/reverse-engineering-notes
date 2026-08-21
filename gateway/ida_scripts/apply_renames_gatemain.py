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

    ("sub_14A37", "Stream_loadFile",
     "Top-level entry: flushes the active window's pending text "
     "(TextWindow_flushPendingText), then reads (Stream_readChunks), "
     "processes (Stream_processChunks), and frees "
     "(Stream_freeChunks) a whole file's worth of chunked buffers."),
    ("sub_28E2C", "TextWindow_flushPendingText",
     "Flushes Windows_pendingText for the current Windows_activeWindow "
     "via TextWindow_addDirect if that window has buffered/queued text "
     "waiting -- distinct from the already-named TextWindow_flushText."),
    ("sub_1DDF6", "Stream_readChunks",
     "Opens filename, seeks to find its total size, then reads it into "
     "up to 8 separately-allocated RAM buffers (dword_C84A6[] array, "
     "each up to ~64KB since one DOS allocation can't hold an arbitrary "
     "file), aborting the whole read early if a key is pressed "
     "(Events_isKeyPending) or if word_C8582 bit 8 is clear or bit "
     "0x40 is set."),
    ("sub_1E052", "Stream_processChunks",
     "Walks the same buffer array Stream_readChunks filled, dispatching "
     "each non-empty one through Stream_processChunk -- again "
     "abortable via Events_isKeyPending between chunks, and gated by "
     "the same word_C8582 bits."),
    ("sub_1E0E8", "Stream_freeChunks",
     "Frees all 8 of the buffer-array slots via kill_pointer_ -- the "
     "cleanup step after Stream_readChunks/Stream_processChunks."),
    ("sub_180E3", "Stream_processChunk",
     "Per-chunk handler: reads a couple of header bytes from the "
     "buffer (offsets 5 and 7, before the 32-byte header/payload "
     "boundary already established) into cx/bx, then calls the actual "
     "processing logic through a configurable callback function "
     "pointer (word_C84D0) with the payload pointer (buffer+0x20) -- "
     "not traced further since the callback's actual target varies by "
     "caller and wasn't identified this pass."),
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
