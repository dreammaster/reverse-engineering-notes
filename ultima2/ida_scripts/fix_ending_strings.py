"""
IDA Pro script: fixes the 2 orphaned write_string call sites that make
up the game's actual ending/victory sequence -- found while building
the string catalog (docs/roadmap.md's "string catalog" item). Neither
site is inside any function IDA currently recognizes (reached via
ordinary fallthrough flow from 0x1734C, not a proc boundary), so the
general sweep (fix_inline_strings.py) never found them via XrefsTo.

Same technique as fix_inline_strings.py: find the terminator, convert
the inline bytes to a proper string literal, remove the bogus
CALL->data fallthrough edge, add the real CALL->(post-data) edge,
resync the following bytes as code, and comment the CALL with the
decoded text.

Site 1 (0x1734F): "\x8D\x8DYOU HAVE SAVED THE UNIVERSE,\x8DAND
COMPLETED ULTIMA ][! SEEK\x8DNOW TO CONQUER WICKED EXODUS,\x8D" -- the
game's actual victory/ending message.

Site 2 (0x173AE): "FOUND IN ULTIMA ]I[-D ]II[-P!" -- an Easter-egg
teaser for future games (]I[ = Ultima III, ]II[ = Ultima IV), reached
by ordinary fallthrough right after site 1's terminator.

USAGE: DRY_RUN=False -- both sites already investigated and confirmed
via read-only diagnostics (investigate_uncommented_write_string.py),
same risk profile as the general sweep's already-validated fixes.
"""

import idaapi
import ida_auto
import ida_bytes
import ida_nalt
import ida_name
import ida_ua
import ida_xref
import idc

DRY_RUN = False

RESYNC_WINDOW = 0x100
TERMINATOR = 0x00
MAX_STRING_LEN = 512

# (call_ea, note)
SITES = [
    (0x1734F, "the game's actual victory/ending message"),
    (0x173AE, "Easter-egg teaser for future Ultima games, reached by "
              "ordinary fallthrough right after site 1"),
]


def find_terminator(start_ea):
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


def fix_site(call_ea, note):
    insn = ida_ua.insn_t()
    if ida_ua.decode_insn(insn, call_ea) == 0:
        print(f"[!] {call_ea:X}: couldn't decode CALL instruction")
        return

    str_start = call_ea + insn.size
    term_ea = find_terminator(str_start)
    if term_ea is None:
        print(f"[!] {call_ea:X}: no terminator found within {MAX_STRING_LEN} bytes")
        return

    str_len = term_ea - str_start + 1
    cont_ea = term_ea + 1

    if DRY_RUN:
        raw = ida_bytes.get_bytes(str_start, str_len - 1)
        print(f"[dry] {call_ea:X}: string @ {str_start:X} len={str_len} "
              f"{raw!r}, resumes @ {cont_ea:X}")
        return

    ida_xref.del_cref(call_ea, str_start, 0)
    ida_bytes.del_items(str_start, ida_bytes.DELIT_SIMPLE, str_len)
    if not ida_bytes.create_strlit(str_start, str_len, ida_nalt.STRTYPE_C):
        print(f"[!] {call_ea:X}: create_strlit FAILED at {str_start:X}")
        return
    resync_after(cont_ea)
    ida_xref.add_cref(call_ea, cont_ea, ida_xref.fl_JN)

    text = ida_bytes.get_strlit_contents(str_start, str_len, ida_nalt.STRTYPE_C)
    if text is not None:
        idc.set_cmt(call_ea, text.decode("latin-1", "replace"), 0)

    print(f"[+] {call_ea:X}: fixed, string @ {str_start:X} len={str_len}, "
          f"resumes @ {cont_ea:X} -- {note}")


def main():
    for call_ea, note in SITES:
        fix_site(call_ea, note)
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the fixes took.")
        ida_auto.auto_wait()


if __name__ == "__main__":
    main()
