#!/usr/bin/env python3
"""Generate src/tests/vectors.h from the Python reference decoders (tools/mjm.py, tools/pc1.py)."""
import glob, os, struct, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import push, panel, mjm, pc1, lng, pcman, mpsections, roomview, intro, objsprites, room_objects, animation

root = os.path.join(here, '..')


def fnv(b):
    h = 0x811C9DC5
    for c in b:
        h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF
    return h


rows = []
for g in ('cadaver1', 'cadaver2'):
    for f in sorted(glob.glob(os.path.join(root, g, '**', '*'), recursive=True)):
        rel = os.path.relpath(f, root)
        if not os.path.isfile(f) or ('/DATA/' not in rel and '/TATA/' not in rel):
            continue
        try:
            u = mjm.mjm_unpack(open(f, 'rb').read())
        except Exception:
            rows.append((rel, -1, 0))  # expected to fail (corrupt retail copy)
            continue
        rows.append((rel, len(u), fnv(u)))
pics = []
for g in ('cadaver1', 'cadaver2'):
    for n in ('LOGO.PC1', 'LOADING2.PC1'):
        p = os.path.join(root, g, n)
        if os.path.exists(p):
            pal, px = pc1.decode(open(p, 'rb').read())
            pics.append((os.path.relpath(p, root), fnv(px)))
lngs = []
for r in rows:
    if r[0].upper().endswith('.LNG') and r[1] >= 0:
        ss = lng.load(os.path.join(root, r[0]))
        blob = b''.join(x.encode('latin1') + b'\0' for x in ss)
        lngs.append((r[0], len(ss), fnv(blob)))
spr_vec = []
for g in ('cadaver1', 'cadaver2'):
    f = os.path.join(root, g, 'DATA', 'PCMAN.DAT')
    if os.path.exists(f):
        sp = pcman.load_sprites(mjm.mjm_unpack(open(f, 'rb').read()))
        blob = b''.join(struct.pack('<hhii', s['hot'][0], s['hot'][1], s['w'], s['h']) + s['pixels'] for s in sp)
        spr_vec.append((os.path.relpath(f, root), len(sp), fnv(blob)))
mp2_vec = []
for r in rows:
    if r[0].upper().endswith('.MP2') and r[1] >= 0:
        secs = mpsections.mp2_sections(mjm.mjm_unpack(open(os.path.join(root, r[0]), 'rb').read()))
        blob = b''
        for sc in secs:
            blob += struct.pack('<iII', sc['count'], sc['data_len'], sc['alloc_len'])
            for rec in sc['records']:
                blob += struct.pack('<i', len(rec)) + rec
        mp2_vec.append((r[0], fnv(blob)))
andata_b1 = mpsections.andata_blocks(mjm.mjm_unpack(open(os.path.join(root, 'cadaver1', 'DATA', 'ANDATA.DAT'), 'rb').read()))[1]
room_vec = []
obj_vec = []
for d in sorted(glob.glob(os.path.join(root, 'cadaver1', 'DATA', '[1-5]')) + glob.glob(os.path.join(root, 'cadaver2', 'TATA', '[1-4]')) +
                glob.glob(os.path.join(root, 'cadaver2', 'DATA', '[1-5]'))):
    mp1, mp2, mp3 = roomview.level_files(d)
    secs = mpsections.mp2_sections(mp2)
    tiles = roomview.load_tiles(mp3)
    fms = roomview.floor_maps(mp1)
    wrecs = roomview.wall_columns(mp1)
    wtiles = roomview.load_wall_tiles(mp3)
    blob = b''.join(tiles) + b''.join(roomview.load_wall_tiles(mp3)) + struct.pack('<16H', *roomview.level_palette(mp1))
    nrooms = 0
    for i, fm in enumerate(fms):
        w, h = roomview.room_size(secs, i)
        if w * h == len(fm) and len(fm):
            W, H, px = roomview.render_floor(tiles, fm, w, h)
            blob += struct.pack('<iii', i, W, H) + px
            if 3 <= w <= 10 and 3 <= h <= 10:
                shell = bytearray(roomview.render_shell(tiles, wtiles, fm, wrecs[i], w, h, room_objects.door_sprites(mp1)))
                blob += bytes(shell)
                gx, gy = roomview.floor_origin(w, h)
                room_objects.draw_objects(shell, secs, objsprites.blocks(mp1), i, gx, gy)
                blob += bytes(shell)
                wsh = bytearray(shell)
                panel.apply_wings(wsh, andata_b1)
                blob += bytes(wsh)
            nrooms += 1
    room_vec.append((os.path.relpath(d, root), len(tiles), nrooms, fnv(blob)))
    ob = b''
    nspr = 0
    for bi, (sz, blk) in enumerate(objsprites.blocks(mp1)):
        r = objsprites.parse_sprite_block(blk)
        if r:
            nspr += 1
            ob += struct.pack('<iii', bi, len(r[1]), len(r[2])) + r[0] + r[2]
            for (fw, fh, fpx) in r[1]:
                ob += struct.pack('<ii', fw, fh) + fpx
            if r[2]:
                sc = [(w >> 8, w & 255) for w in objsprites.script(r[2])]
                offs = objsprites.frame_offsets(r[2])
                ast = dict(delay=0, frame=0, dir=0, pos=0, cnt=1, cad=1, dx=0, dy=0)
                for _ in range(300):
                    animation.step(ast, sc, offs)
                    ob += struct.pack('<6i', ast['pos'], ast['frame'], ast['delay'], ast['dir'], ast['dx'], ast['dy'])
    obj_vec.append((os.path.relpath(d, root), nspr, fnv(ob)))
