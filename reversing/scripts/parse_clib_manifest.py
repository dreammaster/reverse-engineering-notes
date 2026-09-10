"""
parse_clib_manifest.py - reads the CLIB asset-library manifest (this build's
save/room/data packaging format, reverse-engineered from the disassembly and
cross-checked against Common/Clib32.cpp -- see csetlib/clibfindindex/
clibopenfile in reversing/analysis/matches.json) directly out of an actual
Rob Blanc 1 game install, WITHOUT needing IDA or the disassembly at all.

This is a different kind of evidence source than everything else in
reversing/scripts/: instead of reading the disassembly or the 2011 reference
source, it reads the *real, shipped game data* the disassembled binary
actually loads at runtime. Useful whenever a disassembly-only investigation
hits a "not established, no way to check without also having the game's own
data" wall -- e.g. this script was written to settle whether Rob Blanc 1's
own room files actually use the pre-v9 "obsolete v2.00 action editor" room
format load_main_block's version-gated fallback path reads, or whether that
path is dead code the compiled engine merely carries but this specific game
never triggers (see the `whataction`/`val1`/`val2`/`otcond`/`points` fields
in apply_structs.py's RoomStruct, and reversing/notes/struct-layout-drift.md
for the full writeup).

Handles BOTH CLIB manifest shapes this project has confirmed in Common/
Clib32.cpp's csetlib()/read_new_format_clib():
  - lib_version 6  (this game's DOS release, ac2game.dat): a short num_files,
    13-byte-stride filenames (password-XOR'd, modifier usually 0), 4-byte
    lengths, a 2-byte-per-file flags/ratio block to skip, then raw data
    starting immediately after -- offsets computed by walking cumulative
    lengths from a single ftell() anchor.
  - lib_version 10 (this game's Windows release, rb.exe self-contained EXE,
    signature appended to end of file per the `clibendfilesig` trailer):
    a 4-byte num_data_files, 20-byte-stride data_filenames, a 4-byte
    num_files, 25-byte-stride filenames, then parallel 4-byte offset/length
    arrays and a 1-byte-per-file file_datafile array -- offsets are stored
    directly but need `+= absoffs` (the appended-block's own start position,
    read from the trailer) added when file_datafile[i]==0 (data lives in
    this same file, not a separate chained datafile).
Versions 11/15/20/21 (read_new_new_format_clib/read_new_new_enc_format_clib)
are NOT implemented here -- this build's own csetlib only ever produces 6 or
10 (see csetlib's own matches.json entry: "if(lib_version!=6 &&
lib_version!=10) return -3;"), so there's never been a reason to.

Usage (no IDA needed, run with plain python3):
    python3 reversing/scripts/parse_clib_manifest.py "C:\\games\\ags\\robblanc1\\ac2game.dat"
    python3 reversing/scripts/parse_clib_manifest.py "C:\\games\\ags\\robblanc1_win\\rb.exe"
    python3 reversing/scripts/parse_clib_manifest.py <path> --versions   (also reads
        each *.crm's own room_file_header.version, a plain little-endian
        `short` at the start of the extracted bytes on any little-endian
        host -- the room file format's own `#ifdef ALLEGRO_BIG_ENDIAN`
        special-case reader in Common/acroom.h never applies here)

Prints the file manifest (name, absolute offset, length[, room version]) to
stdout. Does not extract/write any file contents -- read-only inspection.
"""
import struct
import sys

CLIB_END_SIG = b"CLIB\x01\x02\x03\x04SIGE"


def _read_cstr(buf, maxlen):
    return buf[:maxlen].split(b"\x00", 1)[0].decode("latin1")


def find_clib_start(data):
    """Returns the absolute file offset of the 'CLIB' signature block --
    either at the start of the file (lib packaged standalone, e.g. a plain
    ac2game.dat) or wherever the appended-to-EXE trailer says it is."""
    if data[0:4] == b"CLIB":
        return 0
    if data[-12:] == CLIB_END_SIG:
        absoffs = struct.unpack_from("<I", data, len(data) - 16)[0]
        if data[absoffs:absoffs + 4] == b"CLIB":
            return absoffs
        raise ValueError(
            f"trailer points at offset {absoffs} but no 'CLIB' signature "
            "found there"
        )
    raise ValueError("no 'CLIB' signature found (neither at offset 0 nor "
                      "via the appended-to-EXE trailer)")


def parse_clib(data):
    start = find_clib_start(data)
    pos = start
    sig5 = data[pos:pos + 5]
    assert sig5[:4] == b"CLIB", f"expected CLIB signature at {pos}"
    pos += 5
    lib_version = data[pos]
    pos += 1

    entries = []  # list of (name, abs_offset, length)

    if lib_version == 6:
        passwmodifier = data[pos]; pos += 1
        pos += 1  # unused byte
        num_files = struct.unpack_from("<h", data, pos)[0]; pos += 2
        pos += 13  # skip password dooberry
        names = []
        for _ in range(num_files):
            raw = data[pos:pos + 13]; pos += 13
            transformed = bytes((b - passwmodifier) & 0xFF for b in raw)
            names.append(_read_cstr(transformed, 13))
        lengths = list(struct.unpack_from("<%dI" % num_files, data, pos))
        pos += 4 * num_files
        pos += 2 * num_files  # flags & ratio, skipped
        offsets = [0] * num_files
        offsets[0] = pos
        for i in range(1, num_files):
            offsets[i] = offsets[i - 1] + lengths[i - 1]
        # this whole manifest lives inside `data` itself, relative to `start`
        # only when the lib was appended to a host file -- for a lib_version 6
        # manifest (this project has only ever seen this at file-start, e.g.
        # a plain standalone ac2game.dat) offsets are already absolute-in-file.
        entries = list(zip(names, offsets, lengths))

    elif lib_version == 10:
        chain_byte = data[pos]; pos += 1
        if chain_byte != 0:
            raise ValueError("lib_version 10 chain byte != 0 -- this is not "
                              "the first datafile in the chain; unsupported")
        num_data_files = struct.unpack_from("<i", data, pos)[0]; pos += 4
        pos += 20 * num_data_files  # data_filenames, not needed here
        num_files = struct.unpack_from("<i", data, pos)[0]; pos += 4
        names = []
        for _ in range(num_files):
            raw = data[pos:pos + 25]; pos += 25
            names.append(_read_cstr(raw, 25))
        offsets = list(struct.unpack_from("<%di" % num_files, data, pos))
        pos += 4 * num_files
        lengths = list(struct.unpack_from("<%di" % num_files, data, pos))
        pos += 4 * num_files
        file_datafile = list(data[pos:pos + num_files])
        pos += num_files
        for i in range(num_files):
            if file_datafile[i] == 0:
                offsets[i] += start
        entries = list(zip(names, offsets, lengths))

    else:
        raise ValueError(
            f"lib_version {lib_version} not handled by this script (this "
            "build's own csetlib only ever produces 6 or 10 -- see its "
            "matches.json entry)"
        )

    return lib_version, entries


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    path = sys.argv[1]
    want_versions = "--versions" in sys.argv[2:]

    with open(path, "rb") as f:
        data = f.read()

    lib_version, entries = parse_clib(data)
    print(f"CLIB lib_version={lib_version}, {len(entries)} files")
    for name, off, length in entries:
        line = f"  {name:20s} offset={off:<10d} length={length}"
        if want_versions and name.lower().endswith(".crm"):
            ver = struct.unpack_from("<h", data, off)[0]
            line += f"  room_file_version={ver}"
        print(line)


if __name__ == "__main__":
    main()
