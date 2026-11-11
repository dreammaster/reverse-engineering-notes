#!/usr/bin/env python3
"""Generate src/tests/vectors.h from the Python reference decoders (tools/mjm.py, tools/pc1.py)."""
import glob, os, struct, sys
here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
import statuspanel, verbs, message, status, push, panel, mjm, pc1, lng, pcman, mpsections, roomview, intro, objsprites, room_objects, animation

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
# ---- status text: room names + DAY n on a blank canvas, every room of every level
font = mpsections.andata_blocks(mjm.mjm_unpack(open(os.path.join(root, 'cadaver1', 'DATA', 'ANDATA.DAT'), 'rb').read()))[4]
status_vec = []
for d in sorted(glob.glob(os.path.join(root, 'cadaver1', 'DATA', '[1-5]')) + glob.glob(os.path.join(root, 'cadaver2', 'TATA', '[1-4]')) +
                glob.glob(os.path.join(root, 'cadaver2', 'DATA', '[1-5]'))):
    mp1, mp2, mp3 = roomview.level_files(d)
    secs = mpsections.mp2_sections(mp2)
    names = lng.load(glob.glob(os.path.join(d, 'ENG*.LNG'))[0])
    tbl = status.name_table_of(mp1)
    h, cnt = 0x811C9DC5, 0
    for r, rec in enumerate(secs[0]['records']):
        if len(rec) < 0x1B:
            continue
        nm = status.room_name_string(rec, tbl, names)
        cv = bytearray(64000)
        x, y, w = status.DAY_BOX
        status.draw_centred(cv, status.DAY_BOX, 'DAY %d' % (1 + r % 12), font)
        status.draw_centred(cv, status.NAME_BOX, nm, font)
        for c in nm.encode() + b'\0' + bytes(cv):
            h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF
        cnt += 1
    status_vec.append((os.path.relpath(d, root), cnt, h))
# ---- message scrolls: layout + rendered rectangle of every 7th string of every level's language file
_an = mpsections.andata_blocks(mjm.mjm_unpack(open(os.path.join(root, 'cadaver1', 'DATA', 'ANDATA.DAT'), 'rb').read()))
msg_vec = []
for d in sorted(glob.glob(os.path.join(root, 'cadaver1', 'DATA', '[1-5]')) + glob.glob(os.path.join(root, 'cadaver2', 'TATA', '[1-4]')) +
                glob.glob(os.path.join(root, 'cadaver2', 'DATA', '[1-5]'))):
    names = lng.load(glob.glob(os.path.join(d, 'ENG*.LNG'))[0])
    h, cnt = 0x811C9DC5, 0
    for i in range(0, len(names), 7):
        t = names[i]
        if not t or len(t) > 200:
            continue
        cv = bytearray(64000)
        w, lines, fill, mrows = message.draw_message(cv, t, i % 4, _an[2], _an[3], rnd=lambda: 3)
        x, y = 0x20 + 16 * (i % 4), 0x14 + 16 * (i % 4)
        crop = b''.join(bytes(cv[(y + r) * 320 + x:(y + r) * 320 + x + 8 * (fill + 2)]) for r in range(8 * (mrows + 4)))
        for c in bytes([w, len(lines)]) + '\0'.join(lines).encode() + b'\0' + crop:
            h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF
        cnt += 1
    msg_vec.append((os.path.relpath(d, root), cnt, h))
# ---- verb lists + icon drawing for every sprite block header of every level
verb_vec = []
for d in sorted(glob.glob(os.path.join(root, 'cadaver1', 'DATA', '[1-5]')) + glob.glob(os.path.join(root, 'cadaver2', 'TATA', '[1-4]')) +
                glob.glob(os.path.join(root, 'cadaver2', 'DATA', '[1-5]'))):
    mp1 = roomview.level_files(d)[0]
    h, cnt = 0x811C9DC5, 0
    for bi, (sz, blk) in enumerate(objsprites.blocks(mp1)):
        if sz <= 1 or not objsprites.parse_sprite_block(blk):
            continue
        for fl2 in (0, 4, 0x40, 0x44):
            vl = verbs.verb_list(blk[:32], fl2, 0)
            cv = bytearray(64000)
            verbs.draw_verbs(cv, vl, _an[0])
            for c in bytes(vl) + bytes(cv[145 * 320:196 * 320]):
                h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF
        cnt += 1
    verb_vec.append((os.path.relpath(d, root), cnt, h))
