"""
Standalone (non-IDA) decoder for Ultima II (DOS) `tlkx???` shop/NPC
text files. No dependency on IDA or the disassembly project -- useful
on its own for anyone poking at the game's data files.

Format (confirmed against real TLKXFF files, not just the EXE):

- On disk the file is 384 bytes, matching the ModdingWiki's figure.
  BUT the DOS port only ever reads and uses the first 256 bytes -- the
  disassembly proves this three ways (see docs/file-formats.md in the
  reverse-engineering-notes repo): the FCB read is a hardcoded
  `cx=0x100`, there's no second read to pick up the remaining 128
  bytes, and the destination buffer is only 256 bytes wide before the
  next buffer starts.
- Checked directly against real files: bytes [256:384] are not new
  content. They're a stale duplicate of a chunk of bytes [0:256]
  (offset varies per file), consistent with leftover scratch-buffer
  content captured whichever tool originally wrote these files rather
  than meaningful game data. Every file checked confirms this, so it's
  safe to always ignore the trailing 128 bytes.
- Byte 0 of the 256-byte logical file is a null header/marker byte
  (decodes to an empty first "string").
- The rest is a run of null (0x00)-terminated strings. `\r` (0x0D) is
  used as an in-string line break (3 rows x 30 cols on screen).
- "Encryption": every byte has its high bit set when stored
  (`stored = plaintext | 0x80`). The game's own decode -- there is no
  dedicated decrypt step, it's folded into the universal
  character-output routine -- is `plaintext = stored & 0x7F`. That's
  what this script does; it is NOT `stored - 128`, which gives the
  wrong answer for the raw 0x00 header/terminator bytes (they're
  already "decoded" on disk, not encoded).

Usage:
    python decode_talk_file.py TLKX03 [TLKX11 ...]
"""

import sys


def decode_talk_file(data: bytes) -> list[str]:
    logical = data[:256]
    decoded = bytes(b & 0x7F for b in logical)
    strings = decoded.split(b"\x00")
    # First entry is the empty string before the header marker; drop it.
    return [s.decode("ascii", "replace") for s in strings[1:] if s]


def main(argv: list[str]) -> int:
    if not argv:
        print(__doc__)
        return 1

    for path in argv:
        with open(path, "rb") as f:
            data = f.read()
        print(f"=== {path} ({len(data)} bytes on disk) ===")
        for i, text in enumerate(decode_talk_file(data)):
            print(f"  [{i}] {text!r}")
        print()

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
