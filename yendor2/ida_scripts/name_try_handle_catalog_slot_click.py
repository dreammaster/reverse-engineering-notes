"""
Names sub_17270, called from RunShopScreen and the main input loop
sub_1869D -- a small gate distinct from the already-documented
"mouse-click buy handler" sub_17032 (also called from both of the
same sites, at different offsets).

Calls HitTestCatalogSlot; returns immediately if no hit (ax==0) or if
the hit slot is empty ([si]==0). Otherwise dispatches to a large,
untraced helper (sub_219FA, 251 lines, bx=0) -- plausibly a
select/preview interaction on the clicked catalog slot, separate from
the actual purchase flow sub_17032 handles. Named narrowly around its
confirmed gate behavior; sub_219FA's own effect isn't traced this
round. -> TryHandleCatalogSlotClick

Run via:
    .\run_ida_script.ps1 name_try_handle_catalog_slot_click.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x17270
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryHandleCatalogSlotClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryHandleCatalogSlotClick': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Gate: HitTestCatalogSlot, bail if no hit or the slot is empty "
    "([si]==0). Otherwise dispatches to untraced sub_219FA (bx=0) -- "
    "distinct from the documented buy handler sub_17032. Called from "
    "RunShopScreen and sub_1869D.",
    False,
)
