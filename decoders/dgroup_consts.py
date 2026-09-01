#!/usr/bin/env python3
"""
dgroup_consts.py -- dump the compiled-BASIC constant pool from a LotA module EXE.

Legacy of the Ancients was built with the Microsoft BASIC Compiler 6.0.  Every
module (.EXE) shares the LEGLIB.EXE runtime and a common DGROUP layout: the low
~0x1AC0 bytes are LEGLIB scratch (value stack at ds:111C, the arithmetic
op-dispatch table at ds:0F7C, ...), and the module's own SINGLE / INTEGER / STRING
constants and variables follow.

IDA's `.asm` export renders the whole DGROUP as `db 0`, so the constant *values*
never made it into `../out.asm` & friends -- but they ARE in the packed-then-
unpacked EXE image.  This reads them straight out.

The value-stack arithmetic (`rtm_FF44` etc., dispatched through `[ds:0F7C + n]`)
operates on SINGLEs, so `mov bx, <off> ; call rtm_FF4E` means "divide TOS by the
4-byte float at DGROUP:<off>".  Point this script at those offsets.

    python dgroup_consts.py OUT.EXE                 # auto-locate DGROUP, dump the
                                                    # combat pool
    python dgroup_consts.py OUT.EXE 2476 2482 290A  # specific ds: offsets
    python dgroup_consts.py OUT.EXE --anchor "ENEMY HIT BY BLOW OF " 2E70

DGROUP base is found from a known string literal + its known ds: offset (the
`--anchor TEXT OFF` pair); without one we fall back to the value verified for
OUT.EXE.  Standard library only.
"""
import struct
import sys

# MZ header parse -----------------------------------------------------------

def load_image(path):
    d = open(path, "rb").read()
    (sig, lastpg, pages, nreloc, hdrpar, minal, maxal,
     ss, sp, csum, ip, cs, reloff, ovl) = struct.unpack("<14H", d[:28])
    assert sig == 0x5A4D, "not an MZ EXE"
    hdr = hdrpar * 16
    # relocations, so we can tell a relocated (runtime-patched) word from a real one
    relocs = set()
    for i in range(nreloc):
        o, s = struct.unpack("<HH", d[reloff + 4 * i: reloff + 4 * i + 4])
        relocs.add((s, o))
    # IDA loads the file image at linear 0x10000; file offset hdr <-> linear 0x10000
    return d, hdr, relocs


# DGROUP base -------------------------------------------------------------------

# verified for C:\games\lota\OUT.EXE (MD5 41e67d1e...): the string
# "ENEMY HIT BY BLOW OF " sits at ds:2E74, giving DGROUP:0 at file 0x8C80.
_KNOWN = {
    "ENEMY HIT BY BLOW OF ": 0x2E74,
}

def find_dgroup_base(d, hdr, anchor_text, anchor_off):
    """Return the file offset of DGROUP:0000."""
    if anchor_text is None:
        anchor_text, anchor_off = next(iter(_KNOWN.items()))
    needle = anchor_text.encode("latin1")
    i = d.find(needle)
    if i < 0:
        raise SystemExit(f"anchor string {anchor_text!r} not found in image")
    if d.find(needle, i + 1) >= 0:
        print(f"  (note: {anchor_text!r} appears more than once; using first)")
    return i - anchor_off


# readers ---------------------------------------------------------------------

def dump(path, offsets, anchor=None):
    d, hdr, relocs = load_image(path)
    at, ao = (anchor if anchor else (None, None))
    base = find_dgroup_base(d, hdr, at, ao)
    print(f"{path}: DGROUP:0000 at file 0x{base:X}\n")
    print(f"  {'ds:off':>8}  {'float':>13}  {'int16':>8}  {'int32':>12}   notes")
    print("  " + "-" * 72)
    for off in offsets:
        b4 = d[base + off: base + off + 4]
        f = struct.unpack("<f", b4)[0]
        i16 = struct.unpack("<h", b4[:2])[0]
        i32 = struct.unpack("<i", b4)[0]
        # string-descriptor guess: dw len ; dw off  with a sane, in-image target
        note = ""
        ln, so = struct.unpack("<HH", b4)
        if 0 < ln < 128 and 0 < so < 0x8000:
            txt = d[base + so: base + so + ln]
            if all(9 <= c < 127 for c in txt):
                note = f'str "{txt.decode("latin1")}"'
        if not note and abs(f) > 1e-9 and 1e-4 < abs(f) < 1e9:
            note = "single"
        print(f"  {off:>8X}  {f:>13.5g}  {i16:>8}  {i32:>12}   {note}")


COMBAT_POOL = [  # ds: offsets touched by resolvePlayerAttack / rollCreatureStats
    0x2476, 0x247A, 0x247E, 0x2482, 0x24E6, 0x24EA,
    0x279C, 0x280A, 0x280E, 0x2812,
    0x2906, 0x290A, 0x290E,
    0x2970, 0x2974,
    0x2C26, 0x2C2A, 0x2C2E, 0x2C32,
    0x2E6C,
]

if __name__ == "__main__":
    a = sys.argv[1:]
    if not a:
        raise SystemExit(__doc__)
    exe = a[0]
    anchor = None
    rest = a[1:]
    if rest and rest[0] == "--anchor":
        anchor = (rest[1], int(rest[2], 16))
        rest = rest[3:]
    offs = [int(x, 16) for x in rest] if rest else COMBAT_POOL
    dump(exe, offs, anchor)
