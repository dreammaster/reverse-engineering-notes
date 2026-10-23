"""Make an IDA .asm export easier to read when porting it: comments and XREF noise are dropped, data labels become DS offsets,
ES: context fields get their names, driver calls collapse to one line, and shared routines carry the names of their C++ ports.

    python pretty_asm.py progN <data segment image offset in hex> > /tmp/progN.pretty.txt

The data segment image offset is the one the exe loads into DS (hex of the `mov ax, imm; mov ds, ax` operand times 16, e.g. PROG4: D600).
Labels such as word_23FA9 are named after their linear address in IDA; the DS offset is address - 10000h - image offset.
"""
import re
import sys

ES_FIELDS = {
    0x00: "tileSlots", 0x0E: "cpuClass", 0x16: "imgSeg", 0x18: "imgOff", 0x1A: "x1", 0x1C: "y1", 0x1E: "x2", 0x20: "y2", 0x22: "x3", 0x24: "y3",
    0x2A: "textCol", 0x2B: "textRow", 0x2C: "textChar", 0x2E: "textFg", 0x2F: "textBg", 0x30: "colour", 0x31: "drawPage", 0x32: "destPage",
    0x33: "videoKind", 0x34: "keyColour", 0x35: "font", 0x36: "iconBarState", 0x676: "paletteCmd", 0x678: "viewHalfW", 0x679: "viewHalfH",
    0x67E: "viewOffX", 0x680: "viewOffY", 0x912: "flags912", 0x914: "w914", 0x952: "mousePresent", 0x953: "mouseFlags", 0x955: "cursorHide",
    0x956: "cursorShape", 0x957: "cursorPage", 0x958: "cursorMode", 0x962: "curX", 0x964: "curY", 0x966: "mouseX", 0x968: "mouseY",
}
for i in range(16):
    ES_FIELDS[0x64 + i] = "cmap[%d]" % i

SHARED = {
    "shared_5c3cce": "flushKeys", "shared_019ed0": "drawBottomDecorations", "shared_23db76": "drawIconBar(al)", "shared_21818f": "input(ah,al,bl)",
    "shared_0bb58f": "drawFrame(al,ah,si)", "shared_23f116": "drawBox(bx)", "shared_9c11a6": "random(ax)", "shared_72afff": "formatNumber(ax,bx)",
    "shared_9a625d": "drawTopCorners", "shared_89fc75": "drawBottomCorners", "shared_67284e": "iconClick(al)", "shared_8f30c2": "money(al,bx)",
    "shared_d2992b": "loadParty", "shared_4a4289": "saveParty", "shared_c9c8cc": "pickLeader", "shared_e79de9": "strlen(bx)->ax",
    "shared_ed3965": "trimSpaces(bx)", "shared_ede455": "readText(bx,cx)", "shared_d809a1": "deriveStats", "shared_c7ef80": "drawTableString(al,bx)",
    "shared_1d0ebe": "infoPanel(al)", "shared_728316": "soundCue", "drawString": "drawString(bx)->bx", "drawStringLines": "drawStringLines(bx,cx)",
    "tileOp": "tileOp(al)", "delayTicks": "delayTicks(ax)", "pollInput": "pollInput", "fileReadRecordTo": "readRecordTo(ax,bx,di)",
    "fileWriteRecordFrom": "writeRecordFrom(ax,bx,si)", "sendPaletteCommand": "paletteCmd(al)", "waitInput": "waitInput",
}

VID = {}


def main():
    prog, base = sys.argv[1], int(sys.argv[2], 16)
    lines = open("%s.asm" % prog, encoding="latin-1").read().split("\n")
    out = []
    for line in lines:
        s = line.rstrip()
        if not s.strip() or s.lstrip().startswith(";"):
            continue
        m = re.search(r"; VID fn([0-9A-F]{2}) (\w+)", s)
        vidname = m.group(2) if m else None
        s = re.sub(r"\s{2,};.*$", "", s)             # trailing comments
        # data labels -> DS offsets
        def lab(mo):
            addr = int(mo.group(2), 16)
            off = addr - 0x10000 - base
            return "ds:%04X" % off if 0 <= off < 0x10000 else mo.group(0)
        s = re.sub(r"\b(byte|word|dword|unk|off|asc|stru)_([0-9A-F]{4,6})\b(?!:)", lab, s)
        s = re.sub(r"\bparm_([0-9A-F]{2})\b", lambda mo: "parm%s" % mo.group(1), s)
        # es: fields
        def esf(mo):
            off = int(mo.group(1), 16)
            return "es.%s" % ES_FIELDS.get(off, "%X" % off)
        s = re.sub(r"\bes:([0-9A-F]{1,4})h?\b", esf, s)
        for k, v in SHARED.items():
            s = re.sub(r"\b%s\b" % re.escape(k), v, s) if k.startswith("shared_") or k in ("drawString", "tileOp") else s
        if vidname and "int     62h" in s:
            out.append("                VID(%s)" % vidname)
            continue
        if re.match(r"\s+mov\s+ah, [0-9A-F]{1,2}h\s*$", s) or re.match(r"\s+mov\s+ax, 1[0-9A-F]00h\s*$", s):
            out.append(s)
            continue
        out.append(s)
    # drop the AH load right before VID(...)
    res = []
    for i, s in enumerate(out):
        if i + 1 < len(out) and out[i + 1].strip().startswith("VID(") and re.match(r"\s+mov\s+a[hx], ", s):
            res.append("  " + s.strip() + "   ; ->")
            continue
        res.append(s)
    sys.stdout.write("\n".join(res) + "\n")


main()
