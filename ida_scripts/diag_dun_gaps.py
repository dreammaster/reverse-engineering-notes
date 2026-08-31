"""Read-only: list dun.idb seg000 functions with a lot of db-bytes in
their body, + the undefined runs between functions. Diagnostic for
fix_dun_coerce_gaps.py."""
import idautils
import ida_bytes
import ida_funcs
import ida_segment

s = ida_segment.get_segm_by_name("seg000")
S0, S0E = s.start_ea, s.end_ea


def body_stats(f):
    ins = dat = und = 0
    for a in range(f.start_ea, f.end_ea):
        fl = ida_bytes.get_full_flags(a)
        if ida_bytes.is_code(fl):
            ins += 1
        elif ida_bytes.is_unknown(fl):
            und += 1
        else:
            dat += 1
    return ins, dat, und


print("=== functions with >20% non-code body ===")
funcs = sorted(idautils.Functions(S0, S0E))
for ea in funcs:
    f = ida_funcs.get_func(ea)
    ins, dat, und = body_stats(f)
    tot = ins + dat + und
    if tot and (dat + und) * 100 // tot > 20:
        print(f"  {ea:#08x}-{f.end_ea:#08x}  {idautils.ida_name.get_name(ea):24} "
              f"insn={ins} data={dat} undef={und}")

print("\n=== undefined/data runs (>=6 bytes) not inside any function ===")
run_lo = None
for a in range(S0, S0E):
    fl = ida_bytes.get_full_flags(a)
    bad = (ida_bytes.is_unknown(fl)
           or (ida_bytes.is_data(fl) and not ida_bytes.is_strlit(fl)))
    inf = ida_funcs.get_func(a) is not None
    if bad and not inf and run_lo is None:
        run_lo = a
    elif (not bad or inf) and run_lo is not None:
        if a - run_lo >= 6:
            print(f"  {run_lo:#08x}-{a:#08x}  ({a - run_lo} bytes)")
        run_lo = None
if run_lo is not None and S0E - run_lo >= 6:
    print(f"  {run_lo:#08x}-{S0E:#08x}  ({S0E - run_lo} bytes)")
