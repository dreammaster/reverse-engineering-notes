"""
IDA Pro script: master list of symbol renames (functions + globals) for
OUT.EXE (ultima1_out.idb).

Single accumulating script instead of one standalone name_*.py file per
finding, mirroring the ultima2 project's apply_renames.py convention.
Whenever a function's or global's role becomes clear enough to name
confidently, add an entry to RENAMES below and re-run. Safe to re-run
repeatedly -- each entry is checked against the address's *current* name
and skipped if already applied.

Convention: DRY_RUN starts True here (unlike ultima2's apply_renames.py,
which Paul flipped to False on 2026-08-17 after the pattern proved
reliable there -- no equivalent decision made for this project yet).
Run once with DRY_RUN True, check the output, then flip and re-run.

For fuller justification of each rename, see the matching section of
docs/overview.md.

    .\\run_ida_script.ps1 -Idb ultima1_out -ScriptName apply_renames_out.py -NoExport
"""

import idc

DRY_RUN = False

# (ea, new_name, note)
RENAMES = [
    (0x1A81D, "getKeypressAndWaitRaw",
     "poll-loop identical to getKeypressAndWait (getKeypress + wait(1) or "
     "wait(3) depending whether a key was ready) but WITHOUT the _toupper "
     "call -- getKeypressAndWait is a separate, independent copy of this "
     "same loop, not a wrapper around this. All 15 call sites push "
     "_textColor (never any other value); some callers _toupper the "
     "result themselves afterward, others don't. See "
     "docs/overview.md#getkeypressandwaitraw-and-getkeypressandwait-duplicated-poll-loop."),

    (0x1A30C, "toLowerLetter",
     "hand-rolled tolower: if arg is 'A'-'Z' add 20h, else pass through "
     "unchanged. Called right after getKeypressAndWaitRaw in several "
     "menu-selection paths (drop/dropArmor/dropWeapon/etc.) to normalize "
     "a letter keypress before subtracting 'a' to get a menu index."),

    (0x1981D, "_nmalloc",
     "near-heap allocator: walks a singly-linked free-list "
     "(word_1EC56=sentinel head, word_1EC5A=cursor), splits a free block "
     "if it's big enough, else calls _nheapgrow to extend the segment via "
     "DOS INT 21h/4Ah (SETBLOCK) and links the new space in via _nfree "
     "before retrying. Underscore-prefixed to match this codebase's "
     "existing convention for runtime-library internals (_fopen, "
     "_toupper) -- this is the classic Microsoft C near-heap allocator, "
     "not game-specific code. See "
     "docs/overview.md#near-heap-allocator-_nmalloc-_nfree-_nheapgrow."),

    (0x198C0, "_nfree",
     "near-heap free: walks the same free-list as _nmalloc, coalesces "
     "the freed block with adjacent free blocks on either side. See "
     "_nmalloc entry above."),

    (0x19EA3, "_nheapgrow",
     "grows the near heap via DOS INT 21h/4Ah (ADJUST MEMORY BLOCK "
     "SIZE). Only called from _nmalloc. Note: this proc's disassembly "
     "contains two additional 'push bp; mov bp,sp; ...' prologues after "
     "the first RETN with NO incoming xrefs (dead/unreachable) -- almost "
     "certainly two sibling CRT entry points (e.g. a realloc/msize "
     "variant) from the same library object file that got linked in but "
     "never called by this program. Left un-split since they're inert; "
     "worth revisiting if a future finding calls into the middle of this "
     "proc."),

    (0x1B0B5, "readAmount",
     "up-to-4-digit numeric text entry at a screen position: isDigit-"
     "filters keypresses (via getKeypressAndWaitRaw), handles backspace "
     "(char 8) to delete the last digit, on Enter (or any non-digit/"
     "non-backspace key) converts the accumulated digit buffer to a "
     "number using word_1F95E (a {1,10,100,...} powers-of-ten table -- "
     "currently mis-typed in the IDB as one `dw 1` word followed by raw "
     "db bytes, not a proper array; see roadmap.md). Callers: dropPence, "
     "transactCastle, transactGrocer -- all 'how many/how much' prompts."),

    (0x1B094, "isDigit",
     "byte in ['0'..'9'] check, returns 1/0 in ax. Only caller is "
     "readAmount."),
]


def apply_rename(ea, new_name, note):
    cur = idc.get_name(ea)
    if cur == new_name:
        print(f"{ea:X}: already {new_name!r} -- skipping")
        return
    print(f"{ea:X}: {cur!r} -> {new_name!r}")
    print(f"    {note}")
    if DRY_RUN:
        return
    ok = idc.set_name(ea, new_name, idc.SN_NOWARN)
    if not ok:
        print("    [!] rename FAILED")


def main():
    for ea, new_name, note in RENAMES:
        apply_rename(ea, new_name, note)
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names "
              "took, then update docs/roadmap.md's checklist.")


if __name__ == "__main__":
    main()
