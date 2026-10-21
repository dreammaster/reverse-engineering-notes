"""
IDA Pro script: master list of symbol renames, comments and data labels for
KEH.EXE (fod/Fountain of Dreams KEH.idb).

Single accumulating script. Whenever a finding is confirmed, add an entry to
RENAMES / GLOBALS below and re-run. Idempotent: entries already applied are
skipped silently. Each entry carries a justification note; fuller write-ups
live in docs/overview.md.

    powershell -ExecutionPolicy Bypass -File run_ida_script.ps1 -Idb KEH -ScriptName apply_renames_keh.py

Data labels are given as dseg-relative offsets (the form used in the
disassembly operands), converted to EAs via DSEG_BASE.
"""

import idc
import ida_name

DSEG_BASE = 0x21240
DRY_RUN = False

# (code ea, new_name, note)
RENAMES = [
    # -- screen / dirty-rect layer. The screen is a 320x200 4bpp (160 bytes
    # per scanline) offscreen buffer (_picDecoded_0 segment) that is copied
    # to B800h through a list of dirty rectangles. --
    (0x1D9F5, "addDirtyRectFromBounds",
     "loads x/y/w/h from a Bounds struct and calls addDirtyRect"),
    (0x2077F, "addDirtyRect",
     "far; appends a 14-byte entry (screen dest, src, row count, words/row) "
     "to the dirty list at dseg:A251; AX=x, SI=y, DI=w, CX=h"),
    (0x208E2, "flushDirtyRects",
     "far; blitDirtyRects then reset the list head/count"),
    (0x208FC, "blitDirtyRects",
     "copies each queued rectangle from the offscreen buffer to video memory B800h"),
    (0x20772, "clearDirtyRects", "far; resets list head and count only"),
    (0x1DA17, "refreshBounds",
     "queue a Bounds rect and flush it to the screen immediately"),
    (0x1DC28, "refreshCurrentWindow",
     "refreshBounds on the current window's rect (field_1A override if set)"),
    (0x20720, "restoreTextMode",
     "far; INT 10h mode 3, used on exit and error paths"),

    # -- window / text layer --
    (0x1DDFD, "newLine",
     "carriage-return inside the current window; on overflow of the last "
     "line pages output (pauses for a key when byte_22A74 is set) or scrolls"),
    (0x1E18D, "scrollWindowUp",
     "scrolls the window contents up one text row then delays according to "
     "word_22C44 (text speed 1-8, adjusted live by the < > , . keys)"),
    (0x1DB58, "substituteCaret",
     "copies a string to unk_2323A, expanding '^' to the string at word_22C8A"),
    (0x1DBFD, "writeStringFlushRun",
     "word-wrap helper of writeString: emits chars from ptr1 up to ptr3"),
    (0x1DC0B, "writeStringRun",
     "writes CX characters from _writeStringPtr1 via writeChar"),
    (0x1DE81, "writeCharAtWindowPos",
     "writeChar at window-relative (x, y); arg0 = char"),
    (0x1E376, "writeIndexedString",
     "skips N NUL-terminated strings in the table at dseg:A834 and prints the Nth"),
    (0x1E2B8, "invertRect",
     "XORs the pixels of a window rect (4bpp) and optionally refreshes it"),
    (0x1E28A, "winClear", "zero-fills the window rectangle in the offscreen buffer"),

    # -- map view rendering --
    (0x1DC46, "buildMapView",
     "builds the 19x9 tile-word grid at unk_230E4 around the party position "
     "(mapPos - (9,4)) from the loaded map; cell flags 0x40/0x80 add overlay bits"),
    (0x1DD19, "drawMapViewTiles", "draws the 19x9 grid built by buildMapView via drawTile"),
    (0x1DD4C, "redrawMapView", "buildMapView + refresh the map viewport rect"),
    (0x1E098, "drawTile",
     "blits a 16x16 4bpp tile (128 bytes) with an optional masked overlay "
     "tile; AX = tile word (low 11 bits index, high bits overlay)"),
    (0x1E017, "blitTileRaw", "raw 16x16 blit of the current-window-relative cell"),
    (0x1E141, "writeBorderCell", "blits an 8x8 glyph (32 bytes) from fontData at textX/textY"),

    # -- misc runtime --
    (0x1DEE7, "rngByte",
     "16-byte additive-carry PRNG state at unk_22A64; returns next byte in AL"),
    (0x1DEB1, "randomRange",
     "random(min, max) inclusive, by rejection sampling rngByte"),
    (0x1DEDB, "rngByteSaveRegs", "rngByte wrapper that preserves SI/BX/CX/DX"),
    (0x1E08C, "getBiosTicks", "INT 1Ah AH=0, returns the low word of the tick count"),
    (0x1E4D0, "lzssDecode",
     "LZSS: 4096-byte ring buffer at dseg-relative 0xA8DD, start 0xFEE, "
     "12-bit offset / 4-bit length+3, flag bytes consumed LSB first"),
    (0x1E5BD, "lzssDecodeSegs",
     "args set the source/dest segments (word_2BB18/2BB1A) then lzssDecode"),
    (0x21130, "playSound",
     "far; starts PC-speaker sound effect N (<=6) from the table at dseg:-458C; "
     "waits for vertical retrace and steps sound script until finished"),
    (0x2119E, "soundTick",
     "interprets the speaker script one tick: 83h=note, 81h=repeat, 84h=stop"),
    (0x21090, "blitPic",
     "far; copies a width/height-prefixed picture (4-byte header) to the "
     "offscreen buffer at (x, y)"),
    (0x210DD, "blitPicFrame", "far; blitPic of frame N from a picture-set with offset table"),

    # -- game helpers --
    (0x1B452, "redrawGameScreen",
     "resets windows, redraws party list, map view and clock"),
    (0x1B29B, "showMessageWaitKey",
     "prints text in the dialog window (or window7), pages output, waits for key 0xFE"),
    (0x17E09, "markMapDirty", "sets bit 0x80 of the map header flags (dword_22C6E+5)"),
    (0x1073E, "isWithinRadiusOfParty",
     "(radius, x, y) -> true if the point is within a square radius of the party map position"),
    (0x12AB2, "setMapCellHighBits",
     "sets the top 5 bits of the map cell word at (x, y), preserving the low 11 bits"),
    (0x12B93, "checkPartyWipeAndQuit",
     "if no member has con > -15: revive them to -65 (0xFFBF) and run the end handling, then quit(arg)"),
    (0x16F6B, "isMemberActive", "true if member.con (+44h, minus level overflow >15) is positive"),
    (0x2045A, "removeMemberFromParty",
     "removes a member by party slot: clears its roster flag, shifts the party arrays, decrements partyCount"),
    (0x115CB, "drawListMenuPage",
     "draws one page (max 8 rows) of a scrolling list menu with up/down markers; "
     "rows are drawn through a callback"),
    (0x11682, "listMenuSelect",
     "runs a scrolling list menu: keys 1-8, PgUp/PgDn/cursor keys; returns the chosen index"),
]

