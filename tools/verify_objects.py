#!/usr/bin/env python3
"""Compare thing -> sprite -> box -> screen position (objplace.py) with the entries the original builds and projects
(pickles from oracle_rooms.py).   python3 tools/verify_objects.py 1:rooms_lv1.pkl ..."""
import pickle, struct, sys, os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import things, mjm, objsprites, objplace, roomview
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
for lv, pk in ((int(a.split(':')[0]), a.split(':')[1]) for a in sys.argv[1:]):
    out = pickle.load(open(pk, 'rb'))
    d = '%s/cadaver1/DATA/%d' % (root, lv)
    secs, names, th, rooms = things.load_level(d)
    mp1 = mjm.mjm_unpack(open('%s/LEV%d.MP1' % (d, lv), 'rb').read())
    B = objsprites.blocks(mp1)
    hdrs = {i: struct.unpack('>16H', b[:32]) for i, (s, b) in enumerate(B) if s > 1 and len(b) >= 32}
    raw = {i: b[:32] for i, (s, b) in enumerate(B) if s > 1 and len(b) >= 32}
    tot = ok = nomatch = badpos = nanim = 0
    shown = 0
    for n, o in out.items():
        r = secs[0]['records'][n]
        w, h = r[4], r[5]
        if not (3 <= w <= 10 and 3 <= h <= 10):
            continue
        gx, gy = roomview.floor_origin(w, h)
        ents = {}
        for i in range(len(o['ents']) // 50):
            e = o['ents'][50 * i:50 * i + 50]
            vo, y, sh = o['proj'][i]
            ents.setdefault(tuple(e[:6]), []).append(((vo % 40) * 8 + sh, vo // 40))
        for t in rooms[n]:
            rec = th[t] if t < len(th) else b''
            if len(rec) < 8 or rec[3] & 0x80:
                continue
            b = struct.unpack('>H', rec[6:8])[0]
            if b not in hdrs:
                continue
            hd = hdrs[b]
            pb = objsprites.parse_sprite_block(B[b][1])
            animated = bool(pb and pb[2])
            box = objplace.trimmed_box(rec[0], rec[1], rec[2], raw[b])
            tot += 1
            if box not in ents:
                nomatch += 1
                if nomatch <= 2:
                    print('  level %d room %d thing %d block %d: box %s not among %s' % (lv, n, t, b, box, sorted(ents)[:12]))
                continue
            if animated:
                nanim += 1
                continue
            pos = objplace.project(rec[0], rec[1], rec[2], hd, gy * 320 + gx)
            if pos in ents[box]:
                ok += 1
            else:
                badpos += 1
                if shown < 3:
                    shown += 1
                    print('  level %d room %d thing %d block %d: ours %s game %s box %s' % (lv, n, t, b, pos, ents[box], box))
    print('level %d: %d placed things, %d positions equal, %d box not found, %d position differs (animated blocks not compared: %d)' % (lv, tot, ok, nomatch, badpos, nanim))
