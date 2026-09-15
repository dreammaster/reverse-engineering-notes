"""
Read-only: ranks unnamed functions by how many distinct named data/code
references they contain, as a proxy for "how easy will this be to
confidently identify". Now that fix_ds_segreg.py unlocked data-ref
resolution, this is much more useful than before.

    .\run_ida_script.ps1 rank_naming_candidates.py -NoExport
"""
import idautils
import idc
import ida_funcs

GENERIC_PREFIXES = ("byte_", "word_", "dword_", "unk_", "loc_", "sub_", "off_", "asc_")

def named_refs(f):
    refs = set()
    cur = f.start_ea
    while cur < f.end_ea:
        for x in list(idautils.DataRefsFrom(cur)) + list(idautils.CodeRefsFrom(cur, 0)):
            nm = idc.get_name(x)
            if nm and not nm.startswith(GENERIC_PREFIXES):
                refs.add(nm)
        cur = idc.next_head(cur, f.end_ea)
    return refs

results = []
for ea in idautils.Functions():
    if not idc.get_func_name(ea).startswith("sub_"):
        continue
    f = ida_funcs.get_func(ea)
    if f.end_ea - f.start_ea > 300:
        continue  # skip huge ones for this pass, harder to read quickly
    refs = named_refs(f)
    if refs:
        results.append((len(refs), ea, f.end_ea - f.start_ea, sorted(refs)))

results.sort(reverse=True)
print(f"{len(results)} unnamed functions (<=300 bytes) with named refs")
for count, ea, size, refs in results[:60]:
    print(f"  {ea:#x} (size={size}, {count} refs): {', '.join(refs[:8])}")
