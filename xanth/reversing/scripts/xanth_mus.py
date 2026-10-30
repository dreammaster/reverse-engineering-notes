"""
Reader for Companions of Xanth .MUS music archives (from open_mus_file_mb in xanth.idb and verified on the data).

Directory: 12-byte slots from file offset 0 until the first data offset (= offset of record 0, 1441-byte gap included): {u32 offset; u16 size; u16 a; u16 b; u16 c}.
The directory has 120 slots; the first 24 are used (the rest are zero/0xFFFF filler), two per tune (index 2*t-2 and 2*t-1 for tune t; the engine picks one by
the sound-device bit of word_8912E: one MIDI arrangement per device class).  Each record points at a standard MIDI file (MThd/MTrk, format 1, 120 ticks) -
not XMIDI as in Shannara.  Even records: size == SMF length exactly and (a,b,c) are small numbers (1,3,4) whose meaning is not decoded; odd records: size ==
SMF length + 4 and a=b=c=0.

    python xanth_mus.py XANTH_01.MUS            # list + verify
"""
import struct
import sys


def records(d):
    first = struct.unpack_from("<I", d, 0)[0]
    out = []
    for i in range(first // 12):
        off, size, a, b, c = struct.unpack_from("<IHHHH", d, i * 12)
        out.append((off, size, a, b, c))
    return out


def smf_length(d, off):
    """length of the SMF at off, or None"""
    if d[off:off + 4] != b"MThd":
        return None
    hl = struct.unpack_from(">I", d, off + 4)[0]
    p = off + 8 + hl
    ntr = struct.unpack_from(">H", d, off + 10)[0]
    for _ in range(ntr):
        if d[p:p + 4] != b"MTrk":
            return None
        p += 8 + struct.unpack_from(">I", d, p + 4)[0]
    return p - off


if __name__ == "__main__":
    d = open(sys.argv[1], "rb").read()
    rs = records(d)
    ok = 0
    used = 0
    for i, (off, size, a, b, c) in enumerate(rs):
        if off == 0:
            continue
        used += 1
        ln = smf_length(d, off)
        good = ln is not None and ln + (4 if i & 1 else 0) == size
        ok += good
        print(i, hex(off), size, "smf", ln, "OK" if good else "??", (a, b, c))
    print(len(rs), "slots,", used, "used,", ok, "consistent")
