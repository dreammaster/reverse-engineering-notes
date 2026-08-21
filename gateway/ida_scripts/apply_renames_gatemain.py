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

    ("sub_1535E", "Score_add",
     "Confirmed by decoding msgId 0x803/0x804 from real GATESTR.DAT: "
     "'[Your score has just gone up by %d.' / ' NOTE: You can activate "
     "and deactivate score-change notification using the NOTIFY "
     "command.'. Adds its argument to _score, and if _scoreNotifyEnabled "
     "is set, prints the above (the NOTE only once, gated by "
     "_scoreNotifyTipShown)."),
    ("Persisted_val128", "_score",
     "Confirmed via Score_add (adds to this) and msgId 0x2A: 'You have "
     "achieved a score of %d out of 1600, in %d turns.' -- the two "
     "printf args at that call site are (_turnCount, _score) in that "
     "order, matching the message's two %d's."),
    ("Persisted_val3", "_turnCount",
     "Incremented once per main() game-loop iteration (a classic turn "
     "counter); confirmed as the first %d in msgId 0x2A's "
     "score-and-turns status message, paired with _score -- see above."),
    ("Persisted_val11", "_scoreNotifyEnabled",
     "Gates whether Score_add prints its notification at all -- the "
     "in-game NOTIFY command's persisted toggle, per msgId 0x804's "
     "text (see Score_add's note)."),
    ("Persisted_val12", "_scoreNotifyTipShown",
     "One-time flag: Score_add prints the NOTIFY-command explanation "
     "(msgId 0x804) only the first time a score notification fires, "
     "then sets this so it isn't repeated."),
    ("Persisted_val175", "_gameTicks",
     "Confirmed via msgId 0x29: 'It is Dorman day %d.', computed as "
     "_gameTicks/480 + 1 right before that message -- a real-time game "
     "clock in ticks, 480 ticks per in-universe 'Dorman day'."),
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