intro_vec = []
for g, key in (('cadaver1', 'cadaver'), ('cadaver2', 'payoff')):
    d = os.path.join(root, g)
    if os.path.exists(os.path.join(d, 'FLAME.DAT')):
        blob = b''
        for lf in range(6):
            pal, px = intro.render_title(d, key, lf)
            blob += px
        intro_vec.append((g, fnv(blob)))
# ---- collision test: random worlds, Python reference (tools/collision.py) vs src/engine/collision.c
class Rng:
    def __init__(self, s): self.s = s & 0xFFFFFFFF or 1
    def next(self):
        x = self.s
        x ^= (x << 13) & 0xFFFFFFFF; x ^= x >> 17; x ^= (x << 5) & 0xFFFFFFFF
        self.s = x
        return x
    def rnd(self, n): return self.next() % n


def collision_vector(seed):
    import collision
    r = Rng(seed)
    ds = bytearray(0x10000); ss = bytearray(0x10000)
    b = lambda v: v & 0xFF
    ds[0x5F4] = 8 * (3 + r.rnd(8)); ds[0x5F5] = 8 * (3 + r.rnd(8))
    ds[0x5D2] = r.rnd(8); ds[0x5D1] = r.rnd(8); ds[0x5DF] = r.rnd(20); ds[0x5E0] = r.rnd(20)
    ds[0x5F9] = r.rnd(2)
    nz = r.rnd(5)
    ds[0x54:0x56] = [0, 0x180D, 0x17DF, 0x17B1, 0x1782][nz].to_bytes(2, 'little')
    for j in range(4):
        x1 = r.rnd(90); y1 = r.rnd(90)
        ds[0x5E4 + 4 * j:0x5E8 + 4 * j] = bytes([b(x1), b(y1), b(x1 - r.rnd(12)), b(y1 - r.rnd(12))])
    nobj = r.rnd(25)
    ds[0x224] = nobj; ds[0x2A:0x2C] = (0x1000).to_bytes(2, 'little')
    objs = []
    for i in range(nobj):
        x1 = r.rnd(100); y1 = r.rnd(100); zb = r.rnd(30)
        box = [x1, y1, x1 - r.rnd(10), y1 - r.rnd(10), zb + r.rnd(30), zb]
        marker = 0xFF if r.rnd(2) else r.rnd(20)
        flag4 = 4 if r.rnd(4) == 0 else 0
        e = 0x1000 + 50 * i
        for k in range(6): ds[e + k] = b(box[k])
        rec = 0x3000 + 16 * i
        ds[e + 0x0A:e + 0x0C] = rec.to_bytes(2, 'little'); ds[e + 0x14] = marker; ds[rec + 15] = flag4
        objs.append((rec, marker, flag4, box))
    nex = r.rnd(4)
    ds[0x3A:0x3C] = (0x500).to_bytes(2, 'little'); ds[0x22E] = nex
    exits = []
    for i in range(nex):
        x1 = r.rnd(100); y1 = r.rnd(100); zb = r.rnd(20)
        box = [x1, y1, x1 - r.rnd(12), y1 - r.rnd(12), zb + r.rnd(40), zb]
        e = 0x500 + 50 * i
        for k in range(6): ss[e + k] = b(box[k])
        ss[e + 0x0A:e + 0x0C] = (0x4000 + i).to_bytes(2, 'little')
        exits.append(box)
    ss[0x500 + 50 * nex:0x500 + 50 * nex + 2] = b'\xff\xff'
    selfk = r.rnd(nobj + 1)
    si = 0x3000 + 16 * selfk if selfk < nobj else 0x2FF0
    h = 0x811C9DC5
    def mix(v):
        nonlocal h
        for sh in (0, 8, 16, 24):
            h = ((h ^ ((v >> sh) & 0xFF)) * 0x01000193) & 0xFFFFFFFF
    for q in range(60):
        x = b(r.rnd(110) - 5); y = b(r.rnd(110) - 5); z = r.rnd(40); xs = 1 + r.rnd(9); ys = 1 + r.rnd(9); zs = 1 + r.rnd(30)
        res = collision.collision_test(ds, x, y, z, xs, ys, zs, si, ss)
        for v in (res['zf'], res['count'], res['flag'], len(res['ptrs'])) + tuple(res['ptrs']) + ((res['di'] or 0),):
            mix(v)
    # pushing: each object has a weight and a size; the mover is the entry `selfk` or a free mover
    ents = []
    for i, (rec, marker, flag4, box) in enumerate(objs):
        ents.append(dict(rec=rec, pos=[b(box[0]), b(box[1]), b(box[5])], w=(i * 7 + seed) % 23, sz=(1 + (i + seed) % 7, 1 + (i * 3 + seed) % 6, 1 + (i * 5 + seed) % 20)))
    pl = dict(rec=0x2FF0, pos=[r.rnd(100), r.rnd(100), r.rnd(5)], w=0, sz=(7, 7, 30))
    for q in range(20):
        dx, dy = [(1, 0), (-1, 0), (0, 1), (0, -1)][r.rnd(4)]
        pl['pos'] = [b(r.rnd(110) - 5), b(r.rnd(110) - 5), r.rnd(5)]
        st, mv = push.push_move(bytes(ds), bytes(ss), ents, pl, dx, dy, 50)
        mix(['moved', 'free', 'blocked', 'solid', 'pushfail'].index(st)); mix(len(mv))
        for j in mv: mix(j)
    return h


