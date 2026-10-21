"""
KEH.EXE: merge IDA mis-split function fragments into their real parents and create
missed functions (qsort comparators). Idempotent. Run with the normal export:

    run_ida_script.ps1 -Idb KEH -ScriptName fix_funcs_keh.py
"""
import idc
import idautils
import ida_funcs
import ida_auto
import ida_name

# (parent start, new end, name)
MERGES = [
    (0x11FB7, 0x1251E, "itemActionMenu"),
    (0x13716, 0x138CA, "postCombatCleanup"),
    (0x14F1A, 0x15335, "buildCombatants"),
    (0x15390, 0x154B0, "sortCombatants"),
    (0x15691, 0x1695C, "runCombat"),
    (0x16F3A, 0x16F6B, "attrBonus"),
    (0x17140, 0x1738A, "retrieveStoreMember"),
    (0x17854, 0x17AAB, "checkTimedEffects"),
]
# (start, end, name) -- functions IDA never created
NEWFUNCS = [
    (0x15335, 0x1534C, "cmpCombatantsBySpeedDesc"),
    (0x1534C, 0x15363, "cmpCombatantsBySpeedAsc"),
    (0x15363, 0x15390, "cmpCombatantsByIndex"),
]

for start, end, name in MERGES:
    inside = [f for f in idautils.Functions(start, end)]
    cur = ida_funcs.get_func(start)
    if cur and cur.start_ea == start and cur.end_ea == end:
        print("ok already %X-%X %s" % (start, end, name))
        continue
    for f in inside:
        ida_funcs.del_func(f)
    ida_auto.plan_and_wait(start, end)
    ok = ida_funcs.add_func(start, end)
    ida_name.set_name(start, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN)
    print("%s %X-%X %s (removed %d)" % ("merged" if ok else "FAILED", start, end, name, len(inside)))

for start, end, name in NEWFUNCS:
    if ida_funcs.get_func(start) and ida_funcs.get_func(start).start_ea == start:
        print("ok already %X %s" % (start, name))
        continue
    ida_auto.plan_and_wait(start, end)
    ok = ida_funcs.add_func(start, end)
    ida_name.set_name(start, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN)
    print("%s new %X-%X %s" % ("added" if ok else "FAILED", start, end, name))

# drop stale fragment labels/comments left inside the merged functions
for ea in (0x1202D, 0x12065, 0x12074, 0x13838, 0x15220, 0x1542D, 0x16F43, 0x17261, 0x17970, 0x16867, 0x16328):
    f = ida_funcs.get_func(ea)
    if f and f.start_ea == ea:
        continue
    nm = idc.get_name(ea)
    if nm and "_part" in nm or nm in ("runCombatExecute", "printInnocentKillPenalty"):
        ida_name.set_name(ea, "", ida_name.SN_NOWARN)
        print("cleared label %X (%s)" % (ea, nm))
    idc.set_cmt(ea, "", 1)