# (dseg offset, new_name, note)
GLOBALS = [
    (0x1A2E, "_map", "far ptr to the loaded map (w,h @0/2; flags @5; cells at +0x414, 8 bytes each)"),
    (0x1A04, "_textSpeed", "scroll delay level 1-8"),
    (0x1834, "_pageOnOverflow", "nonzero: pause at end of window instead of scrolling"),
    (0x1A4A, "_caretSubstitution", "ptr to the string that replaces '^' in writeString"),
    (0x1824, "_rngState", "16-byte PRNG state used by rngByte"),
    (0xA7D0, "_dirtyRectNext", "next free entry in the dirty-rect list"),
    (0xA7CE, "_dirtyRectCount", "number of queued dirty rects"),
    (0xA7D2, "_dirtyRectCursor", "read cursor while blitting"),
]


def apply():
    for ea, name, note in RENAMES:
        cur = idc.get_name(ea)
        if cur == name:
            continue
        if DRY_RUN:
            print("[dry] %X %s -> %s" % (ea, cur, name))
            continue
        if not idc.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            print("FAILED rename %X %s -> %s" % (ea, cur, name))
        else:
            idc.set_func_cmt(ea, note, 1)
            print("renamed %X %s -> %s" % (ea, cur, name))
    for off, name, note in GLOBALS:
        ea = DSEG_BASE + off
        cur = idc.get_name(ea)
        if cur == name:
            continue
        if DRY_RUN:
            print("[dry] data %X %s -> %s" % (ea, cur, name))
            continue
        if not idc.set_name(ea, name, ida_name.SN_NOCHECK | ida_name.SN_NOWARN):
            print("FAILED data %X %s -> %s" % (ea, cur, name))
        else:
            idc.set_cmt(ea, note, 1)
            print("renamed data %X %s -> %s" % (ea, cur, name))


apply()