coll_vec = [(s, collision_vector(s)) for s in range(1, 41)]

# ACL: run the start-up event (14) of every room with handlers; Python tools/acl_vm.py vs src/engine/acl.c
import acl, acl_vm
acl_vec = []
for d in sorted(glob.glob(os.path.join(root, 'cadaver*', '*', '[0-9]'))):
    mp2 = mjm.mjm_unpack(open(glob.glob(os.path.join(d, 'LEV*.MP2'))[0], 'rb').read())
    secs = mpsections.mp2_sections(mp2)
    thr = secs[3]['records']
    h = 0x811C9DC5
    cnt = 0
    for n, rr in enumerate(secs[0]['records']):
        if not secs[0]['present'][n] or len(rr) <= 0x20 or not rr[0x1F] or n >= 128:
            continue
        try:
            hs = acl.room_handlers(rr)
        except Exception:
            continue
        if not any(x['type'] == 14 for x in hs):
            continue
        things_ = {t: bytearray(r[:16]) for t, r in enumerate(thr) if len(r) >= 16 and secs[3]['present'][t] and t < 1000}
        vm = acl_vm.VM(things_, cur=0, room=n)
        acl_vm.fire(vm, hs, 14)
        cnt += 1
        for v in [n] + [things_[t][3] for t in sorted(things_)] + [things_[t][15] for t in sorted(things_)] + list(vm.vars):
            h = ((h ^ (v & 0xFF)) * 0x01000193) & 0xFFFFFFFF
    acl_vec.append((os.path.relpath(d, root), cnt, h))

