"""
dump_characters_from_data.py - walks load_game_file's own exact byte-
consumption sequence (as read from the disassembly) through `ac2game.dta`'s
real bytes, past the main `GameSetupStructBase` blob, the `WordsDictionary`,
an unidentified fixed-size skip block, the compiled global script
(`fread_script`'s own format), and the `ViewStruct272` array, to reach and
decode the real `CharacterInfo` array -- independent, real-data validation
of one of the most heavily-confirmed structs in this whole project.

This is the deepest use yet of the "read the real shipped game data"
technique (see parse_clib_manifest.py / dump_gamesetup_from_data.py) --
everything before the character data has to be walked correctly (each
variable-length section's own size has to be read and skipped precisely) or
the character bytes will be read from the wrong position entirely. Two
built-in self-checks make wrong alignment immediately obvious rather than
silently printing garbage: the compiled script's own signature must read as
literal `"SCOM"`, and the script block's own trailing sentinel must read as
exactly `0xBEEFCAFE` (`ENDFILESIG`, Common/CSRUN.CPP) -- if either fails,
something upstream is misaligned and the character data that follows cannot
be trusted.

Confirmed sequence, calibrated specifically to THIS build's disassembly
(not blindly copied from the 2011 reference source, which has since grown
several fields/features this build predates -- e.g. no `numSections`/
sections-reading block, since this build's own `ccScript` doesn't have
that field at all; see fread_script's own matches.json entry):
  1. ac2game.dta's own small header (see dump_gamesetup_from_data.py)
  2. the raw GameSetupStructBase blob (0xBF84 bytes, this project's own
     independently-confirmed total size)
  3. if GameSetupStructBase.dict != 0: WordsDictionary's own format
     (num_words, then num_words * (a length-prefixed word + a 2-byte
     wordnum) -- see read_dictionary's own matches.json entry)
  4. one `getw()`-prefixed forward skip (relative to current position) --
     role not identified, presumably a legacy/obsolete block this build's
     own load_game_file still walks past but never populates from
  5. the compiled global script, fread_script's own exact format (see that
     function's own disassembly-derived layout, replicated field for field
     below)
  6. the `ViewStruct272` array, `numviews * 0x8D4` bytes read as one flat
     block (see the `ViewStruct272` struct-layout-drift.md writeup for the
     0x8D4 stride's own derivation)
  7. a second `getw()`-prefixed forward skip, this time SCALED by 0x204
     bytes per unit -- role not identified either
  8. the real `CharacterInfo` array, `numcharacters * 0x140` bytes
     (`0x140`==`sizeof(CharacterInfo)`, this project's own independently-
     confirmed total size)

Only decodes CharacterInfo past this point -- extend past step 8 (dialog
topics, GUIs, sprite flags, etc.) if a future round needs to validate
something further into the file.

Usage (no IDA needed, plain python3):
    python3 reversing/scripts/dump_characters_from_data.py "C:\\games\\ags\\robblanc1_win\\rb.exe"
"""
import struct
import sys

from parse_clib_manifest import parse_clib

GSB_SIZE = 0xBF84


class Reader:
    def __init__(self, buf):
        self.buf = buf
        self.pos = 0

    def read(self, n):
        b = self.buf[self.pos:self.pos + n]
        self.pos += n
        return b

    def i32(self):
        return struct.unpack('<i', self.read(4))[0]

    def cstr(self, maxlen=None):
        """NUL-terminated string, AGS freadstring()-style (CSRUN.CPP:2005)."""
        out = bytearray()
        while maxlen is None or len(out) < maxlen:
            c = self.read(1)
            if not c or c == b"\x00":
                break
            out += c
        return bytes(out)


def find_ac2game_dta(data):
    lib_version, entries = parse_clib(data)
    for name, off, length in entries:
        if name.lower() == "ac2game.dta":
            return off, length
    raise ValueError("ac2game.dta not found in this CLIB manifest")


