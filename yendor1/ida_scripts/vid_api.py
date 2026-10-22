"""
INT 62h video-driver function table (VID16.DRV), shared by the IDA scripts.

AH selects the function. Parameters live in the driver's context block (the
segment returned by fn00 in AX, addressed as ES:xxxx by the caller); only a
handful take something in AL. See docs1/video-api.md for the field map.
Verified by reading VID16.DRV (2026-10-22).
"""

# ah -> (name, one-line summary)
VID_FUNCS = {
    0x00: ("vidInit", "install: alloc ctx + 200 tile slots, read ROM fonts, init mouse; AX=ctx seg (FFFF=fail)"),
    0x01: ("vidShutdown", "free driver memory, unhook INT 62h"),
    0x02: ("vidSetGraphicsMode", "INT 10h mode 10h (640x350x16)"),
    0x03: ("vidPutPixel", "pixel at (x1,y1) colour ctx+30h"),
    0x04: ("vidGetPixel", "colour of pixel (x1,y1) -> ctx+30h"),
    0x05: ("vidLine", "line (x1,y1)-(x2,y2) colour ctx+30h"),
    0x06: ("vidRect", "rectangle outline (x1,y1)-(x2,y2)"),
    0x07: ("vidFillRect", "filled rectangle (x1,y1)-(x2,y2)"),
    0x08: ("vidDrawChar", "ROM-font char ctx+2Ch at text cell (ctx+2Ah col, ctx+2Bh row)"),
    0x09: ("vidGetImageRaw", "capture screen rect to ctx+16h:18h, no header"),
    0x0A: ("vidPutImage", "draw planar image at ctx+16h:18h, size ctx+10h/12h/14h, at (x1,y1)"),
    0x0B: ("vidGetImage", "capture rect (x1,y1,x2,y2) to ctx+16h:18h with 5-byte header"),
    0x0C: ("vidPutImageHdr", "draw image from ctx+16h:18h stream (5-byte header) at (x1,y1)"),
    0x0D: ("vidCopyTileRow", "VRAM latch-copy AL*32px span (x1,y1,page ctx+31h)->(x3,y3,page ctx+32h)"),
    0x0E: ("vidScrollRight", "scroll the map view content right one 32px step"),
    0x0F: ("vidScrollLeft", "scroll the map view content left one 32px step"),
    0x10: ("vidScrollUp", "scroll the map view content up one 32px step"),
    0x11: ("vidScrollDown", "scroll the map view content down one 32px step"),
    0x12: ("vidShowPage", "AL = page to display (INT 10h/05h), cursor-safe, then delay ctx+8"),
    0x13: ("vidCopyRectKeyed", "copy rect (x1,y1,x2,y2)->(x3,y3), skipping colour ctx+34h"),
    0x14: ("vidCopyScreen0To1AndShow", "copy whole page 0 to page 1 and display page 1"),
    0x15: ("vidRevealCenterOut0To1", "wipe page 0 onto page 1 from the centre columns outward"),
    0x16: ("vidRevealRightToLeft0To1", "wipe page 0 onto page 1 right to left, delay ctx+0Ah"),
    0x17: ("vidRevealEdgesIn0To1", "wipe page 0 onto page 1 from both edges inward"),
    0x19: ("vidCopyRows0To1AndShow", "copy full scanlines y1..y2 of page 0 to page 1 at y3, show page 1"),
    0x1E: ("vidCopyScreen1To0AndShow", "copy whole page 1 to page 0 and display page 0"),
    0x1F: ("vidRevealCenterOut1To0", "wipe page 1 onto page 0 from the centre columns outward"),
    0x20: ("vidRevealRightToLeft1To0", "wipe page 1 onto page 0 right to left, delay ctx+0Ah"),
    0x21: ("vidRevealEdgesIn1To0", "wipe page 1 onto page 0 from both edges inward"),
    0x23: ("vidCopyRows1To0AndShow", "copy full scanlines y1..y2 of page 1 to page 0 at y3, show page 0"),
    0x28: ("vidMouseShow", "cursor show (nesting counter ctx+955h, starts at -1 = hidden)"),
    0x29: ("vidMouseHide", "cursor hide (nesting counter)"),
    0x2A: ("vidMouseSetBounds", "mouse clip rect = (x1,y1,x2,y2); clamps position"),
    0x2B: ("vidMouseRefresh", "redraw cursor if the mouse moved (ctx+953h bit 7)"),
    0x2C: ("vidMouseSetPos", "move mouse to (x1,y1), clamped to bounds"),
    0x2D: ("vidMouseSetCursorMode", "AL = cursor mode (ctx+958h): 0 arrow, 1 hotspot box, 2 directional"),
}
