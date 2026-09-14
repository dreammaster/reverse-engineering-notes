"""
Recovers the ErrorTable jump/dispatch structure at 0x28A31 (seg102), which
the fresh 8.2 auto-analysis left as unexplored/undifferentiated bytes since
nothing calls these handlers directly -- they're only reached indirectly
through this table (referenced from ErrorCheck+0x16). The IDA-8.3-era
database had this fully worked out (each entry defined as its own tiny
`proc`, each loading one specific error-message offset before jumping to
ErrorExit) -- ground truth cross-checked directly against the committed
pre-session yendor2.asm (git show HEAD:yendor2/yendor2.asm). This script
rebuilds that structure from scratch against the current database (function
boundaries, table-entry/message operand "offset" typing, and names derived
directly from each handler's own referenced error-message string -- not
guessed), since none of it survived the .idb recreation.

Table layout: ErrorTable dw[20] at 0x28A31 (ErrorExit + 19 handlers),
terminated by a single 0 byte at 0x28A59, immediately followed by seg103
at 0x28A5A.

Run via:
    .\run_ida_script.ps1 fix_error_table.py
"""
import idc
import ida_bytes
import ida_funcs
import ida_offset
import ida_name

ERROR_TABLE_EA = 0x28A31
N_ENTRIES = 20

# (handler ea, message-string name or None if this entry is the generic
# ErrorExit itself, human name suffix or None to leave it sub_XXXXX --
# the handful pointing at the unnamed "." strings (asc_28971 etc.) carry no
# real semantic signal, so they're deliberately left unnamed rather than
# invented).
ENTRIES = [
    (0x2899C, None, None),  # ErrorExit itself (already a named function)
    (0x289CE, "aMemoryAllocati", "MemoryAllocation"),
    (0x289D3, "aProblemWithPic", "ProblemWithPictureVga"),
    (0x289D8, "aProblemWithPal", "ProblemWithPalette"),
    (0x289DD, "aRequiredExpand", "RequiredExpandedMemMgr"),
    (0x289E2, "aMinimumOf1mbEx", "MinimumExpandedRam"),
    (0x289E7, "aAnEmmMappingEr", "EmmMappingError"),
    (0x289EC, "aProblemWithMus", "ProblemWithMusic"),
    (0x289F1, "aProblemWithADr", "ProblemWithDriver"),
    (0x289F6, "aProblemWithWor", "ProblemWithWorldDat"),
    (0x289FB, "aProblemWithCur", "ProblemWithCurgame"),
    (0x28A00, "aProblemWithASa", "ProblemWithSavedGame"),
    (0x28A05, "aExpandedMemory", "ExpandedMemAllocFail"),
    (0x28A0A, "aProblemRetreiv", "ProblemRetrievingText"),
    (0x28A0F, "aProblemRetreiv_0", "ProblemRetrievingNpc"),
    (0x28A14, "aProblemRetreiv_1", "ProblemRetrievingConvo"),
    (0x28A19, "asc_28971", None),
    (0x28A1F, "asc_28975", None),
    (0x28A25, "asc_28979", None),
    (0x28A2B, "asc_28979", None),
]

assert len(ENTRIES) == N_ENTRIES


def log(msg):
    print(msg)


def main():
    # --- 1. define/verify each handler function ---
    made_func, already_func, func_fail = 0, 0, []
    for ea, msg_name, human in ENTRIES:
        f = ida_funcs.get_func(ea)
        if f and f.start_ea == ea:
            already_func += 1
            continue
        ok = ida_funcs.add_func(ea)
        if ok:
            made_func += 1
        else:
            func_fail.append(ea)
    log(f"functions: {already_func} already defined, {made_func} newly created, "
        f"{len(func_fail)} failed: {[hex(e) for e in func_fail]}")

    # --- 2. rename handlers with a real semantic name derived from their
    #        own referenced message string ---
    renamed, rename_fail = 0, []
    for ea, msg_name, human in ENTRIES:
        if human is None:
            continue
        new_name = f"ShowErr_{human}"
        if idc.get_name(ea) == new_name:
            continue
        res = ida_name.set_name(ea, new_name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
        if res:
            renamed += 1
        else:
            rename_fail.append((ea, new_name))
    log(f"renamed {renamed} handlers, {len(rename_fail)} failed: {rename_fail}")

    # --- 3. type each handler's "mov ax, offset <message>" immediate as a
    #        real offset (so it displays symbolically instead of raw hex,
    #        and creates the dxref to the string) ---
    # Derive the segment-relative base once from a known-good pair, then
    # reuse it (all entries are the same segment).
    base = None
    for ea, msg_name, human in ENTRIES:
        if msg_name is None:
            continue
        msg_ea = idc.get_name_ea_simple(msg_name)
        if msg_ea == idc.BADADDR:
            continue
        raw = idc.get_operand_value(ea, 1)
        base = msg_ea - raw
        break
    log(f"derived offset base = {base:#x}" if base is not None else "could not derive base!")

    msg_ok, msg_fail = 0, []
    for ea, msg_name, human in ENTRIES:
        if msg_name is None or base is None:
            continue
        msg_ea = idc.get_name_ea_simple(msg_name)
        if msg_ea == idc.BADADDR:
            msg_fail.append((ea, msg_name, "name not found"))
            continue
        ok = ida_offset.op_plain_offset(ea, 1, base)
        if ok and idc.get_operand_value(ea, 1) == msg_ea:
            msg_ok += 1
        else:
            msg_fail.append((ea, msg_name, "op_plain_offset mismatch/failed"))
    log(f"message-offset typing: {msg_ok} ok, {len(msg_fail)} failed: {msg_fail}")

    # --- 4. type the ErrorTable's own 20 words as offsets to their handlers ---
    # First make sure the table region is actually a run of word items.
    ida_bytes.del_items(ERROR_TABLE_EA, ida_bytes.DELIT_SIMPLE, N_ENTRIES * 2)
    for i in range(N_ENTRIES):
        ea = ERROR_TABLE_EA + i * 2
        ida_bytes.create_word(ea, 2)

    tbl_base = None
    raw0 = ida_bytes.get_wide_word(ERROR_TABLE_EA)
    tbl_base = ENTRIES[0][0] - raw0  # entry 0 is ErrorExit, known address
    log(f"derived table base = {tbl_base:#x}")

    tbl_ok, tbl_fail = 0, []
    for i, (target_ea, _, _) in enumerate(ENTRIES):
        ea = ERROR_TABLE_EA + i * 2
        ok = ida_offset.op_plain_offset(ea, 0, tbl_base)
        if ok and idc.get_operand_value(ea, 0) == target_ea:
            tbl_ok += 1
        else:
            tbl_fail.append((ea, target_ea))
    log(f"table-entry typing: {tbl_ok} ok, {len(tbl_fail)} failed: "
        f"{[(hex(a), hex(b)) for a, b in tbl_fail]}")

    ida_name.set_name(ERROR_TABLE_EA, "ErrorTable", ida_name.SN_NOWARN | ida_name.SN_FORCE)
    ida_bytes.set_cmt(ERROR_TABLE_EA,
                       "Jump table of error-message handlers, indexed from ErrorCheck. "
                       "Each entry sets AX to an error-message offset (into the block "
                       "starting near aMemoryAllocati) then falls into ErrorExit.",
                       False)


main()
