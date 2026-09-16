"""
Names sub_1CF50 and sub_1CFC8, high confidence: a fixed-depth-3 nested
container search chain confirmed by FindItemInsideContainer's own
pre-existing comment ("recurses into nested container items the same
way via sub_1CF50"). All three levels are structurally identical
(load a container's 8-slot contents via FileEntry_Read, scan for an
item id in [word_3293E, word_32940], and -- if a non-matching slot's
item catalog record has flag [+0xC] bit 0x2000 set -- recurse one level
deeper), just against different fixed scratch-buffer offsets per level:
FindItemInsideContainer (depth 1, 0xBC28/0xBC2A) -> sub_1CF50 (depth 2,
0xBC4A/0xBC4C) -> sub_1CFC8 (depth 3, 0xBC6C/0xBC6E, terminal -- no
further recursion, confirming the chain stops at 3 levels).

sub_1CF50 -> FindItemInsideContainerLevel2
sub_1CFC8 -> FindItemInsideContainerLevel3

Run via:
    .\run_ida_script.ps1 name_container_nesting_levels.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x1CF50, "FindItemInsideContainerLevel2"),
    (0x1CFC8, "FindItemInsideContainerLevel3"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1CF50,
    "Depth-2 step of FindItemInsideContainer's nested-container search: "
    "same shape (load contents, scan 8 slots, recurse via "
    "FindItemInsideContainerLevel3 on a flagged sub-container item), "
    "different fixed scratch-buffer offsets (0xBC4A/0xBC4C). Called "
    "only from FindItemInsideContainer.",
    False,
)
ida_bytes.set_cmt(
    0x1CFC8,
    "Depth-3, terminal step of FindItemInsideContainer's nested- "
    "container search: same shape as the depth-1/2 steps but does not "
    "recurse further, so the search chain stops at 3 levels deep. "
    "Called only from FindItemInsideContainerLevel2.",
    False,
)
