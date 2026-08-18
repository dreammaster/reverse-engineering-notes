"""
IDA Pro script: renames the two auto-named segments now that their
roles are clear, and fixes sg08e3's segment class (was UNK).

- sg01a2 (0x10000) -> CODE: the main segment, holds nearly all code
  plus embedded read-only data (TEXT_STRINGS, tile graphics, string
  literals) -- already correctly classed CODE, just auto-named.
- sg08e3 (0x17410) -> DATA: holds `player`/`_mapMonsters` and most
  runtime globals, but was never properly classified (SegClass UNK) --
  fixed to DATA here too.

Segments use a different IDA API than plain address renames
(idc.set_name), hence a dedicated script rather than folding into
apply_renames.py -- see that script's docstring for the scope split.

Does NOT touch seg002 (0x18AC0, also classed CODE) -- its content is
garbled, mis-disassembled garbage (nonsense FPU instructions, a jump
to a nonsensical far pointer), not real code. Flagged as a separate
follow-up in docs/roadmap.md, out of scope for this rename pass.

USAGE: DRY_RUN=False per this project's convention for direct,
low-risk renames (same risk profile as apply_renames.py).
"""

import ida_segment

DRY_RUN = False

# (start_ea, new_name, new_class_or_None)
SEGMENTS = [
    (0x10000, "CODE", None),
    (0x17410, "DATA", "DATA"),
]


def apply_segment_change(start_ea, new_name, new_class):
    seg = ida_segment.getseg(start_ea)
    if seg is None:
        print(f"{start_ea:X}: [!] no segment found at this address, skipping")
        return

    cur_name = ida_segment.get_segm_name(seg)
    cur_class = ida_segment.get_segm_class(seg)

    name_changed = cur_name != new_name
    class_changed = new_class is not None and cur_class != new_class

    if not name_changed and not class_changed:
        print(f"{start_ea:X}: already {cur_name!r} (class {cur_class!r}) -- skipping")
        return

    print(f"{start_ea:X}: {cur_name!r} -> {new_name!r}"
          + (f", class {cur_class!r} -> {new_class!r}" if class_changed else ""))
    if DRY_RUN:
        return

    if name_changed:
        if not ida_segment.set_segm_name(seg, new_name):
            print("    [!] set_segm_name FAILED")
    if class_changed:
        if not ida_segment.set_segm_class(seg, new_class):
            print("    [!] set_segm_class FAILED")


def main():
    for start_ea, new_name, new_class in SEGMENTS:
        apply_segment_change(start_ea, new_name, new_class)
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the new names took.")


if __name__ == "__main__":
    main()
