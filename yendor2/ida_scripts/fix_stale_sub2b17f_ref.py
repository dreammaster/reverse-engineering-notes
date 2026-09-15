"""
Small follow-up: IsItemRangeAvailable's own comment still says
"sub_2B17F", now renamed CheckQuestItemsCompleted. Fixes the reference.

Run via:
    .\run_ida_script.ps1 fix_stale_sub2b17f_ref.py
"""
import idc
import ida_bytes

ea = 0x1CDBC
c = idc.get_cmt(ea, False)
print(f"old: {c!r}")
c2 = c.replace("sub_2B17F", "CheckQuestItemsCompleted")
ida_bytes.set_cmt(ea, c2, False)
print("fixed")
