#!/usr/bin/env python3
"""
Decoder for Cadaver *.LNG text tables (ENGn/FREn/GERn.LNG, after the mjm container).

Layout of the unpacked file:
    u16be  count
    u16be  offsets[count]        relative to  string_area = 2 + 2*count + 2
    ...    2 bytes (not part of any string - purpose unknown)
    ...    string data

A string is a run of 6-bit codes packed 4-per-3-bytes, MSB first; code 0 ends it, codes
48..63 produce no output.  Code -> character (CAD.EXE text6_alphabet, 1F17:F853, 1-based):

    1..26 A-Z   27 ' '   28 '.'   29 ','   30..39 '0'-'9'   40/41 CR   42 '-'   43 '?'
    44 '"'      45 '<'   46 '='   47 '>'

'<' '=' '>' are German umlauts: Payoff (TATA) files use < = O-umlaut, = = A-umlaut, > = U-umlaut
(TR=GT, F>R, G<TTER); Cadaver 1's German files (DATA/*/GER*.LNG) use '<' for every umlaut
(T<R, G<TTER). The French texts have no accents at all.
The game keeps the table (converted to little endian) and the string area in its stack
segment (ss:9738 / ss:9B20) and decodes with CAD.EXE text6_decode (1000:75CA).

    python lng.py FILE.LNG            list all strings
    python lng.py FILE.LNG 17         one string
"""
import os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mjm

ALPHABET = '\0ABCDEFGHIJKLMNOPQRSTUVWXYZ .,0123456789\r\r-?"<=>'


def decode_string(data, pos):
    out = []
    while pos < len(data):
        b = data[pos:pos + 3].ljust(3, b'\0')
        for c in (b[0] >> 2, ((b[0] & 3) << 4) | (b[1] >> 4), ((b[1] & 15) << 2) | (b[2] >> 6), b[2] & 0x3F):
            if c == 0:
                return ''.join(out)
            if c < len(ALPHABET):
                out.append(ALPHABET[c])
        pos += 3
    return ''.join(out)


def parse(d):
    (count,) = struct.unpack('>H', d[:2])
    offs = struct.unpack('>%dH' % count, d[2:2 + 2 * count])
    base = 2 + 2 * count + 2
    return [decode_string(d, base + o) for o in offs]


def load(path):
    with open(path, 'rb') as f:
        return parse(mjm.mjm_unpack(f.read()))


if __name__ == '__main__':
    strings = load(sys.argv[1])
    if len(sys.argv) > 2:
        print(strings[int(sys.argv[2])])
    else:
        for i, s in enumerate(strings):
            print('%3d  %r' % (i, s))