# ---- character status dialog: a few states per level, rendered rectangle
sd_vec = []
for d in sorted(glob.glob(os.path.join(root, 'cadaver1', 'DATA', '[1-5]')) + glob.glob(os.path.join(root, 'cadaver2', 'TATA', '[1-4]')) +
                glob.glob(os.path.join(root, 'cadaver2', 'DATA', '[1-5]'))):
    names = lng.load(glob.glob(os.path.join(d, 'ENG*.LNG'))[0])
    mp1 = roomview.level_files(d)[0]
    tb = statuspanel.rank_table(mp1)
    h, cnt = 0x811C9DC5, 0
    for k in range(6):
        sc = (k * 7919 * 13) % 65000
        st = (k * 37 % 100 + 1, 100, k * 211 % 5000, sc, 1 + k % 5, k * 17 % 100)
        cv = bytearray(64000)
        statuspanel.draw_dialog(cv, _an[2], _an[3], names, st[0], st[1], st[2], st[3], st[4], st[5], statuspanel.rank_string(tb, sc))
        for c in bytes(cv[10 * 320:74 * 320]):
            h = ((h ^ c) * 0x01000193) & 0xFFFFFFFF
        cnt += 1
    sd_vec.append((os.path.relpath(d, root), cnt, h))
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
# ---- native routines of ROUTINEn.TAD: deterministic scenarios on the Python 8086 interpreter (tools/x86.py)
import x86
def tad_entries(u):
    base = struct.unpack_from('<H', u, 0x24)[0]            # mov ax, <data table offset> in the init stub (0x2C; the Payoff's new levels: 0x80)
    t = struct.unpack_from('<5H', u, base)
    ents = []
    p = t[3]
    while p + 2 <= len(u):
        w = struct.unpack_from('<H', u, p)[0]
        if w == 0 or w > 0x1000:
            break
        ents.append(w)
        p += 2
    for k in range(9, 16):
        if base + 4 * k + 2 <= len(u):
            w = struct.unpack_from('<H', u, base + 4 * k)[0]
            if 0x40 < w < 0x1000:
                ents.append(w + 2)
    return ents


def tad_scenario(u, ent, seed):
    r = Rng(seed)
    mem = bytearray(0x100000)
    DSG, SSG, TADG, HKG = 0x1000, 0x2100, 0x3000, 0x4000
    mem[TADG * 16:TADG * 16 + len(u)] = u
    ds = DSG * 16
    mem[ds + 0x78E:ds + 0x792] = struct.pack('<HH', 1, HKG)
    mem[ds + 0xB6:ds + 0xB8] = struct.pack('<H', 0x100)
    for i in range(32):
        mem[SSG * 16 + 0x100 + 2 * i:SSG * 16 + 0x102 + 2 * i] = struct.pack('<H', r.next() & 0xFFFF)
    for _ in range(r.rnd(40)):
        a = 0x590 + r.rnd(0x190)
        k = r.rnd(7)
        mem[ds + a] = (0, 1, 2, 4, 8, 0xFF)[k] if k < 6 else r.rnd(256)
    for _ in range(r.rnd(30)):
        mem[ds + 0x3D00 + r.rnd(0x300)] = r.rnd(256)
    cpu = x86.CPU(mem)
    cpu.r['ax'] = r.rnd(12)
    for n in ('bx', 'cx', 'dx', 'si', 'di'):
        cpu.r[n] = r.next() & 0xFFFF
    cpu.r['bp'] = 0x3DDA + 0x10 * r.rnd(4)
    cpu.r['sp'] = 0xFF00 - 2
    cpu.s.update(ds=DSG, es=DSG, ss=SSG, cs=TADG)
    cpu.ww(SSG, 0xFF00 - 2, 1)
    cpu.ip = ent
    trace = []

    def hook(c):
        s792 = c.rw(DSG, 0x792)
        trace.append((s792, c.r['ax'], c.r['bx'], c.r['cx'], c.r['dx'], c.r['si'], c.r['di'], c.r['bp']))
        c.r['ax'] = (c.r['ax'] * 31 + s792 + c.r['bx'] * 7 + c.r['cx'] * 3 + c.r['dx']) & 0xFFFF
        c.ip = c.pop()
        c.s['cs'] = c.pop()
    cpu.hooks[(HKG, 1)] = hook
    h = 0x811C9DC5
    try:
        cpu.run(stop=(TADG, 1), max_steps=100000)
        okf = 1
    except x86.X86Error:
        okf = 0
    vals = [okf] + [cpu.r[n] for n in ('ax', 'bx', 'cx', 'dx', 'si', 'di', 'bp', 'sp')]
    for t in trace:
        vals += list(t)
    for a in list(range(0x590, 0x720)) + list(range(0x3D00, 0x4000)):
        vals.append(mem[ds + a])
    return vals


