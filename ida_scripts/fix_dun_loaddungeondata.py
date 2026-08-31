"""One-off: coerce loadDungeonData (was sub_12E9B) in dun.idb -- the
per-dungeon data loader. IDA's coerce_code pass bailed after the first
`call far` and left the body as `db` bytes. This undefines the body,
re-decodes it linearly, names the function, and gives the file-I/O
run-time thunks it uses their real names.

    .\run_ida_script.ps1 -Idb dun -ScriptName fix_dun_loaddungeondata.py

Recovered flow -- for each of DUNM<n>.BSV, DUNDATA.BSV, DUNOBJ.BSV:
    rtm_11(arraySeg)             push the target BASIC array's descriptor
    build "DUNM"+STR$(ds:1ACAh)+".BSV"  (rtm_D2 STR$, rtm_46 trim, concat)
    rtm_FE63(&name$)             resolve the game-disk path + OPEN
    rtm_FE07(&name$, &destOff)   BLOAD -- read [FD][seg:2][off:2][len:2]
                                header + payload (via rtm_02)
DUNM<n>.BSV -> dungeonMapArray (ds:1E2Ah) offset 0; DUNDATA.BSV -> same
array +0x800; DUNOBJ.BSV -> spriteBank (ds:1E58h).
"""
import idc
import ida_bytes
import ida_funcs
import ida_auto
import ida_segment

START = 0x12E9B
END = 0x12F9C


def coerce(lo, hi):
    ida_bytes.del_items(lo, ida_bytes.DELIT_SIMPLE | ida_bytes.DELIT_NOTRUNC, hi - lo)
    a, n = lo, 0
    while a < hi:
        ln = idc.create_insn(a)
        if ln <= 0:
            ida_bytes.del_items(a, ida_bytes.DELIT_SIMPLE, 1)
            ln = idc.create_insn(a)
        if ln <= 0:
            a += 1
            continue
        n += 1
        a += ln
    return n


def main():
    ida_funcs.del_func(START)
    n = coerce(START, END)
    ida_auto.auto_wait()
    ida_funcs.add_func(START, END)
    ida_auto.auto_wait()

    idc.set_name(START, "loadDungeonData", idc.SN_NOWARN | idc.SN_CHECK)
    idc.set_func_cmt(
        START,
        "Per-dungeon data load (called from processTileFeature on level "
        "entry). For each of DUNM<dungeonNo>.BSV (name built from "
        "ds:1ACAh via STR$ + trim + concat), DUNDATA.BSV and DUNOBJ.BSV: "
        "rtm_11 pushes the target BASIC array's descriptor, rtm_FE63(&name$) "
        "resolves the game-disk path and opens the file, rtm_FE07(&name$, "
        "&destOff) BLOADs it (reads the 7-byte [FD][seg][off][len] header "
        "then the payload). DUNM<n> -> dungeonMapArray (ds:1E2Ah) offset 0; "
        "DUNDATA.BSV -> the same array +0x800; DUNOBJ.BSV -> spriteBank "
        "(ds:1E58h). No pointer-table relocation happens here.",
        1,
    )

    s = ida_segment.get_segm_by_name("seg002")
    if s:
        # rtm_FE63 thunk -> resolve-path + open;  rtm_FE07 thunk -> BLOAD.
        # (0x0674 is the rtm_FE63 thunk, 0x03AF the rtm_11 thunk in dun.)
        for off, name, cmt in (
            (0x0674, "rt_openGameFile",
             "rtm_FE63: resolve a game data-file's path (which drive, per "
             "DRCONFIG.DAT) and OPEN it. Called before every BLOAD / random "
             "GET/PUT. arg &name$."),
        ):
            ea = s.start_ea + off
            idc.set_name(ea, name, idc.SN_NOWARN | idc.SN_CHECK)
            idc.set_cmt(ea, cmt, 1)
        # find the rtm_FE07 thunk by its current name and rename to rt_bload
        for cand in ("rt_relocBsaveImage", "rt_FE07"):
            ea = idc.get_name_ea_simple(cand)
            if ea != idc.BADADDR:
                idc.set_name(ea, "rt_bload", idc.SN_NOWARN | idc.SN_CHECK)
                idc.set_cmt(ea,
                            "rtm_FE07: BLOAD -- read a BSAVE image "
                            "([FD][seg:2][off:2][len:2] header + payload) via "
                            "rtm_02 into the array pushed by rtm_11. "
                            "args (&name$, &destOffset).", 1)
                break
        # undo the earlier bad name on the rtm_11 thunk
        ea = idc.get_name_ea_simple("rt_bloadImage")
        if ea != idc.BADADDR:
            idc.set_name(ea, "rt_11", idc.SN_NOWARN | idc.SN_CHECK)

    print(f"loadDungeonData: coerced {n} insns")
    ida_auto.auto_wait()


main()
