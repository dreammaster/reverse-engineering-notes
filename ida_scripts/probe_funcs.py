"""Read-only: print function boundaries and unowned gaps inside given spans. Run with -NoExport."""
import idc, idautils, ida_funcs
spans = [(0x11FA6, 0x12540), (0x136B8, 0x138D0), (0x14F1A, 0x15400), (0x15390, 0x154D6), (0x15EE0, 0x16340),
         (0x16780, 0x16960), (0x16F30, 0x16F70), (0x17140, 0x1738B), (0x17854, 0x17AAC)]
funcs = sorted((f, idc.get_func_attr(f, idc.FUNCATTR_END), idc.get_func_name(f)) for f in idautils.Functions())
for a, b in spans:
    print("== %X-%X" % (a, b))
    cur = a
    for s, e, n in funcs:
        if e <= a or s >= b:
            continue
        if s > cur:
            print("  GAP %X-%X (%d bytes)" % (cur, s, s - cur))
        print("  func %X-%X %s" % (s, e, n))
        cur = max(cur, e)
    if cur < b:
        print("  GAP %X-%X" % (cur, b))