# room exits of every room of every level directory: Python roomexits.py vs src/engine/roomexits.c
import roomexits, objplace, landing
exit_vec = []
for d in sorted(glob.glob(os.path.join(root, 'cadaver*', '*', '[0-9]'))):
    mp1 = mjm.mjm_unpack(open(glob.glob(os.path.join(d, 'LEV*.MP1'))[0], 'rb').read())
    mp2 = mjm.mjm_unpack(open(glob.glob(os.path.join(d, 'LEV*.MP2'))[0], 'rb').read())
    secs = mpsections.mp2_sections(mp2)
    parsed = {}
    rawhdr = {}
    for bi, (sz, blk) in enumerate(objsprites.blocks(mp1)):
        p = objsprites.parse_sprite_block(blk) if sz > 1 else None
        if p:
            parsed[bi] = struct.unpack('>16H', p[0])
            rawhdr[bi] = p[0]
    hd = lambda i: (parsed[i][8], parsed[i][9], parsed[i][10]) if i in parsed else None
    h = 0x811C9DC5
    cnt = 0
    for n, rec in enumerate(secs[0]['records']):
        if not secs[0]['present'][n]:
            continue
        ex = roomexits.room_exits(rec, secs[1]['records'], hd, rec[21] if len(rec) > 21 else 0, rec[20] if len(rec) > 20 else 0)
        for e in ex:
            cnt += 1
            for v in (n,) + tuple(e) + landing.landing(e, rec):
                for sh in (0, 8, 16, 24):
                    h = ((h ^ ((v >> sh) & 0xFF)) * 0x01000193) & 0xFFFFFFFF
        if not secs[3]['present']:
            continue
        rl = secs[2]['records'][n] if n < len(secs[2]['records']) and secs[2]['present'][n] else b''
        for k in range(len(rl) // 2):
            t = (rl[2 * k] << 8) | rl[2 * k + 1]
            tr = secs[3]['records'][t] if t < len(secs[3]['records']) and secs[3]['present'][t] else b''
            if len(tr) < 8 or tr[3] & 0x80:
                continue
            bk = (tr[6] << 8) | tr[7]
            if bk not in parsed:
                continue
            for v in (n, t) + objplace.trimmed_box(tr[0], tr[1], tr[2], rawhdr[bk]):
                for sh in (0, 8, 16, 24):
                    h = ((h ^ ((v >> sh) & 0xFF)) * 0x01000193) & 0xFFFFFFFF
            cnt += 1000000
    exit_vec.append((os.path.relpath(d, root), cnt, h))

with open(os.path.join(root, 'src', 'tests', 'vectors.h'), 'w') as o:
    o.write('/* generated by tools/gen_test_vectors.py - do not edit */\n')
    o.write('typedef struct { const char *path; long size; unsigned hash; } MjmVector;\n')
    o.write('static const MjmVector mjm_vectors[] = {\n')
    for r in rows:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *path; unsigned pixhash; } PicVector;\n')
    o.write('static const PicVector pic_vectors[] = {\n')
    for r in pics:
        o.write('\t{"%s", 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *path; int count; unsigned hash; } LngVector;\n')
    o.write('static const LngVector lng_vectors[] = {\n')
    for r in lngs:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *path; int count; unsigned hash; } SpriteVector;\n')
    o.write('static const SpriteVector sprite_vectors[] = {\n')
    for r in spr_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *path; unsigned hash; } Mp2Vector;\n')
    o.write('static const Mp2Vector mp2_vectors[] = {\n')
    for r in mp2_vec:
        o.write('\t{"%s", 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *dir; int tiles; int rooms; unsigned hash; } RoomVector;\n')
    o.write('static const RoomVector room_vectors[] = {\n')
    for r in room_vec:
        o.write('\t{"%s", %d, %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *dir; unsigned hash; } IntroVector;\n')
    o.write('static const IntroVector intro_vectors[] = {\n')
    for r in intro_vec:
        o.write('\t{"%s", 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *dir; int sprites; unsigned hash; } ObjVector;\n')
    o.write('static const ObjVector obj_vectors[] = {\n')
    for r in obj_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { unsigned seed; unsigned hash; } CollVector;\n')
    o.write('static const CollVector coll_vectors[] = {\n')
    for r in coll_vec:
        o.write('\t{%d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *dir; int exits; unsigned hash; } ExitVector;\n')
    o.write('static const ExitVector exit_vectors[] = {\n')
    for r in exit_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *dir; int rooms; unsigned hash; } AclVector;\n')
    o.write('static const AclVector acl_vectors[] = {\n')
    for r in acl_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
print(len(rows), 'mjm vectors,', len(pics), 'pictures,', len(lngs), 'language tables')
