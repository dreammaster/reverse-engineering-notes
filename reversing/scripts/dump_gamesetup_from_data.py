"""
dump_gamesetup_from_data.py - reads the real, live `GameSetupStructBase`
blob straight out of an actual Rob Blanc 1 install's `ac2game.dta` (located
via parse_clib_manifest.py's own CLIB-parsing logic) and prints a handful of
already-confirmed fields' ACTUAL VALUES for this specific game.

A companion to parse_clib_manifest.py, aimed one level deeper: that script
locates packaged files inside the CLIB archive; this one additionally
parses `ac2game.dta`'s OWN small file-level header (a plaintext 30-byte
signature, a 4-byte marker this build's own `load_game_file` checks against
a literal `0x0C`, and a length-prefixed "engine needs" version string) to
find exactly where the raw `GameSetupStructBase` bytes begin, then reads a
handpicked set of already-confirmed field offsets from apply_structs.py's
own GameSetupStructBase declaration.

Purpose: independent, real-data VALIDATION of this project's own struct
mapping (not a disassembly technique) -- every value printed should read as
a small, contextually sane number/string if the confirmed offsets are
right. Used to validate GameSetupStructBase end to end (gamename=="Rob
Blanc I" verbatim; numfonts==3, matching the CLIB manifest's own 3 packaged
.wfn files exactly -- see reversing/notes/struct-layout-drift.md) and to
notice numiface==4 (the compiled game DATA carries 4 populated
InterfaceElement slots, even though the ENGINE CODE was separately found,
via exhaustive disassembly search, to never read any of those elements' own
remaining fields).

Only reads a hardcoded, hand-picked field list below -- extend FIELDS as
new offsets get confirmed and are worth spot-checking against real data.
Read-only; does not modify anything.

Usage (no IDA needed, plain python3):
    python3 reversing/scripts/dump_gamesetup_from_data.py "C:\\games\\ags\\robblanc1_win\\rb.exe"
    python3 reversing/scripts/dump_gamesetup_from_data.py "C:\\games\\ags\\robblanc1\\ac2game.dat"
"""
import struct
import sys

from parse_clib_manifest import parse_clib

GAMESETUP_SIZE = 0xBF84  # this project's own independently-confirmed total
                          # (the fread/fwrite sizeof() constant used
                          # throughout load_ac2game_dta/SaveGameSlot/
                          # restore_game_data -- see apply_structs.py)

# (name, offset, reader) -- reader is one of 'str30'/'i32'/'u16'/'bytes'
FIELDS = [
    ("gamename",      0x0000, "str30"),
    ("options[20]",   0x001E, "bytes20"),
    ("numiface",      0x253C, "i32"),
    ("numviews",      0x2540, "i32"),
    ("numcharacters", 0x2638, "i32"),
    ("numinvitems",   0x8538, "u16"),
    ("numdialog",     0x9FCC, "i32"),
    ("numfonts",      0x9FD4, "i32"),
    ("color_depth",   0x9FD8, "i32"),
    ("uniqueid",      0x9FE8, "i32"),
    ("langcodes",     0x9FF6, "bytes15"),
    ("numgui",        0xA7EC, "i32"),
]


def find_ac2game_dta(data):
    lib_version, entries = parse_clib(data)
    for name, off, length in entries:
        if name.lower() == "ac2game.dta":
            return off, length
    raise ValueError("ac2game.dta not found in this CLIB manifest")


def parse_ac2game_header(buf):
    pos = 0
    teststr = buf[pos:pos + 30]; pos += 30
    marker = struct.unpack_from("<i", buf, pos)[0]; pos += 4
    verlen = struct.unpack_from("<i", buf, pos)[0]; pos += 4
    verstr = buf[pos:pos + verlen]; pos += verlen
    return {"teststr": teststr, "marker": marker, "verstr": verstr}, pos


def read_field(gsb, offset, kind):
    if kind == "str30":
        return gsb[offset:offset + 30].split(b"\x00", 1)[0].decode("latin1")
    if kind.startswith("bytes"):
        n = int(kind[len("bytes"):])
        return list(gsb[offset:offset + n])
    if kind == "i32":
        return struct.unpack_from("<i", gsb, offset)[0]
    if kind == "u16":
        return struct.unpack_from("<H", gsb, offset)[0]
    raise ValueError(f"unknown reader kind {kind!r}")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    path = sys.argv[1]
    with open(path, "rb") as f:
        data = f.read()

    off, length = find_ac2game_dta(data)
    buf = data[off:off + length]
    header, hdr_len = parse_ac2game_header(buf)
    print("ac2game.dta header:", header)

    if header["marker"] != 12:
        print(f"WARNING: header marker is {header['marker']}, not the 12 "
              "this build's OWN load_game_file checks for -- this parser's "
              "header-shape assumptions are calibrated to rb.exe (the "
              "Windows release, this project's actual disassembly target) "
              "specifically. The DOS release's ac2game.dat is loaded by a "
              "DIFFERENT engine binary (ac.exe) with a different header "
              "shape not implemented here -- expect garbage below.")

    gsb = buf[hdr_len:hdr_len + GAMESETUP_SIZE]
    if len(gsb) != GAMESETUP_SIZE:
        print(f"WARNING: only {len(gsb)} bytes available, expected "
              f"{GAMESETUP_SIZE} -- offsets past this point are untrustworthy")

    print("GameSetupStructBase fields:")
    for name, offset, kind in FIELDS:
        try:
            value = read_field(gsb, offset, kind)
        except Exception as e:
            value = f"<error: {e}>"
        print(f"  {name:16s} @ +0x{offset:04X} = {value}")


if __name__ == "__main__":
    main()
