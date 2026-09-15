"""
Names sub_190E9, called from the main input loop sub_1869D and
sub_18F6C: given a slot pointer (si), bails if empty. Otherwise
resolves the record (SelectPartyRecordById), calls sub_266A9 (not
traced), then restores a portrait-sized region from EMS page 0x55D8
(the same page RestorePortraitPanelFromEMS/RestoreDialogAreaFromEMS
use) at a position computed from word_328BC/word_328C0 -- a
per-slot-position variant of the portrait EMS-restore pattern.

-> RestorePortraitAreaAtPosition

Run via:
    .\run_ida_script.ps1 name_restore_portrait_at_position.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x190E9
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RestorePortraitAreaAtPosition", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RestorePortraitAreaAtPosition': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Resolves a party record (SelectPartyRecordById), calls sub_266A9 "
    "(not traced), then restores a portrait-sized EMS-cached region "
    "(page 0x55D8) at a position from word_328BC/word_328C0. Called "
    "from sub_1869D and sub_18F6C.",
    False,
)
