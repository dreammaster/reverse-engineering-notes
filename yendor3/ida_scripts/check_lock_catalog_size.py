"""
Read-only: prints the initial values of word_2ECF8/word_3320E
(yendor3's equivalents of yendor2's _val10/_val9), which
LoadCurgameRecord uses to size/locate its own small record table
relative to the lock-definition catalog loaded by loadWorldDat2.

    .\run_ida_script.ps1 check_lock_catalog_size.py -NoExport
"""
import idc

for name in ["word_2ECF8", "word_3320E"]:
    ea = idc.get_name_ea_simple(name)
    val = idc.get_wide_word(ea)
    print(f"{name}: ea={ea:#x} value={val:#x} ({val})")
