#!/usr/bin/env python3
"""
UCSD p-System volume reader for the DOS Wizardry booter disks.

The DOS releases in the "Ultimate Wizardry Archives" (WIZ1.DSK .. WIZ5.DSK,
plus the SAVEn.DSK party disks) are not DOS/FAT floppies. Each is a linear
512-byte-block image of a UCSD p-System volume; WIZ1.COM is a ~1.8 KB real-mode
shim that hooks INT 13h to serve the image and boots it. The p-System's
x86 p-code interpreter is SYSTEM.INTERP and the game itself is p-code linked
into SYSTEM.PASCAL.

On-disk layout (all little-endian):

  block 0..1   bootstrap
  block 2..N   directory (N given by the volume header's DLASTBLK, usually 6,
               i.e. blocks 2..5 -> 2048 bytes -> 1 header entry + up to 77 files)
  block 6..    file data, each file occupying a contiguous block range

Directory entry = 26 bytes.

  Volume header (entry 0):
    +0  u16  DFIRSTBLK   always 0
    +2  u16  DLASTBLK    first block past the directory (blocks 2..DLASTBLK-1)
    +4  u16  DFKIND      0 for the volume entry
    +6  u8   name length (<=7)
    +7  7s   volume name
    +14 u16  DEOVBLK     nominal block count of the volume
    +16 u16  DNUMFILES   number of file entries that follow
    +18 u16  DLOADTIME
    +20 u16  DLASTBOOT
    +22 4s   reserved

  File entry:
    +0  u16  DFIRSTBLK   first block of the file
    +2  u16  DLASTBLK    first block past the file (length = DLASTBLK-DFIRSTBLK)
    +4  u16  DFKIND      low 4 bits = file kind (see KIND_NAMES)
    +6  u8   name length (<=15)
    +7  15s  file name
    +22 u16  DLASTBYTE   bytes used in the file's last block (1..512)
    +24 u16  DACCESS     modification date (UCSD packed date)

Usage:
    python tools/ucsd_disk.py list    <image.dsk>
    python tools/ucsd_disk.py extract <image.dsk> <NAME> [outfile]
    python tools/ucsd_disk.py extractall <image.dsk> <outdir>
    python tools/ucsd_disk.py block   <image.dsk> <blocknum> [count]
"""

import os
import sys
import struct

BLOCK = 512
DIR_START_BLOCK = 2
ENTRY_SIZE = 26

KIND_NAMES = {
    0: "vol/none",
    1: "badblocks",
    2: "code",
    3: "text",
    4: "info",
    5: "data",
    6: "graf",
    7: "foto",
    8: "securedir",
}


class DirEntry:
    __slots__ = ("first", "last", "kind", "name", "last_byte", "access", "index")

    def __init__(self, first, last, kind, name, last_byte, access, index):
        self.first = first
        self.last = last
        self.kind = kind
        self.name = name
        self.last_byte = last_byte
        self.access = access
        self.index = index

    @property
    def nblocks(self):
        return self.last - self.first

    @property
    def size(self):
        if self.nblocks <= 0:
            return 0
        return (self.nblocks - 1) * BLOCK + self.last_byte

    @property
    def kind_name(self):
        return KIND_NAMES.get(self.kind & 0x0F, f"?{self.kind:#06x}")


