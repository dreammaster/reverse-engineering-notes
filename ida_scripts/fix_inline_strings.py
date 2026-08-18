"""
IDA Pro script: fix the "inline string after CALL" pattern in Ultima II (DOS).

Functions like write_string are called with the text to print stored
immediately after the CALL instruction, in the code stream itself.
write_string reads the return address off the stack, treats it as a
pointer to a null-terminated ASCII string, prints it, then rewrites the
return address to point just past the string before it RETs -- so
execution resumes after the string, not after the CALL.

IDA doesn't know that, so it:
  1. Disassembles the string bytes as garbage instructions.
  2. Draws a normal fallthrough edge from the CALL into those bytes,
     which is what breaks the function graph once you fix it by hand
     with U / A / C -- the graph builder still thinks the CALL falls
     through into a node that no longer has any code in it.

For every call site to one of TARGET_FUNCS, this script:
  1. Finds the extent of the null-terminated string right after the call.
  2. Converts it to a proper ASCII string literal.
  3. Removes the bogus "falls through into the string" edge.
  4. Adds the real edge: CALL -> first instruction after the string.
  5. Makes sure that instruction (and a bounded stretch after it) is
     re-disassembled as code, since it was previously misaligned.
  6. Drops a comment on the CALL showing the string it prints.

USAGE
-----
1. BACK UP your .idb/.i64 first (or at least know Ctrl+Z works).
2. Add any other function names with the same calling convention to
   TARGET_FUNCS below.
3. Run once with DRY_RUN = True and check the Output window log --
   nothing is modified in this mode.
4. Set DRY_RUN = False and run again. Re-running later is safe/idempotent:
   already-fixed call sites (where the string literal already exists)
   are detected and skipped.

Run via File > Script file... (Alt+F7) or File > Recent scripts (Shift+F9).

RE-RUN NOTE (2026-08-17): flipped back to DRY_RUN = True for a fresh
pass. This script was last run (validated against ORIGIN/PROUDLY
PRESENTS/BY LORD BRITISH) *before* the A-Z command_jump_table got
resolved -- that fix exposed huge amounts of previously-orphaned code
across all 26 command handlers (14.5k-line .asm diff), including new
`call write_string` sites this script never had a chance to see, since
find_call_sites() only finds xrefs IDA already recognizes. `view`'s
"aView" gap (and `board`'s Ship/Frigate/Airplane gaps, already fixed
by hand) are exactly this: real write_string calls that only became
disassembled instructions after the jump-table fix. Re-running now,
scoped to the whole binary as always, should catch `view`'s gap and
may surface others in the other 25 commands that haven't been read
line-by-line yet. Dry-run first given the number of sites this could
now touch is unknown and probably much larger than last time.

FOLLOW-UP (same day): first re-run fixed 2 new sites cleanly (ONE
MOMENT PLEASE!, MINAX IS DEAD!!) but did NOT catch `view`'s "aView"
gap -- turned out already_processed() was too permissive (see its
docstring) and was treating an already-malformed 4-byte 'VIEW' string
literal as "already fixed" without checking it was actually
terminated correctly. Fixed that check; re-running again with
DRY_RUN = True to see what else this now catches before applying.
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

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

# Names of functions that consume an inline string via the return address.
# Add more here as you identify them.
TARGET_FUNCS = [
    "write_string",
]

# Terminator byte for the inline strings (confirmed: null-terminated).
TERMINATOR = 0x00

# Safety limit: how far to scan looking for the terminator before giving up
# and flagging the site for manual review instead of guessing.
MAX_STRING_LEN = 512

# How far past the string to clear/re-disassemble so downstream code that
# was previously misaligned gets a chance to realign correctly. Stops early
# at the first existing name/label so we don't clobber unrelated code.
RESYNC_WINDOW = 0x100

DRY_RUN = False  # flip to False once the dry-run log looks right


def find_call_sites(func_ea):
    """All code refs to func_ea that are actual CALL instructions."""
    sites = []
    for xref in idautils.XrefsTo(func_ea, 0):
        if xref.type in (ida_xref.fl_CN, ida_xref.fl_CF):
            sites.append(xref.frm)
    return sites


def already_processed(ea):
    """True if ea already looks like a CORRECTLY-terminated string literal
    (idempotent re-run). Deliberately checks more than just "is this typed
    as a string" -- found via `view`'s "aView" site (2026-08-17): it was
    already a strlit-typed item, just a malformed 4-byte one ('VIEW', no
    terminator) from some earlier partial classification, and the old
    is_strlit-only check treated that as permanently "already fixed",
    silently skipping the real fix forever. Now also verifies the item's
    last byte is actually the terminator before trusting it."""
    flags = ida_bytes.get_flags(ea)
    if not ida_bytes.is_strlit(flags):
        return False
    item_len = ida_bytes.get_item_size(ea)
    if item_len <= 0:
        return False
    return ida_bytes.get_byte(ea + item_len - 1) == TERMINATOR


def find_terminator(start_ea):
    """Address of the terminator byte, or None if not found within range."""
    ea = start_ea
    end = start_ea + MAX_STRING_LEN
    while ea < end:
        if not ida_bytes.is_loaded(ea):
            return None
        if ida_bytes.get_byte(ea) == TERMINATOR:
            return ea
        ea += 1
    return None


def resync_after(cont_ea):
    """Clear a bounded stretch after cont_ea and queue it for re-analysis,
    so instructions that were misaligned by the old bogus disassembly get
    rebuilt starting from the now-correct cont_ea boundary."""
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


def fix_call_site(call_ea, func_name):
    insn = ida_ua.insn_t()
    if ida_ua.decode_insn(insn, call_ea) == 0:
        print(f"[!] {call_ea:X}: couldn't decode CALL instruction, skipping")
        return False

    str_start = call_ea + insn.size

    if already_processed(str_start):
        print(f"[=] {call_ea:X}: string at {str_start:X} already fixed, skipping")
        return True

    term_ea = find_terminator(str_start)
    if term_ea is None:
        print(f"[!] {call_ea:X}: no terminator within {MAX_STRING_LEN} bytes "
              f"of {str_start:X} -- skipping, needs manual review")
        return False

    str_len = term_ea - str_start + 1  # include the terminator byte
    cont_ea = term_ea + 1

    if DRY_RUN:
        raw = ida_bytes.get_bytes(str_start, str_len - 1)
        print(f"[dry] {call_ea:X}: string @ {str_start:X} len={str_len} "
              f"{raw!r}, resumes @ {cont_ea:X}")
        return True

    # 1. Remove the bogus fallthrough edge from the CALL into the string.
    ida_xref.del_cref(call_ea, str_start, 0)

    # 2. Clear whatever garbage IDA made of the string bytes, then make
    #    a proper string literal there.
    ida_bytes.del_items(str_start, ida_bytes.DELIT_SIMPLE, str_len)
    if not ida_bytes.create_strlit(str_start, str_len, ida_nalt.STRTYPE_C):
        print(f"[!] {call_ea:X}: create_strlit failed at {str_start:X}")
        return False

    # 3. Re-disassemble from where execution actually resumes.
    resync_after(cont_ea)

    # 4. Wire up the real control-flow edge: CALL -> code after the string.
    ida_xref.add_cref(call_ea, cont_ea, ida_xref.fl_JN)

    # 5. Comment the call with the string it prints.
    text = ida_bytes.get_strlit_contents(str_start, str_len, ida_nalt.STRTYPE_C)
    if text is not None:
        idc.set_cmt(call_ea, text.decode("latin-1", "replace"), 0)

    print(f"[+] {call_ea:X}: fixed, string @ {str_start:X} len={str_len}, "
          f"resumes @ {cont_ea:X}")
    return True


def main():
    total = fixed = 0
    for name in TARGET_FUNCS:
        func_ea = idc.get_name_ea_simple(name)
        if func_ea == idaapi.BADADDR:
            print(f"[!] function '{name}' not found, skipping")
            continue

        # So future/automatic analysis of any not-yet-visited call sites
        # doesn't assume a normal fallthrough return either.
        if not DRY_RUN:
            pfn = ida_funcs.get_func(func_ea)
            if pfn is not None and not (pfn.flags & ida_funcs.FUNC_NORET):
                pfn.flags |= ida_funcs.FUNC_NORET
                ida_funcs.update_func(pfn)

        sites = find_call_sites(func_ea)
        print(f"--- {name} @ {func_ea:X}: {len(sites)} call site(s) ---")
        for call_ea in sites:
            total += 1
            if fix_call_site(call_ea, name):
                fixed += 1

    mode = "DRY RUN -- nothing changed" if DRY_RUN else "applied"
    print(f"\nDone ({mode}): {fixed}/{total} call sites fixed.")
    if not DRY_RUN:
        ida_auto.auto_wait()


if __name__ == "__main__":
    main()
