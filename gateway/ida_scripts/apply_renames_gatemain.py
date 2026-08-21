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
]


def main():
    print(f"DRY_RUN = {DRY_RUN}")
    for ea, new_name, note in RENAMES:
        old_name = idc.get_name(ea)
        if DRY_RUN:
            print(f"{ea:#x}: {old_name!r} -> {new_name!r}   ({note})")
        else:
            ok = idc.set_name(ea, new_name, idc.SN_NOCHECK)
            print(f"{ea:#x}: {old_name!r} -> {new_name!r}   ok={ok}")


main()
