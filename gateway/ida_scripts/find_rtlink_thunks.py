"""
Read-only survey: gatemain.idb's overlay manager is the commercial RTLink
(Polytron/Blinker-era DOS overlay linker) -- confirmed by reading several
of the highest-caller-count still-unnamed (sub_XXXXX) functions from
rank_unnamed_functions.py, all of which turned out to be identical
2-instruction trampolines:

    call near ptr rtlink_thunk
    jmp  <real target, in another overlay segment>

This is a linker-generated call stub, one per (caller-segment, callee)
pair -- not unique game logic -- so a large fraction of the "unnamed,
high caller count" functions flagged by rank_unnamed_functions.py are
actually this same boilerplate repeated many times, not independent
functions worth tracing one at a time.

This script finds every function matching that exact shape and reports
how many there are and what they jump to, to size up a batch-rename
pass (apply_rtlink_thunks_gatemain.py) before writing it.

    .\\run_ida_script.ps1 -Idb gatemain -ScriptName find_rtlink_thunks.py -NoExport
"""

import idc
import idautils
import ida_bytes
import ida_funcs

rtlink_thunk_ea = idc.get_name_ea_simple("rtlink_thunk")
print(f"rtlink_thunk = {rtlink_thunk_ea:#x}")

matches = []
non_matches_examined = 0
for ea in idautils.Functions():
    fname = idc.get_func_name(ea)
    if not fname.startswith("sub_"):
        continue
    end = idc.get_func_attr(ea, idc.FUNCATTR_END)
    size = end - ea
    if size > 12 or size < 5:
        continue
    # Expect: call near ptr rtlink_thunk (E8 xx xx), then a jmp (near E9 or far EA)
    b0 = ida_bytes.get_byte(ea)
    if b0 != 0xE8:
        continue
    call_target = ea + 3 + idc.get_wide_word(ea + 1)
    # get_wide_word returns unsigned; treat as signed 16-bit rel offset
    rel = ida_bytes.get_word(ea + 1)
    if rel >= 0x8000:
        rel -= 0x10000
    call_target = (ea + 3 + rel) & 0xFFFFF
    if call_target != rtlink_thunk_ea:
        non_matches_examined += 1
        continue
    jmp_ea = ea + 3
    insn_name = idc.print_insn_mnem(jmp_ea)
    targets = list(idautils.CodeRefsFrom(jmp_ea, 0))
    if not targets:
        non_matches_examined += 1
        continue
    op0 = targets[0]
    # Some far jmps land in a tail chunk IDA attributes back to this SAME
    # function (a relocated continuation, not a call to a different
    # function) -- not a real cross-function thunk, skip it.
    if idc.get_func_attr(op0, idc.FUNCATTR_START) == ea:
        non_matches_examined += 1
        continue
    target_fname = idc.get_func_name(op0) or idc.get_name(op0) or f"{op0:#x}"
    matches.append((ea, size, insn_name, target_fname))

print(f"\n{len(matches)} rtlink-thunk-shaped functions found "
      f"(size 5-12, starting with call rtlink_thunk)")
print(f"{non_matches_examined} other small (5-12 byte) sub_ functions did NOT match (different call target)")

from collections import Counter
target_counts = Counter(t[3] for t in matches)
print(f"\n{len(target_counts)} distinct jump targets among the thunks")
print("\nsample (first 20):")
for ea, size, mnem, target in matches[:20]:
    print(f"  {ea:#x} size={size} {mnem} -> {target}")

dupe_targets = {t: c for t, c in target_counts.items() if c > 1}
print(f"\n{len(dupe_targets)} targets have multiple thunks pointing at them (expected -- one per caller overlay segment)")
for t, c in sorted(dupe_targets.items(), key=lambda x: -x[1])[:10]:
    print(f"  {t}: {c} thunks")
