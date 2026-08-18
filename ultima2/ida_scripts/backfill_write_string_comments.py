"""
IDA Pro script: backfills the decoded-text comment on write_string
call sites that are already correctly-fixed string literals but never
got a comment.

Background: fix_inline_strings.py's already_processed() idempotency
check causes fix_call_site() to return early -- before reaching the
idc.set_cmt() line -- for any site that's already a correctly-
terminated string literal. That's the right behavior for avoiding
redundant IDB edits, but it means sites fixed by an earlier pass (or
manually) before comments were part of the fix, or simply skipped on
a later idempotent re-run, permanently never get a comment. Found
2026-08-18 while building the string catalog: 256 of 332 write_string
call sites had no comment despite 256 of those 257 being genuinely
already-fixed (only 1 was truly broken, handled separately by
fix_ending_strings.py).

This script only ever reads the string literal already at each site
and calls idc.set_cmt() -- it never touches string/code classification
or crossrefs, so it's safe to run repeatedly and has no ambiguity
worth a dry-run pass.
"""

import ida_bytes
import ida_nalt
import ida_xref
import ida_ua
import idaapi
import idautils
import idc

TERMINATOR = 0x00


def find_call_sites(func_ea):
    sites = []
    for xref in idautils.XrefsTo(func_ea, 0):
        if xref.type in (ida_xref.fl_CN, ida_xref.fl_CF):
            sites.append(xref.frm)
    return sites


def already_processed(ea):
    flags = ida_bytes.get_flags(ea)
    if not ida_bytes.is_strlit(flags):
        return False
    item_len = ida_bytes.get_item_size(ea)
    if item_len <= 0:
        return False
    return ida_bytes.get_byte(ea + item_len - 1) == TERMINATOR


def main():
    func_ea = idc.get_name_ea_simple("write_string")
    if func_ea == idaapi.BADADDR:
        print("[!] write_string not found -- aborting")
        return

    sites = find_call_sites(func_ea)
    backfilled = 0
    already_had = 0
    still_broken = 0

    for call_ea in sorted(sites):
        if idc.get_cmt(call_ea, 0):
            already_had += 1
            continue

        insn = ida_ua.insn_t()
        if ida_ua.decode_insn(insn, call_ea) == 0:
            print(f"[!] {call_ea:X}: couldn't decode CALL, skipping")
            continue
        str_start = call_ea + insn.size

        if not already_processed(str_start):
            print(f"[!] {call_ea:X}: not a fixed strlit at {str_start:X} -- "
                  "genuinely still broken, not backfilling")
            still_broken += 1
            continue

        # get_strlit_contents() unreliable for some already-fixed items
        # (returns b'' despite clearly valid, non-empty raw bytes --
        # found 2026-08-18, likely a string-subtype mismatch from
        # whatever process originally created some of these). Read the
        # raw bytes directly up to the terminator instead -- simpler
        # and doesn't depend on getting the string subtype right.
        item_len = ida_bytes.get_item_size(str_start)
        raw = ida_bytes.get_bytes(str_start, item_len)
        if raw is None:
            print(f"[!] {call_ea:X}: get_bytes FAILED at {str_start:X}")
            continue
        term_idx = raw.find(b"\x00")
        text = raw[:term_idx] if term_idx != -1 else raw
        if not text:
            print(f"[!] {call_ea:X}: genuinely empty string at {str_start:X}, skipping")
            continue

        idc.set_cmt(call_ea, text.decode("latin-1", "replace"), 0)
        backfilled += 1

    print(f"\n{already_had} already had a comment, {backfilled} backfilled, "
          f"{still_broken} genuinely still broken (not touched).")


if __name__ == "__main__":
    main()
