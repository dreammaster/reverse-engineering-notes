#!/usr/bin/env python3
"""
renames.py - maintain the rename tables in renames/ (see renames/README.md).

  tools/renames.py check  [<game>/<stem> ...]   validate tables against the unpacked binaries + .asm exports
  tools/renames.py fill   [<game>/<stem> ...]   fill in missing segoff / sig columns from the binary
  tools/renames.py port   <src> <dst>           find the routines of one build in another (CAD.EXE -> GAME.EXE ...)
  tools/renames.py merge  <game>/<stem>         merge an export_renames.py dump (renames done in IDA) into the table
  tools/renames.py stats                        counts per table

<game>/<stem> is the IDB name under disasm/, e.g. cadaver1/cad, cadaver2/game, cadaver1/intro.
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
import renames_lib as R  # noqa: E402


def table_path(key):
    return os.path.join(ROOT, 'renames', key + '.tsv')


def all_keys():
    return sorted(os.path.relpath(p, os.path.join(ROOT, 'renames'))[:-4].replace(os.sep, '/')
                  for p in glob.glob(os.path.join(ROOT, 'renames', '*', '*.tsv')) if not p.endswith('.export.tsv'))


def exe_for(key):
    game, stem = key.split('/')
    for f in os.listdir(os.path.join(ROOT, 'unpacked', game)):
        low = f.lower()
        s = low[:-4] if low.endswith('.exe') else low.replace('.', '_')
        if s == stem:
            return os.path.join(ROOT, 'unpacked', game, f)
    raise SystemExit('no unpacked binary for %s' % key)


def load_image(key):
    game, stem = key.split('/')
    return R.Image(exe_for(key), os.path.join(ROOT, 'disasm', game, stem + '.idc'))


def read_header_comments(path):
    out = []
    if os.path.exists(path):
        for l in open(path, encoding='utf-8'):
            if l.startswith('#') and not l.startswith('# ea\t'):
                out.append(l[2:].rstrip('\n') if l.startswith('# ') else l[1:].rstrip('\n'))
            elif l.strip():
                break
    return '\n'.join(out)


def save(key, rows):
    p = table_path(key)
    os.makedirs(os.path.dirname(p), exist_ok=True)
    R.write_tsv(p, rows, read_header_comments(p))


def auto_name(row):
    """Name IDA's auto-namer would have used for this row."""
    if row.kind == 'func':
        return 'sub_%X' % row.ea
    if row.kind == 'label':
        return 'loc_%X' % row.ea
    return ''


def cmd_check(keys):
    bad = 0
    for key in keys or all_keys():
        rows, errs = R.parse_tsv(table_path(key))
        img = load_image(key)
        game, stem = key.split('/')
        asm_path = os.path.join(ROOT, 'disasm', game, stem + '.asm')
        asm = open(asm_path, encoding='latin1').read() if os.path.exists(asm_path) else ''
        seen_ea, seen_name = {}, {}
        for r in rows:
            where = '%s:%d (%#x %s)' % (key, r.line, r.ea, r.new)
            if r.ea in seen_ea:
                errs.append('%s: duplicate address (also line %d)' % (where, seen_ea[r.ea]))
            seen_ea[r.ea] = r.line
            if r.new in seen_name:
                errs.append('%s: duplicate name (also line %d)' % (where, seen_name[r.new]))
            seen_name[r.new] = r.line
            if img.segs and img.seg_of(r.ea) is None:
                errs.append('%s: address outside every segment' % where)
            if r.segoff and img.segs and r.segoff != img.segoff(r.ea):
                errs.append('%s: segoff %s should be %s' % (where, r.segoff, img.segoff(r.ea)))
            if r.sig and not R.sig_matches(img.bytes_at(r.ea, 64), r.sig):
                errs.append('%s: signature does not match the binary (have %s)' % (where, R.default_sig(img, r.ea, len(R.sig_bytes(r.sig)))))
            if r.old and asm and not re.search(r'\b%s\b' % re.escape(r.old), asm):
                errs.append('%s: old name %s not found in %s' % (where, r.old, os.path.relpath(asm_path, ROOT)))
            if r.old and r.kind != 'data' and r.old != auto_name(r):
                errs.append('%s: old name %s does not match the address (expected %s)' % (where, r.old, auto_name(r)))
        print('%-18s %3d rows, %d problem(s)' % (key, len(rows), len(errs)))
        for e in errs:
            print('   ', e)
        bad += len(errs)
    return 1 if bad else 0


