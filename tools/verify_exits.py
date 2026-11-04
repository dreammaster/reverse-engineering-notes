#!/usr/bin/env python3
"""Compare roomexits.room_exits with the exit entries the original builds (pickles written by an oracle run, see docs)."""
import pickle, struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import things, mjm, objsprites, roomexits
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
for lv, pk in ((int(a.split(':')[0]), a.split(':')[1]) for a in sys.argv[1:]):
    out = pickle.load(open(pk, 'rb'))
    d = '%s/cadaver1/DATA/%d' % (root, lv)
    secs, names, th, rooms = things.load_level(d)
    mp1 = mjm.mjm_unpack(open('%s/LEV%d.MP1' % (d, lv), 'rb').read())
    B = objsprites.blocks(mp1)
    hd = lambda i: struct.unpack('>16H', B[i][1][:32])[8:11]
    bad = 0
    for n, o in out.items():
        r = secs[0]['records'][n]
        got = [tuple(e[:6]) + (e[0x16],) for e in o['exits']]
        exp = roomexits.room_exits(r, secs[1]['records'], hd, r[21], r[20])
        if got != exp or o['b'][:2] != [r[20], r[21]] or o['b'][4:] != [r[4] * 8, r[5] * 8]:
            bad += 1
            print('level', lv, 'room', n, got, exp)
    print('level %d: %d rooms, %d mismatches' % (lv, len(out), bad))