class Volume:
    def __init__(self, path):
        self.path = path
        with open(path, "rb") as fh:
            self.data = fh.read()
        self.total_blocks = len(self.data) // BLOCK
        self._parse_dir()

    def read_block(self, n, count=1):
        off = n * BLOCK
        return self.data[off:off + count * BLOCK]

    def _entry_bytes(self, i):
        # directory is a flat array of 26-byte entries starting at block 2
        off = DIR_START_BLOCK * BLOCK + i * ENTRY_SIZE
        return self.data[off:off + ENTRY_SIZE]

    def _parse_dir(self):
        hdr = self._entry_bytes(0)
        self.dir_first, self.dir_last, self.vol_kind = struct.unpack_from("<HHH", hdr, 0)
        nlen = hdr[6]
        self.vol_name = hdr[7:7 + nlen].decode("ascii", "replace")
        (self.eov_blocks, self.num_files,
         self.load_time, self.last_boot) = struct.unpack_from("<HHHH", hdr, 14)

        self.entries = []
        for i in range(1, self.num_files + 1):
            raw = self._entry_bytes(i)
            if len(raw) < ENTRY_SIZE:
                break
            first, last, kind = struct.unpack_from("<HHH", raw, 0)
            nlen = raw[6]
            if nlen == 0 or nlen > 15:
                # defensive: stop on an obviously bogus entry
                continue
            name = raw[7:7 + nlen].decode("ascii", "replace")
            last_byte, access = struct.unpack_from("<HH", raw, 22)
            self.entries.append(DirEntry(first, last, kind, name, last_byte, access, i))

    def find(self, name):
        up = name.upper()
        for e in self.entries:
            if e.name.upper() == up:
                return e
        return None

    def file_bytes(self, entry):
        raw = self.read_block(entry.first, entry.nblocks)
        return raw[:entry.size]


def cmd_list(args):
    vol = Volume(args[0])
    print(f"image        : {vol.path}  ({len(vol.data)} bytes, {vol.total_blocks} blocks)")
    print(f"volume       : {vol.vol_name!r}")
    print(f"directory    : blocks {vol.dir_first + DIR_START_BLOCK}..{vol.dir_last - 1}")
    print(f"eov blocks   : {vol.eov_blocks}   files: {vol.num_files}   last boot: {vol.last_boot:#06x}")
    if vol.eov_blocks > vol.total_blocks:
        print(f"  !! volume header claims {vol.eov_blocks} blocks but image only has "
              f"{vol.total_blocks} -- image may be truncated")
    print()
    print(f"  {'#':>2}  {'name':<16} {'kind':<10} {'first':>6} {'last':>6} {'blocks':>6} {'bytes':>8}")
    for e in vol.entries:
        flag = "" if e.last <= vol.total_blocks else "  <-- past end of image"
        print(f"  {e.index:>2}  {e.name:<16} {e.kind_name:<10} {e.first:>6} {e.last:>6} "
              f"{e.nblocks:>6} {e.size:>8}{flag}")


def cmd_extract(args):
    vol = Volume(args[0])
    name = args[1]
    e = vol.find(name)
    if not e:
        print(f"not found: {name}", file=sys.stderr)
        sys.exit(1)
    out = args[2] if len(args) > 2 else e.name
    with open(out, "wb") as fh:
        fh.write(vol.file_bytes(e))
    print(f"wrote {out}  ({e.size} bytes, kind={e.kind_name}, blocks {e.first}..{e.last - 1})")


def cmd_extractall(args):
    vol = Volume(args[0])
    outdir = args[1]
    os.makedirs(outdir, exist_ok=True)
    for e in vol.entries:
        safe = e.name.replace("/", "_")
        path = os.path.join(outdir, safe)
        with open(path, "wb") as fh:
            fh.write(vol.file_bytes(e))
        print(f"  {safe:<16} {e.size:>8}  {e.kind_name}")
    print(f"extracted {len(vol.entries)} files to {outdir}/")


def cmd_block(args):
    vol = Volume(args[0])
    n = int(args[1], 0)
    count = int(args[2], 0) if len(args) > 2 else 1
    data = vol.read_block(n, count)
    _hexdump(data, base=n * BLOCK)


def _hexdump(data, base=0):
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        hexs = " ".join(f"{b:02x}" for b in chunk)
        text = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        print(f"{base + i:08x}  {hexs:<47}  {text}")


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        sys.exit(2)
    cmd, rest = argv[0], argv[1:]
    handlers = {
        "list": cmd_list,
        "extract": cmd_extract,
        "extractall": cmd_extractall,
        "block": cmd_block,
    }
    if cmd not in handlers:
        print(f"unknown command: {cmd}", file=sys.stderr)
        print(__doc__)
        sys.exit(2)
    handlers[cmd](rest)


if __name__ == "__main__":
    main(sys.argv[1:])
