"""
dump_interface_elements.py - reads the real InterfaceElement[10]/iface[10]
array straight out of GameSetupStructBase's own raw blob (no separate walk
needed -- iface[] sits inline within the main struct, already reachable via
dump_gamesetup_from_data.py's own GSB_SIZE-bounded read) and decodes all
`numiface` populated slots field by field, including each element's own
button[] sub-array.

Written to check whether the DATA behind this build's long-"dead end"
InterfaceElement struct (x/y/x2/y2/vtextalign unconfirmed for many rounds;
"this game most likely just doesn't use the old icon-bar interface system
at all" per an earlier round's exhaustive disassembly search) actually
carries a real, designed icon-bar UI -- see reversing/notes/
struct-layout-drift.md for the answer (it does: a title/status bar, a
9-button verb icon bar with real consecutive sprite numbers, an 8-button
window-border frame, and a bottom panel). This does NOT contradict the
earlier code-side finding -- the ENGINE was already shown, exhaustively, to
never read any of these fields -- it just shows the DATA was a real,
coherent design the AGS Editor produced and this build's compiled code
simply never wired up to anything, the same "data existing and code
reading it are different questions" nuance already found for `numiface`
itself.

Usage (no IDA needed, plain python3):
    python3 reversing/scripts/dump_interface_elements.py "C:\\games\\ags\\robblanc1_win\\rb.exe"
"""
import struct
import sys

from parse_clib_manifest import parse_clib

GSB_SIZE = 0xBF84
IFACE_BASE = 0x534       # GameSetupStructBase.iface[10], see apply_structs.py
IFACE_STRIDE = 0x334     # sizeof(InterfaceElement)
NUMIFACE_OFFSET = 0x253C
MAX_BUTTONS = 20         # MAXBUTTON, see apply_structs.py's InterfaceElement


def find_ac2game_dta(data):
    lib_version, entries = parse_clib(data)
    for name, off, length in entries:
        if name.lower() == "ac2game.dta":
            return off, length
    raise ValueError("ac2game.dta not found in this CLIB manifest")


def parse_ac2game_header(buf):
    pos = 0
    pos += 30  # teststr
    marker = struct.unpack_from("<i", buf, pos)[0]; pos += 4
    verlen = struct.unpack_from("<i", buf, pos)[0]; pos += 4
    pos += verlen
    return marker, pos


def decode_button(brec):
    x, y, pic, picdown, picover = struct.unpack_from("<5i", brec, 0)
    return {"x": x, "y": y, "pic": pic, "picdown": picdown, "picover": picover}


def decode_interface_element(rec):
    x, y, x2, y2 = struct.unpack_from("<4i", rec, 0x00)
    bgcol, fgcol, bordercol = struct.unpack_from("<3i", rec, 0x10)
    vtextxp, vtextyp, vtextalign = struct.unpack_from("<3i", rec, 0x1C)
    vtext = rec[0x28:0x28 + 40].split(b"\x00", 1)[0].decode("latin1")
    numbuttons = struct.unpack_from("<i", rec, 0x50)[0]
    buttons = [decode_button(rec[0x54 + b * 0x24: 0x54 + (b + 1) * 0x24])
               for b in range(max(0, min(numbuttons, MAX_BUTTONS)))]
    flags = struct.unpack_from("<i", rec, 0x324)[0]
    popup = rec[0x330]
    on = rec[0x331]
    return {
        "x": x, "y": y, "x2": x2, "y2": y2,
        "bgcol": bgcol, "fgcol": fgcol, "bordercol": bordercol,
        "vtextxp": vtextxp, "vtextyp": vtextyp, "vtextalign": vtextalign,
        "vtext": vtext, "numbuttons": numbuttons, "buttons": buttons,
        "flags": flags, "popup": popup, "on": on,
    }


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    with open(sys.argv[1], "rb") as f:
        data = f.read()

    off, length = find_ac2game_dta(data)
    buf = data[off:off + length]
    marker, hdr_len = parse_ac2game_header(buf)
    if marker != 12:
        print(f"WARNING: header marker is {marker}, not 12 -- calibrated to "
              "rb.exe's own header shape, expect garbage below")

    gsb = buf[hdr_len:hdr_len + GSB_SIZE]
    numiface = struct.unpack_from("<i", gsb, NUMIFACE_OFFSET)[0]
    print(f"numiface: {numiface}")

    for i in range(numiface):
        rec = gsb[IFACE_BASE + i * IFACE_STRIDE: IFACE_BASE + (i + 1) * IFACE_STRIDE]
        elem = decode_interface_element(rec)
        buttons = elem.pop("buttons")
        print(f"[{i}] {elem}")
        for b, btn in enumerate(buttons):
            print(f"    button[{b}] {btn}")


if __name__ == "__main__":
    main()
