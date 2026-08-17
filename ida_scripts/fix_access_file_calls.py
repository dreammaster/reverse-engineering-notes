"""
IDA Pro script: fix the "inline filename after CALL access_file" pattern in
Ultima II (DOS).

access_file (asm ~17318) uses the same return-address trick as
write_string, but for an 8-byte, space-padded DOS filename (no dot, no
extension) instead of a null-terminated string:

    pop     bx              ; bx = return address -> inline filename bytes
    add     bx, 8           ; skip the 8 filename bytes
    push    bx              ; corrected return address
    ...                     ; copies the 8 bytes at the *original* return
                            ; address into an FCB filename field

So every `call access_file` is immediately followed by exactly 8 bytes of
ASCII, e.g. 'MAPXFF  ', 'MONXFF  ', 'PLAYER  ', 'MONSTERS'. Some call sites
already happen to show a readable string (prior auto-analysis or manual
work got lucky); others still show garbage disassembled as code. Either
way the bogus CALL-falls-through-into-data graph edge is still wrong, so
this script always fixes the crossrefs and only (re)creates the string
data when what's there isn't already non-code.

Same usage pattern as fix_inline_strings.py: dry run first, back up the
idb, then apply.
"""

import idaapi
import idautils
import idc
import ida_bytes
import ida_funcs
import ida_xref
import ida_auto
import ida_ua
import ida_nalt
import ida_name

TARGET_FUNCS = [
    "access_file",
]

FILENAME_LEN = 8  # fixed-width, space-padded, no terminator

RESYNC_WINDOW = 0x100

DRY_RUN = False  # flip to False once the dry-run log looks right


def find_call_sites(func_ea):
    sites = []
    for xref in idautils.XrefsTo(func_ea, 0):
        if xref.type in (ida_xref.fl_CN, ida_xref.fl_CF):
            sites.append(xref.frm)
    return sites


def needs_data_fix(ea):
    """True if ea is currently disassembled as code (i.e. still broken)."""
    return ida_bytes.is_code(ida_bytes.get_flags(ea))


def resync_after(cont_ea):
    cap = cont_ea + RESYNC_WINDOW
    stop = cap
    ea = ida_bytes.next_head(cont_ea, cap)
    while ea != idaapi.BADADDR and ea < cap:
        if ida_name.get_name(ea):
            stop = ea
            break
        ea = ida_bytes.next_head(ea, cap)

    ida_bytes.del_items(cont_ea, ida_bytes.DELIT_SIMPLE, stop - cont_ea)
    ida_ua.create_insn(cont_ea)
    ida_auto.auto_mark_range(cont_ea, stop, ida_auto.AU_CODE)


def fix_call_site(call_ea):
    insn = ida_ua.insn_t()
    if ida_ua.decode_insn(insn, call_ea) == 0:
        print(f"[!] {call_ea:X}: couldn't decode CALL instruction, skipping")
        return False

    str_start = call_ea + insn.size
    cont_ea = str_start + FILENAME_LEN

    raw = ida_bytes.get_bytes(str_start, FILENAME_LEN)
    if raw is None or len(raw) != FILENAME_LEN:
        print(f"[!] {call_ea:X}: couldn't read {FILENAME_LEN} bytes at "
              f"{str_start:X}, skipping")
        return False
    text = raw.decode("latin-1", "replace")

    if DRY_RUN:
        broken = needs_data_fix(str_start)
        print(f"[dry] {call_ea:X}: filename @ {str_start:X} = {text!r} "
              f"({'currently code -- needs fix' if broken else 'already data'}), "
              f"resumes @ {cont_ea:X}")
        return True

    if needs_data_fix(str_start):
        ida_bytes.del_items(str_start, ida_bytes.DELIT_SIMPLE, FILENAME_LEN)
        if not ida_bytes.create_strlit(str_start, FILENAME_LEN, ida_nalt.STRTYPE_C):
            print(f"[!] {call_ea:X}: create_strlit failed at {str_start:X}")
            return False
        resync_after(cont_ea)

    # Always fix the crossrefs -- even sites where the data already looked
    # right may still carry the bogus fallthrough edge into it.
    ida_xref.del_cref(call_ea, str_start, 0)
    ida_xref.add_cref(call_ea, cont_ea, ida_xref.fl_JN)

    idc.set_cmt(call_ea, text.rstrip(), 0)

    print(f"[+] {call_ea:X}: fixed, filename @ {str_start:X} = {text!r}, "
          f"resumes @ {cont_ea:X}")
    return True


def main():
    total = fixed = 0
    for name in TARGET_FUNCS:
        func_ea = idc.get_name_ea_simple(name)
        if func_ea == idaapi.BADADDR:
            print(f"[!] function '{name}' not found, skipping")
            continue

        if not DRY_RUN:
            pfn = ida_funcs.get_func(func_ea)
            if pfn is not None and not (pfn.flags & ida_funcs.FUNC_NORET):
                pfn.flags |= ida_funcs.FUNC_NORET
                ida_funcs.update_func(pfn)

        sites = find_call_sites(func_ea)
        print(f"--- {name} @ {func_ea:X}: {len(sites)} call site(s) ---")
        for call_ea in sites:
            total += 1
            if fix_call_site(call_ea):
                fixed += 1

    mode = "DRY RUN -- nothing changed" if DRY_RUN else "applied"
    print(f"\nDone ({mode}): {fixed}/{total} call sites fixed.")
    if not DRY_RUN:
        ida_auto.auto_wait()


if __name__ == "__main__":
    main()
