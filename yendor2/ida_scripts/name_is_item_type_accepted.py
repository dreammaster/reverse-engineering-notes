"""
Names sub_2D5E0, called from TryCureAilmentFromIconClick and
sub_26415 (a branch of the main input loop sub_1869D). Structurally
identical to the already-named IsItemEligibleForRepair (a separate
function instance at a different address, but the same logic): checks
the standing location's flags (word_2E546's [+0xC] bits 0xC000/0x800)
against the held item's flags (word_2E548's [+2] bits 0x100/0x40) --
a generic "does this location/context accept this item type" gate,
reused in at least two different UI contexts.

-> IsItemTypeAcceptedByLocation

Run via:
    .\run_ida_script.ps1 name_is_item_type_accepted.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D5E0
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "IsItemTypeAcceptedByLocation", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'IsItemTypeAcceptedByLocation': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Checks the standing location's flags against the held item's "
    "flags -- a generic 'does this context accept this item type' "
    "gate, structurally identical to IsItemEligibleForRepair (a "
    "separate function instance). Called from "
    "TryCureAilmentFromIconClick and sub_26415.",
    False,
)