def cmd_fill(keys):
    for key in keys or all_keys():
        rows, errs = R.parse_tsv(table_path(key))
        if errs:
            print('\n'.join(errs)); return 1
        img = load_image(key)
        n = 0
        for r in rows:
            if not r.segoff:
                r.segoff = img.segoff(r.ea); n += 1
            if not r.sig and (r.kind != 'data' or (r.size and r.size <= 64)):
                r.sig = R.default_sig(img, r.ea, min(r.size or 16, 24)); n += 1
        save(key, rows)
        print('%s: filled %d cell(s)' % (key, n))
    return 0


def cmd_port(src, dst):
    srows, errs = R.parse_tsv(table_path(src))
    if errs:
        print('\n'.join(errs)); return 1
    simg, dimg = load_image(src), load_image(dst)
    dpath = table_path(dst)
    drows = R.parse_tsv(dpath)[0] if os.path.exists(dpath) else []
    have = {r.new for r in drows}
    delta, added, failed = None, 0, 0
    for r in sorted(srows, key=lambda r: r.ea):
        if r.new in have:
            continue
        if r.kind == 'data':
            # strings / tables can be found by their content; variables cannot
            lit = r.sig and '??' not in r.sig and len(R.sig_bytes(r.sig)) >= 8
            hits = [m.start() for m in re.finditer(re.escape(bytes(R.sig_bytes(r.sig))), dimg.data)] if lit else []
            if len(hits) != 1:
                print('  skip   %-24s data (%s)' % (r.new, 'content not unique/absent in target' if lit else 'no literal content to search for'))
                continue
            ea = hits[0] + R.LOAD_BASE
            com = (r.comment + ' ' if r.comment else '') + '[ported from %s %#x by content match]' % (src, r.ea)
            drows.append(R.Row(ea, dimg.segoff(ea), 'data', '', r.new, r.conf, r.size, r.sig, com))
            print('  ok     %-24s %#x -> %#x (content)' % (r.new, r.ea, ea)); added += 1
            continue
        ea, how, score = R.find_equivalent(simg, r.ea, dimg, delta)
        if ea is None:
            print('  FAILED %-24s %#x: %s' % (r.new, r.ea, how)); failed += 1
            continue
        delta = ea - r.ea
        conf = r.conf if how.startswith('sig') else ('likely' if r.conf == 'verified' else r.conf)
        # keep 'unlabeled in IDA' as unlabeled; otherwise IDA's default name for the new address
        old = {'func': 'sub_%X', 'label': 'loc_%X'}[r.kind] % ea if r.old else ''
        com = (r.comment + ' ' if r.comment else '') + '[ported from %s %#x by %s match]' % (src, r.ea, how)
        drows.append(R.Row(ea, dimg.segoff(ea), r.kind, old, r.new, conf, 0,
                           R.default_sig(dimg, ea, 16), com))
        print('  ok     %-24s %#x -> %#x (%s, delta %+#x)' % (r.new, r.ea, ea, how, delta)); added += 1
    save(dst, drows)
    print('%s: %d added, %d failed' % (dst, added, failed))
    return 0


def cmd_merge(key):
    exp = os.path.join(ROOT, 'renames', key + '.export.tsv')
    if not os.path.exists(exp):
        print('no %s (run ida_scripts/export_renames.py first)' % exp); return 1
    img = load_image(key)
    erows, errs = R.parse_tsv(exp)
    if errs:
        print('\n'.join(errs)); return 1
    p = table_path(key)
    rows = R.parse_tsv(p)[0] if os.path.exists(p) else []
    by_ea = {r.ea: r for r in rows}
    added = changed = 0
    for e in erows:
        cur = by_ea.get(e.ea)
        if cur is None:
            e.segoff = img.segoff(e.ea)
            e.sig = e.sig or (R.default_sig(img, e.ea, 16) if e.kind != 'data' else '')
            rows.append(e); added += 1
        elif cur.new != e.new:
            print('  note: %#x is %s in the table but %s in the IDB -- table kept' % (e.ea, cur.new, e.new))
            changed += 1
    save(key, rows)
    print('%s: %d row(s) added from the IDB export, %d disagreement(s)' % (key, added, changed))
    return 0


def cmd_stats():
    for key in all_keys():
        rows = R.parse_tsv(table_path(key))[0]
        c = {k: sum(1 for r in rows if r.conf == k) for k in R.CONF_ORDER}
        print('%-18s %3d rows  (verified %d, likely %d, guess %d)' % (key, len(rows), c['verified'], c['likely'], c['guess']))
    return 0


def main(a):
    if not a:
        print(__doc__); return 2
    cmd, rest = a[0], a[1:]
    if cmd == 'check': return cmd_check(rest)
    if cmd == 'fill': return cmd_fill(rest)
    if cmd == 'port' and len(rest) == 2: return cmd_port(*rest)
    if cmd == 'merge' and len(rest) == 1: return cmd_merge(rest[0])
    if cmd == 'stats': return cmd_stats()
    print(__doc__); return 2


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
