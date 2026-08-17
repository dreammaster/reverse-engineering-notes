"""
IDA Pro script: resolve the A-Z command dispatch jump table in Ultima II
(DOS).

canMoveToTile (asm ~1746-1760, anchored on label loc_10A1F) has:

    cmp     al, 5Ah         ; 'Z'
    jbe     short loc_10A1F ...
loc_10A1F:
    stc
    cmc
    sbb     al, 41h         ; al -= 'A'  (bounds already checked: 'A'..'Z')
    add     al, al          ; al *= 2    (word-size index)
    mov     ah, 0
    mov     si, ax
    mov     bx, cs:[si+730h]
    jmp     bx

This is a 26-entry near-jump table at (segment start + 0x730), one word
per letter A-Z -- almost certainly the single-key command dispatch
(Attack, Board, Cast, ..., Talk, ...). IDA's switch-idiom analysis
doesn't recognize this hand-rolled shape, so it never created the table
or any xrefs through it. Anything reachable *only* via this table --
plausibly the 'T' (Talk) handler, which per docs/overview.md should lead
to the orphaned TLKXFF-loading routine near load_map -- stays invisible:
no code, no xrefs, easy to miss entirely.

This script locates the "jmp bx" via the loc_10A1F anchor, reads the
26-word table right after it, and for each letter:
  - reports the target address and whether it's already code/named/undefined
  - (non-dry-run) creates a proper word array for the table, adds a real
    code xref from the jmp to each target, and kicks off code creation +
    re-analysis at any target that's still undefined.

Run DRY_RUN = True first and eyeball the printed table -- this is a
structural guess (jump table shape, table location, near-vs-far entries)
that should be sanity checked against what you see in IDA before writing
anything, unlike the other two fix scripts which were reading data whose
exact bytes were already known from the .asm export.
"""

import idaapi
import idautils
import idc
import ida_bytes
import ida_ua
import ida_allins
import ida_xref
import ida_auto
import ida_segment
import ida_name

ANCHOR_LABEL = "loc_10A1F"  # start of the bounds-checked index computation
MAX_ANCHOR_SCAN = 12        # how many instructions past the anchor to search for "jmp bx"
TABLE_OFFSET = 0x730        # displacement used in "mov bx, cs:[si+730h]"
NUM_LETTERS = 26

RESYNC_WINDOW = 0x100

DRY_RUN = True


BX_REG = 3  # IDA's x86 register numbering mirrors the ModRM reg-field
            # encoding (0=ax,1=cx,2=dx,3=bx,4=sp,5=bp,6=si,7=di) -- this
            # is an x86 architectural constant, not an IDA-version-specific
            # one, so hardcoding it is safe (idc.str2reg isn't available
            # in this IDA version to look it up dynamically).


def find_jmp_bx(start_ea):
    """Scan forward from start_ea for a decoded 'jmp bx' instruction.
    Decodes opcode/operand directly instead of parsing disassembly text,
    since text-generation helper/flag names have moved around across IDA
    versions (idc.tag_remove, idc.GENDSM_REMOVE_TAGS both gone in 8.3)."""
    ea = start_ea
    seen = []
    for _ in range(MAX_ANCHOR_SCAN):
        if ea == idaapi.BADADDR:
            seen.append("(next_head returned BADADDR)")
            break
        insn = ida_ua.insn_t()
        ok = ida_ua.decode_insn(insn, ea) != 0
        mnem = idc.print_insn_mnem(ea) if ok else "?"
        if ok:
            op1 = f"type={insn.Op1.type} reg={insn.Op1.reg}"
            seen.append(f"{ea:X}: itype={insn.itype} mnem={mnem!r} Op1({op1})")
            if (insn.itype == ida_allins.NN_jmp
                    and insn.Op1.type == ida_ua.o_reg
                    and insn.Op1.reg == BX_REG):
                return ea, seen
        else:
            seen.append(f"{ea:X}: decode_insn failed")
        ea = ida_bytes.next_head(ea, ea + 16)
    return None, seen


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


def main():
    anchor_ea = idc.get_name_ea_simple(ANCHOR_LABEL)
    if anchor_ea == idaapi.BADADDR:
        print(f"[!] anchor label '{ANCHOR_LABEL}' not found -- has this area "
              f"been renamed/refactored since this script was written? "
              f"Locate the 'mov bx, cs:[si+730h] / jmp bx' sequence near "
              f"canMoveToTile manually and update ANCHOR_LABEL.")
        return

    jmp_ea, trace = find_jmp_bx(anchor_ea)
    if jmp_ea is None:
        print(f"[!] couldn't find 'jmp bx' within {MAX_ANCHOR_SCAN} "
              f"instructions of {ANCHOR_LABEL} ({anchor_ea:X}). "
              f"Instructions actually seen (Op1 type=1 means o_reg; "
              f"reg=3 would be bx):")
        for line in trace:
            print(f"    {line}")
        return

    seg = ida_segment.getseg(jmp_ea)
    if seg is None:
        print(f"[!] no segment found containing {jmp_ea:X}")
        return

    table_ea = seg.start_ea + TABLE_OFFSET
    print(f"anchor {ANCHOR_LABEL} @ {anchor_ea:X}, jmp bx @ {jmp_ea:X}, "
          f"segment {seg.start_ea:X}, table @ {table_ea:X}\n")

    entries = []
    for i in range(NUM_LETTERS):
        letter = chr(ord('A') + i)
        entry_ea = table_ea + i * 2
        word_val = ida_bytes.get_word(entry_ea)
        target_ea = seg.start_ea + word_val
        flags = ida_bytes.get_flags(target_ea)
        state = ("code" if ida_bytes.is_code(flags) else
                 "data" if ida_bytes.is_data(flags) else
                 "undefined")
        name = ida_name.get_name(target_ea)
        entries.append((letter, entry_ea, word_val, target_ea, state, name))
        print(f"  {letter}: table[{i:2}] @ {entry_ea:X} = {word_val:04X} "
              f"-> {target_ea:X} ({state}{', ' + name if name else ''})")

    if DRY_RUN:
        print("\n[dry] nothing changed. Check the targets above look like "
              "plausible code addresses (e.g. the 'T' entry landing near "
              "the orphaned TLKXFF block) before setting DRY_RUN = False.")
        return

    # Build a proper word array for the table itself.
    ida_bytes.del_items(table_ea, ida_bytes.DELIT_SIMPLE, NUM_LETTERS * 2)
    for i in range(NUM_LETTERS):
        ida_bytes.create_data(table_ea + i * 2, ida_bytes.FF_WORD, 2, idaapi.BADNODE)
    if not ida_name.get_name(table_ea):
        idc.set_name(table_ea, "command_jump_table", idc.SN_NOWARN)

    for letter, entry_ea, word_val, target_ea, state, name in entries:
        idc.set_cmt(entry_ea, f"'{letter}' command handler", 0)
        ida_xref.add_cref(jmp_ea, target_ea, ida_xref.fl_JN)
        if state == "undefined":
            resync_after(target_ea)
            print(f"[+] {letter}: resolved undefined target {target_ea:X}, "
                  f"queued for code analysis")
        else:
            print(f"[=] {letter}: target {target_ea:X} already {state}, "
                  f"added xref only")

    ida_auto.auto_wait()
    print("\nDone. Re-open the .asm export / refresh and check whether "
          "the TLKXFF routine now has a proper proc + name -- if so it "
          "should turn up as one of the 26 targets above.")


if __name__ == "__main__":
    main()