tad_vec = []
for d in sorted(glob.glob(os.path.join(root, 'cadaver1', 'DATA', '[1-5]')) + glob.glob(os.path.join(root, 'cadaver2', 'TATA', '[1-4]')) +
                glob.glob(os.path.join(root, 'cadaver2', 'DATA', '[1-5]'))):
    fs = glob.glob(os.path.join(d, 'ROUTINE*.TAD')) + glob.glob(os.path.join(d, 'ROUTINE*.PAK'))
    if not fs:
        continue
    u = mjm.mjm_unpack(open(fs[0], 'rb').read())
    if u[0x20] != 0xBB or u[0x23] != 0xB8:
        continue                  # not the known init stub
    h, cnt = 0x811C9DC5, 0
    for ei, ent in enumerate(tad_entries(u)):
        for sc in range(4):
            for v in tad_scenario(u, ent, 1000 + ei * 17 + sc):
                for b in (v & 0xFFFF).to_bytes(2, 'little'):
                    h = ((h ^ b) * 0x01000193) & 0xFFFFFFFF
            cnt += 1
    tad_vec.append((os.path.relpath(d, root), cnt, h))

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
        if not any(x['type'] in (28, 6, 14) for x in hs):
            continue
        things_ = {t: bytearray(r[:16]) for t, r in enumerate(thr) if len(r) >= 16 and secs[3]['present'][t] and t < 1000}
        vm = acl_vm.VM(things_, cur=0, room=n)
        arm = [True] * len(hs)
        for t in (28, 6, 14, 14):
            acl_vm.fire_event(vm, hs, t, 0, arm)
        cnt += 1
        for v in [n] + [things_[t][3] for t in sorted(things_)] + [things_[t][15] for t in sorted(things_)] + list(vm.vars):
            h = ((h ^ (v & 0xFF)) * 0x01000193) & 0xFFFFFFFF
    acl_vec.append((os.path.relpath(d, root), cnt, h))

# thing events 7 (touched) and 9 (contact, argument 0) of every thing that has such handlers
aclt_vec = []
for d in sorted(glob.glob(os.path.join(root, 'cadaver1', 'DATA', '[1-5]')) + glob.glob(os.path.join(root, 'cadaver2', 'TATA', '[1-4]')) +
                glob.glob(os.path.join(root, 'cadaver2', 'DATA', '[1-5]'))):
    mp1, mp2, mp3 = roomview.level_files(d)
    secs = mpsections.mp2_sections(mp2)
    thr = secs[3]['records']
    h = 0x811C9DC5
    cnt = 0
    for t, rr in enumerate(thr):
        if t >= 1000 or not secs[3]['present'][t] or len(rr) <= 16 or not rr[11]:
            continue
        try:
            hs = acl.thing_handlers(rr)
        except Exception:
            continue
        if not any(x['type'] in (7, 9) for x in hs):
            continue
        things_ = {k: bytearray(r[:16]) for k, r in enumerate(thr) if len(r) >= 16 and secs[3]['present'][k] and k < 1000}
        vm = acl_vm.VM(things_, cur=t, room=0)
        arm = [True] * len(hs)
        for ty in (7, 9):
            acl_vm.fire_event(vm, hs, ty, 0, arm, cur=t, follow=False)
        cnt += 1
        for v in [t] + [things_[k][3] for k in sorted(things_)] + [things_[k][15] for k in sorted(things_)] + list(vm.vars):
            h = ((h ^ (v & 0xFF)) * 0x01000193) & 0xFFFFFFFF
    aclt_vec.append((os.path.relpath(d, root), cnt, h))

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
    o.write('typedef struct { const char *dir; int rooms; unsigned hash; } StatusVector;\n')
    o.write('static const StatusVector status_vectors[] = {\n')
    for r in status_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('static const StatusVector aclt_vectors[] = {\n')
    for r in aclt_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('static const StatusVector msg_vectors[] = {\n')
    for r in msg_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('static const StatusVector verb_vectors[] = {\n')
    for r in verb_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('static const StatusVector tad_vectors[] = {\n')
    for r in tad_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('static const StatusVector sd_vectors[] = {\n')
    for r in sd_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
    o.write('typedef struct { const char *dir; int rooms; unsigned hash; } AclVector;\n')
    o.write('static const AclVector acl_vectors[] = {\n')
    for r in acl_vec:
        o.write('\t{"%s", %d, 0x%08XU},\n' % r)
    o.write('};\n')
print(len(rows), 'mjm vectors,', len(pics), 'pictures,', len(lngs), 'language tables')
