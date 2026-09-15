"""
Names sub_13EDF, called 3x from ShowClueBookTransportDetail (once per
mount: Pegasus, Giant Eagle, Magic Dragon). Draws one mount's detail
row: name (si, direct string pointer), "VALUE:" + BCD4 price
([si+0xE]), "USES:" + a formatted number ([si+0x16]), and "TIME:" +
a flight-window line -- "BETWEEN 7P.M. AND 7A.M." (or similar) when
[si+0x18] bit 1 is clear, else "ANYTIME" -- confirmed via message
dump, matching the flight-restriction text already found in
ShowTransportUsagePreview.

-> DrawTransportDetailRow

Run via:
    .\run_ida_script.ps1 name_draw_transport_detail_row.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x13EDF
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawTransportDetailRow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawTransportDetailRow': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws one mount's detail row: name, 'VALUE:' (BCD4 price, "
    "[+0xE]), 'USES:' (formatted number, [+0x16]), 'TIME:' (flight "
    "window -- 'BETWEEN...AND...' or 'ANYTIME' per [+0x18] bit 1). "
    "Called 3x from ShowClueBookTransportDetail.",
    False,
)
