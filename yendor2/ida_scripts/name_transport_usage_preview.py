"""
Names sub_2909C, called from ShowItemUsagePreview: selects one of a
4-entry transport table (base 0x77C6, 0x1A stride -- the same
PEGASUS/GIANT EAGLE/MAGIC DRAGON table drawn by
ShowClueBookTransportDetail) via flag bits in es:[si+0x12], then draws
"NAME:" / "COST:" (formatted BCD price from the entry's +0xE field)
and a flight-time-restriction line -- dumped one variant as "CAN FLY
ANYTIME DAY OR NIGHT" (others, per repair-cost's message dump seen
earlier, read "CAN ONLY FLY BETWEEN 12AM AND 6PM" / "6PM AND 12AM").

The item-use preview for summoning/riding a transport/mount item.
-> ShowTransportUsagePreview

Run via:
    .\run_ida_script.ps1 name_transport_usage_preview.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2909C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowTransportUsagePreview", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowTransportUsagePreview': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Transport/mount item-use preview (called from "
    "ShowItemUsagePreview): selects an entry from the 4-slot transport "
    "table (0x77C6, stride 0x1A -- PEGASUS/GIANT EAGLE/MAGIC DRAGON, "
    "same table as ShowClueBookTransportDetail) via es:[si+0x12] flag "
    "bits, draws 'NAME:'/'COST:' plus a flight-time-restriction line "
    "('CAN FLY ANYTIME DAY OR NIGHT' or a time-window variant).",
    False,
)
