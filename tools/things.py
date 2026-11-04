#!/usr/bin/env python3
"""
Dump a level's rooms and things by name (LEVn.MP2 + ENGn.LNG).  See docs/formats.md section 6.4.

    python things.py cadaver1/DATA/1            # level directory containing LEVn.MP2 and ENGn.LNG
    python things.py cadaver2/TATA/3 --things   # also list every thing record
"""
import glob, os, struct, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mjm, mpsections, lng


def load_level(d):
    mp2 = glob.glob(os.path.join(d, 'LEV*.MP2'))[0]
    eng = glob.glob(os.path.join(d, 'ENG*.LNG'))[0]
    secs = mpsections.mp2_sections(mjm.mjm_unpack(open(mp2, 'rb').read()))
    names = lng.load(eng)
    things = secs[3]['records']                      # 1000 records, index = thing id = name string id
    rooms = [struct.unpack('>%dH' % (len(r) // 2), r[:len(r) // 2 * 2]) for r in secs[2]['records']]
    return secs, names, things, rooms


if __name__ == '__main__':
    secs, names, things, rooms = load_level(sys.argv[1])
    nm = lambda i: names[i] if i < len(names) else '#%d' % i
    seen = {}
    for ri, r in enumerate(rooms):
        for w in r:
            seen.setdefault(w, []).append(ri)
    dup = {k: v for k, v in seen.items() if len(v) > 1}
    print('%d things with data, %d room lists (%d non-empty), %d thing ids listed more than once' % (
        sum(1 for t in things if len(t) >= 6), len(rooms), sum(1 for r in rooms if r), len(dup)))
    for ri, r in enumerate(rooms):
        if r:
            print('room %2d: %s' % (ri, ', '.join(nm(w) for w in r if w < len(names))))
    if '--things' in sys.argv:
        for i, t in enumerate(things):
            if len(t) >= 6:
                f = '' if len(t) < 11 else 'class=%3d flags=%04x +6=%4d slot=%4d room=%3d' % (
                    t[1], struct.unpack('>H', t[2:4])[0], struct.unpack('>H', t[6:8])[0],
                    struct.unpack('>H', t[8:10])[0], t[10])
                print('%4d %-24r %s | %s' % (i, nm(i)[:24], f, t.hex(' ')))
