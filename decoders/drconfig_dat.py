#!/usr/bin/env python3
"""Decode `DRCONFIG.DAT` -- Legacy of the Ancients' disk-layout manifest,
read + rewritten by `CONFIGUR.EXE` and consulted by the LEGLIB file
loader to know which floppy each game file lives on.

    python decoders/drconfig_dat.py [C:\\games\\lota\\DRCONFIG.DAT]

Format
------
    "DD2"                       3-byte magic ("Disk Definition v2")
    then N records, each:
        <filename ASCII>        e.g. "OUT.EXE"
        <1 byte>                disk code
        "\\r\\n"                 CRLF terminator

The shipped file has 84 records covering every game file.  Disk codes
seen: 0x00, 0x01, 0x02, 0x03 (the 4 game floppies, 1..4) and 0x0E
(= 0b1110 -- "present on disks 2/3/4", i.e. any of the play disks;
used for files the game may create or that ride multiple disks:
SAVER.EXE, DIS9.BSV, TCASOBJ.BSV).  CHAR.DAT = 0x00 (disk 1, the boot
disk).  `CONFIGUR.EXE` only edits the drive letters, not this file's
structure.
"""
import sys

MAGIC = b"DD2"


def records(d):
    assert d[:3] == MAGIC, d[:3]
    out = []
    i = 3
    while i < len(d):
        j = d.find(b"\r\n", i)
        if j < 0:
            break
        rec = d[i:j]
        name, code = rec[:-1].decode("latin1"), rec[-1]
        out.append((name, code))
        i = j + 2
    return out


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\DRCONFIG.DAT"
    d = open(path, "rb").read()
    recs = records(d)
    print(f"{path}  {len(d)} B   magic {d[:3]!r}   {len(recs)} records\n")
    by_disk = {}
    for name, code in recs:
        by_disk.setdefault(code, []).append(name)
        print(f"  {name:16} disk 0x{code:02X}")
    print("\nby disk code:")
    for code in sorted(by_disk):
        print(f"  0x{code:02X}  ({len(by_disk[code])}): "
              + ", ".join(by_disk[code]))


if __name__ == "__main__":
    main()
