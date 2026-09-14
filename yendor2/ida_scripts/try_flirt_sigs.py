"""
Read-only-ish discovery script: tries a batch of plausible 16-bit DOS C
runtime FLIRT signature files (bundled with IDA) against yendor2.idb and
reports how many functions each one newly recognizes/names, to find out
which compiler runtime (if any) the shipped .sig files can identify in
this binary. Meant to be run via run_ida_script.ps1 with -NoExport so a
dud attempt doesn't get saved -- rerun without -NoExport once a
productive signature is identified, to keep the match.

    .\run_ida_script.ps1 try_flirt_sigs.py -NoExport
"""
import idaapi
import idautils
import idc
import ida_funcs

CANDIDATES = [
    "vac35wc",    # Watcom-flavored
    "bc31rtd",    # Borland C++ 3.1 runtime, DOS
    "bc31rtw",    # Borland C++ 3.1 runtime, Windows
    "bc15c2",     # Borland C++ 1.5
    "dm16dos",    # Digital Mars 16-bit DOS
    "mv16rdos",   # Microway NDP 16-bit DOS
    "sm16rdos",   # SuperMicroware/other 16-bit DOS
    "mq16rdos",   # MicroQuill 16-bit DOS
    "z116rdos",   # Zortech 1.x DOS
    "z316rdos",   # Zortech 3.x DOS
    "bh16rdos",
]


def named_count():
    total = 0
    named = 0
    lib = 0
    for ea in idautils.Functions():
        total += 1
        if not idc.get_func_name(ea).startswith("sub_"):
            named += 1
        flags = ida_funcs.get_func(ea).flags
        if flags & ida_funcs.FUNC_LIB:
            lib += 1
    return total, named, lib


before = named_count()
print(f"before: total={before[0]} named={before[1]} lib={before[2]}")

for sig in CANDIDATES:
    n = idc.plan_to_apply_idasgn(sig)
    print(f"plan_to_apply_idasgn({sig!r}) -> {n}")

import ida_auto
ida_auto.auto_wait()

after = named_count()
print(f"after: total={after[0]} named={after[1]} lib={after[2]}")
print(f"delta: named +{after[1]-before[1]}, lib +{after[2]-before[2]}")

if after[2] > before[2]:
    print("\nsample library-recognized functions:")
    count = 0
    for ea in idautils.Functions():
        flags = ida_funcs.get_func(ea).flags
        if flags & ida_funcs.FUNC_LIB:
            print(f"  {ea:#x}  {idc.get_func_name(ea)}")
            count += 1
            if count >= 40:
                break
