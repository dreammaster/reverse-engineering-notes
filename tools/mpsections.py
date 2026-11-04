#!/usr/bin/env python3
"""
Walk the "section" structure of LEVn.MP2 (and check ANDATA.DAT's block layout).  See docs/formats.md section 6.

MP2 = sections back to back, each:
    u32be data_len        bytes of record data
    u32be alloc_len       size of the expanded table in memory (= count * record_stride)
    u16be count
    count x (u16be size, u16be offset)   directory; offsets are contiguous from 0
    data_len bytes of records (variable length, big-endian words)

    python mpsections.py FILE.MP2...
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mjm


def _rec(d, pos, size):
    """A record that is not completely inside the file (damaged TATA/2 dump) reads as zeros."""
    r = d[pos:pos + size]
    return r if len(r) == size else bytes(size)


def mp2_sections(d):
    p, secs = 0, []
    while p < len(d):
        if p + 10 > len(d):
            raise ValueError('truncated section header at %#x' % p)
        dl, al, cnt = struct.unpack('>IIH', d[p:p + 10])
        data = p + 10 + 4 * cnt
        ents = struct.unpack('>%dH' % (2 * cnt), d[p + 10:data])
        tot = 0
        for i in range(cnt):
            if ents[2 * i + 1] != tot:
                raise ValueError('directory not contiguous at %#x entry %d' % (p, i))
            tot += ents[2 * i]
        if tot != dl:
            raise ValueError('section at %#x: data_len %d != directory total %d' % (p, dl, tot))
        # cadaver2/TATA/2/LEV2.MP2 in the retail dump is 74 bytes short: tolerate, flag it
        truncated = data + dl > len(d)
        secs.append(dict(pos=p, data_len=dl, alloc_len=al, count=cnt, stride=(al / cnt if cnt else 0),
                         truncated=truncated,
                         records=[_rec(d, data + ents[2 * i + 1], ents[2 * i]) for i in range(cnt)]))
        p = data + dl
    return secs


def andata_blocks(d):
    (n,) = struct.unpack('>H', d[:2])
    sizes = struct.unpack('>%dH' % n, d[2:2 + 2 * n])
    p = 2 + 2 * n
    out = []
    for s in sizes:
        out.append(d[p:p + s])
        p += s
    if p != len(d):
        raise ValueError('ANDATA blocks do not tile the file (%d != %d)' % (p, len(d)))
    return out


if __name__ == '__main__':
    for f in sys.argv[1:]:
        d = mjm.mjm_unpack(open(f, 'rb').read())
        try:
            if f.upper().endswith('.MP2'):
                print(f, ['%d x %g%s' % (s['count'], s['stride'], ' (TRUNCATED)' if s['truncated'] else '')
                          for s in mp2_sections(d)])
            else:
                print(f, [len(b) for b in andata_blocks(d)])
        except ValueError as e:
            print(f, 'ERROR', e)
