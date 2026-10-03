"""Fits ComputeDerivedCharacterStats' per-class formulas by probing an emulation of both games' listings,
checks the fit against the emulator on random vectors, and prints the C tables used in chargen.c
(run: python3 fit_derived.py [--tables]). See docs23/overview.md, "character stat generation"."""
import os, sys, random, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from emu16 import *

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')

PATHS = {2: os.path.join(ROOT, 'yendor2', 'yendor2.asm'), 3: os.path.join(ROOT, 'yendor3', 'yendor3.asm')}
OFFS = list(range(0x58, 0x72, 2))
STATS = {0x58: 'Survival', 0x5A: 'Projectile', 0x5C: 'Slashing', 0x5E: 'Bashing', 0x60: 'Polearm', 0x64: 'Mapping',
         0x66: 'Navigation', 0x68: 'Bartering', 0x6A: 'Repair', 0x6C: 'Thievery', 0x6E: 'Linguistics', 0x70: 'Chemistry'}
# Casting (0x62) is not computed here.
LST = {}


def listing(game):
    if game not in LST:
        LST[game] = [l for l in parse_proc(PATHS[game], 'ComputeDerivedCharacterStats') if not l.strip().startswith('mov     si,')]
    return LST[game]


def derived(game, cls, attrs):
    rec = bytearray(512)
    for i, v in enumerate(attrs):
        rec[0x3C + 2 * i] = v & 255
        rec[0x3D + 2 * i] = v >> 8
    rec[0xE] = cls & 255
    rec[0xF] = cls >> 8
    e = Emu(listing(game), rec, {'ScaleByPercentRounded': scale_pct})
    e.run()
    cur = {o: e.rd(o) for o in OFFS}
    mx = {o: e.rd(o + 0x40) for o in OFFS}
    return cur, mx


def pct(a, p):
    return (((a * p) & M) + 50 & M) // 100


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


def fit(game):
    out = {}
    for off in STATS:
        per_class = {}
        terms_by_class = {}
        for cls in range(0, 12):
            base, _ = derived(game, cls, [0] * 6)
            b0 = base[off]
            terms = []
            flat = True
            for i in range(6):
                vals = []
                for a in (1, 10, 30, 50, 60, 99, 200):
                    at = [0] * 6
                    at[i] = a
                    cur, _ = derived(game, cls, at)
                    vals.append((a, (cur[off] - b0) & M))
                if any(v for _, v in vals):
                    flat = False
                    # copy?
                    if all(v == a for a, v in vals):
                        terms.append((i, 0))
                    else:
                        found = None
                        for p in range(1, 101):
                            if all(v == pct(a, p) for a, v in vals):
                                found = p
                                break
                        assert found, (game, hex(off), cls, i, vals)
                        terms.append((i, found))
            per_class[cls] = (flat, s16(b0), terms)
        out[off] = per_class
    return out


def check(game, model):
    rnd = random.Random(1234)
    for _ in range(3000):
        cls = rnd.randrange(0, 12)
        attrs = [rnd.randrange(0, 130) for _ in range(6)]
        cur, mx = derived(game, cls, attrs)
        for off, per in model.items():
            flat, b0, terms = per[cls]
            if flat:
                v = b0 & M
            else:
                v = b0
                for i, p in terms:
                    v += attrs[i] if p == 0 else pct(attrs[i], p)
                v &= M
            assert cur[off] == v == mx[off], (game, hex(off), cls, attrs, cur[off], v, mx[off])


def main():
    res = {}
    for g in (2, 3):
        m = fit(g)
        check(g, m)
        res[g] = m
        # class 0, 10, 11 identical?
        for off, per in m.items():
            assert per[0] == per[10] == per[11], (g, hex(off))
        print('game', g, 'ok')
    if '--tables' in sys.argv:
        print(gen_c())


def gen_c():
    names = {0: 'PartyStatStrength', 1: 'PartyStatDexterity', 2: 'PartyStatStamina', 3: 'PartyStatIntelligence', 4: 'PartyStatWisdom', 5: 'PartyStatCharisma'}
    out = []
    for g in (2, 3):
        m = fit(g)
        out.append('static const DerivedStatRule g_derivedRulesYendor%d[] = {' % g)
        for off, per in m.items():
            if g == 3 and off == 0x70:
                continue
            # sanity: terms shared
            shared = None
            for cls in range(0, 10):
                flat, b0, terms = per[cls]
                if not flat:
                    if shared is None:
                        shared = terms
                    assert shared == terms
            terms = shared or []
            flat = []
            bonus = []
            for cls in range(0, 10):
                f, b0, t = per[cls]
                flat.append(str(b0) if f else '-1')
                bonus.append('0' if f else str(b0))
            tt = ', '.join('{%s, %d}' % (names[i], p) for i, p in terms)
            out.append('    {0x%02X, %d, {%s}, {%s}, {%s}}, /* %s */' % (off, len(terms), tt, ', '.join(flat), ', '.join(bonus), STATS[off]))
        out.append('};')
        out.append('')
    return '\n'.join(out)


if __name__ == '__main__':
    main()