def gsb_field(gsb, off, kind="i32"):
    if kind == "i32":
        return struct.unpack_from('<i', gsb, off)[0]
    raise ValueError(kind)


def read_fread_script(r):
    """Replicates fread_script's own exact byte-consumption sequence
    (Common/CSRUN.CPP:2029, this build's own no-numSections variant --
    see fread_script's matches.json entry). Returns a dict of the sizes
    read, for sanity-printing."""
    sig = r.read(4)
    if sig != b"SCOM":
        raise ValueError(f"expected 'SCOM' script signature, got {sig!r} "
                          "-- something upstream is misaligned")
    info = {"fileVer": r.i32()}
    info["globaldatasize"] = r.i32()
    info["codesize"] = r.i32()
    info["stringssize"] = r.i32()
    if info["globaldatasize"] > 0:
        r.read(info["globaldatasize"])
    if info["codesize"] > 0:
        r.read(info["codesize"] * 4)
    if info["stringssize"] > 0:
        r.read(info["stringssize"])
    info["numfixups"] = r.i32()
    if info["numfixups"] > 0:
        r.read(info["numfixups"])       # fixuptypes
        r.read(info["numfixups"] * 4)   # fixups
    info["numimports"] = r.i32()
    for _ in range(info["numimports"]):
        r.cstr()
    info["numexports"] = r.i32()
    for _ in range(info["numexports"]):
        r.cstr()
        r.i32()  # export_addr
    endsig = r.i32() & 0xFFFFFFFF
    if endsig != 0xBEEFCAFE:
        raise ValueError(f"expected ENDFILESIG 0xBEEFCAFE, got {endsig:#x} "
                          "-- something in the script block is misaligned")
    info["endsig_ok"] = True
    return info


def walk_to_characters(buf):
    r = Reader(buf)
    r.read(30)               # teststr signature
    marker = r.i32()
    if marker != 12:
        raise ValueError(f"header marker is {marker}, not 12 -- this parser "
                          "is calibrated to rb.exe's own header shape")
    verlen = r.i32()
    verstr = r.read(verlen)

    gsb = r.read(GSB_SIZE)

    dict_ptr = gsb_field(gsb, 0xA7F0)
    if dict_ptr != 0:
        num_words = r.i32()
        for _ in range(num_words):
            wordlen = r.i32()
            r.read(wordlen)  # encrypted word text, not decrypted here
            r.read(2)        # wordnum

    skip1 = r.i32()
    r.pos += skip1

    script_info = read_fread_script(r)

    numviews = gsb_field(gsb, 0x2540)
    r.read(numviews * 0x8D4)

    skip2_units = r.i32()
    r.pos += skip2_units * 0x204

    numcharacters = gsb_field(gsb, 0x2638)
    chardata = r.read(numcharacters * 0x140)

    return gsb, script_info, numcharacters, chardata


def decode_character(rec):
    def i32(off): return struct.unpack_from('<i', rec, off)[0]
    return {
        "defview": i32(0x00),
        "talkview": i32(0x04),
        "view": i32(0x08),
        "room": i32(0x0C),
        "x": i32(0x10),
        "y": i32(0x14),
        "name": rec[0x110:0x110 + 30].split(b"\x00", 1)[0].decode("latin1"),
        "scrname": rec[0x12E:0x12E + 16].split(b"\x00", 1)[0].decode("latin1"),
    }


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    with open(sys.argv[1], "rb") as f:
        data = f.read()

    off, length = find_ac2game_dta(data)
    buf = data[off:off + length]

    gsb, script_info, numcharacters, chardata = walk_to_characters(buf)
    print("fread_script sizes:", script_info)
    print(f"numcharacters: {numcharacters}")
    for i in range(numcharacters):
        rec = chardata[i * 0x140:(i + 1) * 0x140]
        print(f"  [{i}]", decode_character(rec))


if __name__ == "__main__":
    main()
