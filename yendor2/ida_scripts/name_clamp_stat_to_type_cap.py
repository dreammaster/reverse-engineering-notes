"""
Names sub_1B5DA, called twice from unnamed sub_1B4C2 -- a saturating
clamp helper: if ax exceeds a type-dependent cap, writes that cap
into [si] instead (otherwise leaves [si] untouched, i.e. only clamps
on overflow).

The cap is 0x270F (9999) when word_32940 equals 'R' (0x52) or 'T'
(0x54) -- plausibly single-character item-type/category markers, not
confirmed -- and 0x3E7 (999) otherwise, the same cap
ApplyMultiStatEffectForItem already uses for its own stat additions.
-> ClampValueAtSlotToTypeCap

Run via:
    .\run_ida_script.ps1 name_clamp_stat_to_type_cap.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1B5DA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ClampValueAtSlotToTypeCap", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ClampValueAtSlotToTypeCap': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "If ax exceeds a cap, writes the cap into [si]; else no-op. Cap "
    "is 0x270F (9999) when word_32940=='R'/'T' (item-type marker, "
    "not confirmed), else 0x3E7 (999) -- the same cap "
    "ApplyMultiStatEffectForItem uses. Called from sub_1B4C2.",
    False,
)
