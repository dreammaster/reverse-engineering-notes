"""Statement preparation and structured emission for decomp.py."""
import re

from decomp import K, is_k, simp, negate_cond, walk, Unsupported
from decomp_gen import exprs_of, count_uses
from decomp_out import Renderer, Names, CMPOP


# --------------------------------------------------------------------------------------------------------------------------------- argument packing
TEXTVARS = set()      # phi variables that hold a text reference (pointer or message id) built from two halves


def pack_args(args, null_ok=False):
    out = []
    i = 0
    while i < len(args):
        a = args[i]
        b = args[i + 1] if i + 1 < len(args) else None
        if b is not None:
            if a[0] == "k" and b[0] == "k" and (b[1] & 0xFF00) == 0xF000:
                out.append(("msg", (b[1] >> 0) & 0xFF if False else b[1] & 0xFF, a[1]))
                i += 2
                continue
            if a[0] == "addr" and b[0] in ("dseg", "segof"):
                out.append(("str", a[1]))
                i += 2
                continue
            if a[0] == "res" and b[0] == "res" and a[1] == b[1] and a[2] == "ax" and b[2] == "dx":
                out.append(("res", a[1], "ax"))      # a far pointer returned by a call: one value
                i += 2
                continue
            if a[0] == "loc" and b[0] == "loc" and a[1] == b[1] and b[2] == a[2] + 2:
                out.append(("loc", a[1], a[2], 4))
                i += 2
                continue
            if null_ok and a == K(0) and b == K(0):
                out.append(("null",))
                i += 2
                continue
            if a[0] in ("phi", "in") and b[0] in ("phi", "in") and a[1] == "ax" and b[1] == "dx" and a[2] == b[2]:
                if ("phi",) + a[1:] in TEXTVARS:
                    out.append(("textvar", ("phi",) + a[1:]))
                else:
                    out.append(("phi" if a[0] == "phi" else "in",) + a[1:])         # a far pointer built in two phi variables: the segment half carries nothing here
                i += 2
                continue
            if a[0] == "sel" and b[0] == "sel" and a[1] == b[1] and b[2] == ("dseg",) and b[3] == K(0) and a[3] == K(0):
                out.append(("sel", a[1], ("far", a[2], ("dseg",)), ("null",)))
                i += 2
                continue
            if b[0] == "k" and (b[1] & 0xFF00) == 0xF000 and a[0] != "k":
                out.append(("words", a, b))
                i += 2
                continue
            if a[0] == "k" and b[0] == "dseg":
                out.append(("kstr", a[1]))
                i += 2
                continue
            if b[0] == "dseg":
                out.append(("far", a, b))
                i += 2
                continue
        out.append(a)
        i += 1
    return out


def subst(e, fn):
    r = fn(e)
    if r is not None:
        return r
    if not isinstance(e, tuple):
        return e
    return (e[0],) + tuple(subst(x, fn) if isinstance(x, tuple) else ([subst(y, fn) if isinstance(y, tuple) else y for y in x] if isinstance(x, list) else x) for x in e[1:])


def prepare(f):
    """packs call arguments, propagates call results into the variable they are stored in, removes dead temporaries"""
    for b in f.blocks.values():
        new = []
        for s in b.stmts:
            if s[0] == "call":
                new.append(("call", s[1], s[2], pack_args(s[3])))
            else:
                new.append(s)
        b.stmts = new
    # copy propagation of results into the variable they were stored in (same block)
    for b in f.blocks.values():
        st = b.stmts
        for i in range(len(st) - 1):
            if st[i][0] == "call" and st[i + 1][0] == "set":
                cid = st[i][1]
                lhs, rhs = st[i + 1][1], st[i + 1][2]
                if rhs == ("res", cid, "ax") and lhs[0] in ("loc", "tmp", "glob"):
                    key = lhs[1] if lhs[0] != "tmp" else None
                    rep = lambda e, cid=cid, lhs=lhs: lhs if e == ("res", cid, "ax") else None
                    for j in range(i + 2, len(st)):
                        s = st[j]
                        if s[0] == "set":
                            st[j] = ("set", subst(s[1], rep) if s[1][0] not in ("loc", "glob") else s[1], subst(s[2], rep))
                            if s[1][0] == lhs[0] and s[1][1] == lhs[1]:
                                break
                        elif s[0] == "call":
                            st[j] = ("call", s[1], s[2], [subst(a, rep) for a in s[3]])
                            if lhs[0] == "glob":
                                break
                    else:
                        t = b.term
                        if t[0] == "if":
                            b.term = ("if", subst(t[1], rep)) + t[2:]
                        elif t[0] == "ret":
                            b.term = ("ret",) + tuple(subst(x, rep) if isinstance(x, tuple) else x for x in t[1:])
    # stores to si / di that nothing reads (they were only the operands of the compiler's inline string functions)
    for reg in ("si", "di"):
        read = any(x[0] == "loc" and x[1] == reg for b in f.blocks.values() for e in exprs_of(b) for x in walk(e) if not (False))
        # exprs_of lists the target of a set as well: look at the values only
        read = False
        for b in f.blocks.values():
            for s in b.stmts:
                if s[0] == "set" and s[1] == ("loc", reg, 0, 2):
                    continue
                es = [s[2]] if s[0] == "set" else list(s[3])
                if s[0] == "set" and s[1][0] == "mem":
                    es.append(s[1])
                if any(x[0] == "loc" and x[1] == reg for e in es for x in walk(e)):
                    read = True
            for e in exprs_of(Block_(b.term)):
                if any(x[0] == "loc" and x[1] == reg for x in walk(e)):
                    read = True
        if not read:
            for b in f.blocks.values():
                b.stmts = [s for s in b.stmts if not (s[0] == "set" and s[1][0] == "loc" and s[1][1] == reg)]
    # dead temporaries
    changed = True
    while changed:
        changed = False
        used = set()
        for b in f.blocks.values():
            for s in b.stmts:
                es = [s[2]] if s[0] == "set" else list(s[3])
                if s[0] == "set" and s[1][0] != "tmp":
                    es.append(s[1])
                if s[0] == "set" and s[1][0] == "tmp":
                    pass
                for e in es:
                    for x in walk(e):
                        if x[0] == "tmp":
                            used.add(x[1])
            t = b.term
            for e in exprs_of(Block_(t)):
                for x in walk(e):
                    if x[0] == "tmp":
                        used.add(x[1])
        for b in f.blocks.values():
            keep = [s for s in b.stmts if not (s[0] == "set" and s[1][0] == "tmp" and s[1][1] not in used)]
            if len(keep) != len(b.stmts):
                b.stmts = keep
                changed = True


class Block_:
    """adapter so exprs_of can be used on a terminator alone"""
    def __init__(self, term):
        self.stmts = []
        self.term = term


# --------------------------------------------------------------------------------------------------------------------------------- structuring
def remove_dead_vars(f):
    """sets of phi variables and temporaries that nothing reads"""
    changed = True
    while changed:
        changed = False
        used = set()
        for b in f.blocks.values():
            for s in b.stmts:
                es = [s[2]] if s[0] == "set" else list(s[3])
                if s[0] == "set" and s[1][0] == "mem":
                    es.append(s[1])
                for e in es:
                    for x in walk(e):
                        if x[0] in ("phi", "tmp", "in"):
                            used.add(("phi",) + x[1:] if x[0] == "in" else x)
            for e in exprs_of(Block_(b.term)):
                for x in walk(e):
                    if x[0] in ("phi", "tmp", "in"):
                        used.add(("phi",) + x[1:] if x[0] == "in" else x)
        locs_read = set()
        for b in f.blocks.values():
            for s in b.stmts:
                es = [s[2]] if s[0] == "set" else list(s[3])
                if s[0] == "set" and s[1][0] == "mem":
                    es.append(s[1])
                for e in es:
                    for x in walk(e):
                        if x[0] == "loc":
                            locs_read.add(x[1])
            for e in exprs_of(Block_(b.term)):
                for x in walk(e):
                    if x[0] == "loc":
                        locs_read.add(x[1])
        for b in f.blocks.values():
            keep = [s for s in b.stmts if not (s[0] == "set" and (s[1][0] in ("phi", "tmp") and s[1] not in used or s[1][0] == "loc" and s[1][1] not in locs_read and s[1][1] not in ("si", "di") and not s[1][1].startswith("arg")))]
            if len(keep) != len(b.stmts):
                b.stmts = keep
                changed = True


def repack_calls(f):
    for b in f.blocks.values():
        b.stmts = [("call", s[1], s[2], pack_args(s[3])) if s[0] == "call" else s for s in b.stmts]


def pair_far_sets(f):
    """two stores that fill the halves of a far pointer variable become one store of the pointer"""
    for b in f.blocks.values():
        st = b.stmts
        i = 0
        while i < len(st) - 1:
            a, c = st[i], st[i + 1]
            adjacent = False
            if a[0] == "set" and c[0] == "set" and a[1][0] == "loc" and c[1][0] == "loc":
                if a[1][1] == c[1][1] and c[1][2] == a[1][2] + 2:
                    adjacent = True
                elif a[1][1] in f.locs and c[1][1] in f.locs and a[1][2] == 0 and c[1][2] == 0 and f.locs[c[1][1]][1] == f.locs[a[1][1]][1] + 2:
                    adjacent = True        # two names of one dword (var_4 / var_2)
            if adjacent:
                lo, hi = a[2], c[2]
                new = None
                if lo[0] == "res" and hi[0] == "res" and lo[1] == hi[1] and lo[2] == "ax" and hi[2] == "dx":
                    new = lo
                elif lo[0] == "addr" and hi[0] in ("segof", "dseg"):
                    new = lo
                elif lo[0] in ("phi", "in") and hi[0] in ("phi", "in") and lo[1] == "ax" and hi[1] == "dx" and lo[2] == hi[2]:
                    new = lo
                elif lo[0] == "str" or (lo[0] == "kstr" and hi[0] == "dseg"):
                    new = lo
                if new is not None:
                    st[i:i + 2] = [("set", ("loc", a[1][1], a[1][2], 4), new)]
                    continue
            i += 1


def merge_diamonds(f):
    """if (c) x = a; else x = b;  ->  x = c ? a : b"""
    changed = True
    while changed:
        changed = False
        for pid in list(f.order):
            p = f.blocks.get(pid)
            if p is None or p.term[0] != "if":
                continue
            c, tid, fid = p.term[1], p.term[2], p.term[3]
            if tid == fid:
                continue
            t, fl = f.blocks.get(tid), f.blocks.get(fid)
            if t is None or fl is None or t.retonly or fl.retonly or t.pred != [pid] or fl.pred != [pid]:
                continue
            if t.term[0] != "goto" or fl.term[0] != "goto" or t.term[1] != fl.term[1]:
                continue
            if len(t.stmts) != len(fl.stmts) or not t.stmts:
                continue
            if not all(a[0] == "set" and b[0] == "set" and a[1] == b[1] and a[1][0] in ("phi", "tmp") for a, b in zip(t.stmts, fl.stmts)):
                continue
            pairs = list(zip(t.stmts, fl.stmts))
            skip = set()
            for ia, (a, b) in enumerate(pairs):
                if a[1][0] == "phi" and a[1][1] == "ax":
                    for ib, (d, e) in enumerate(pairs):
                        if d[1][0] == "phi" and d[1][1] == "dx" and d[1][2] == a[1][2]:
                            t1, t2 = pack_args([a[2], d[2]], True), pack_args([b[2], e[2]], True)
                            if len(t1) == 1 and len(t2) == 1:
                                TEXTVARS.add(a[1])
                                p.stmts.append(("set", a[1], ("sel", c, ("text", t1[0]), ("text", t2[0]))))
                                skip.update((ia, ib))
            for ia, (a, b) in enumerate(pairs):
                if ia not in skip:
                    p.stmts.append(("set", a[1], ("sel", c, a[2], b[2])))
            jid = t.term[1]
            j = f.blocks[jid]
            j.pred = [x for x in j.pred if x not in (tid, fid)] + [pid]
            p.term = ("goto", jid)
            p.succ = [jid]
            del f.blocks[tid]
            del f.blocks[fid]
            f.order.remove(tid)
            f.order.remove(fid)
            changed = True


def has_res_of_calls(e):
    return any(x[0] == "res" for x in walk(e))


def forward_variables(f):
    """a phi / temporary that is set and then used once, in the next statement, is replaced by its value"""
    for b in f.blocks.values():
        i = 0
        while i < len(b.stmts) - 1:
            s0 = b.stmts[i]
            if s0[0] == "set" and s0[1][0] in ("phi", "tmp"):
                v = s0[1]
                total = sum(1 for bb in f.blocks.values() for e in exprs_of(bb) for x in walk(e) if x == v) - 1
                k = i + 1
                nxt = b.stmts[k]
                while nxt[0] == "call" and Renderer.PURE.match(str(nxt[2])) and not any(x == v for a in nxt[3] for x in walk(a)) and k + 1 < len(b.stmts):
                    k += 1
                    nxt = b.stmts[k]
                es = [nxt[2]] if nxt[0] == "set" else list(nxt[3])
                if nxt[0] == "set" and nxt[1][0] not in ("phi", "tmp", "loc", "glob"):
                    es.append(nxt[1])
                uses = sum(1 for e in es for x in walk(e) if x == v)
                if total == 1 and uses == 1:
                    rep = lambda e, v=v, val=s0[2]: val if e == v else None
                    if nxt[0] == "set":
                        b.stmts[k] = ("set", subst(nxt[1], rep) if nxt[1][0] == "mem" else nxt[1], subst(nxt[2], rep))
                    else:
                        b.stmts[k] = ("call", nxt[1], nxt[2], [subst(a, rep) for a in nxt[3]])
                    del b.stmts[i]
                    continue
            i += 1


def coalesce(f):
    """a block that only continues into a block with no other way in is one block"""
    changed = True
    while changed:
        changed = False
        for aid in list(f.order):
            a = f.blocks.get(aid)
            if a is None or a.term[0] != "goto":
                continue
            bid = a.term[1]
            b = f.blocks.get(bid)
            if b is None or bid == aid or bid == f.entry or b.pred != [aid] or b.retonly or getattr(b, "dead", False):
                continue
            a.stmts = a.stmts + b.stmts
            a.term = b.term
            a.succ = list(b.succ)
            a.exit = b.exit
            for sid in b.succ:
                if sid is not None and sid in f.blocks:
                    pr = f.blocks[sid].pred
                    f.blocks[sid].pred = [aid if x == bid else x for x in pr]
            del f.blocks[bid]
            f.order.remove(bid)
            changed = True


def fold_conditions(f, cnt):
    """a chain of pure tests that share a target (`a && b`, `a || b`) becomes one condition"""
    changed = True
    while changed:
        changed = False
        for b in list(f.blocks.values()):
            if b.term[0] != "if" or getattr(b, "dead", False):
                continue
            c, t, fl = b.term[1], b.term[2], b.term[3]
            for side in ("fall", "taken"):
                g = f.blocks[fl if side == "fall" else t]
                if g is b or g.retonly or g.term[0] != "if" or len(g.pred) != 1 or getattr(g, "dead", False):
                    continue
                if g.stmts:        # a single call whose result is only tested here is part of the condition (short circuit)
                    if len(g.stmts) != 1 or g.stmts[0][0] != "call":
                        continue
                    cid = g.stmts[0][1]
                    if cnt.get((cid, "ax"), 0) != 1 or cnt.get((cid, "dx"), 0) != 0 or not any(x == ("res", cid, "ax") for x in walk(g.term[1])):
                        continue
                    f.force_inline.add(cid)
                    f.folded_calls[cid] = g.stmts[0]
                cg, gt, gf = g.term[1], g.term[2], g.term[3]
                if side == "fall":                # b: if c goto t else g;  g: if cg goto X else Y
                    if gt == t:                   # c || cg -> t, else gf
                        b.term = ("if", ("lor", c, cg), t, gf)
                    elif gf == t:                 # c || !cg -> t, else gt
                        b.term = ("if", ("lor", c, negate_cond(cg)), t, gt)
                    else:
                        continue
                else:                             # b: if c goto g else fl;  g: if cg goto X else Y
                    if gf == fl:                  # c && cg -> gt, else fl
                        b.term = ("if", ("land", c, cg), gt, fl)
                    elif gt == fl:                # c && !cg -> gf, else fl
                        b.term = ("if", ("land", c, negate_cond(cg)), gf, fl)
                    else:
                        continue
                g.dead = True
                b.succ = [b.term[3], b.term[2]]
                for sid in g.succ:
                    if sid is not None:
                        pr = f.blocks[sid].pred
                        if g.id in pr:
                            pr.remove(g.id)
                        if b.id not in pr:
                            pr.append(b.id)
                changed = True
                break


def postdoms(f):
    ids = list(f.blocks)
    succ = {b: [s for s in f.blocks[b].succ if s is not None and not f.blocks[s].retonly] for b in ids}
    for b in ids:
        t = f.blocks[b].term
        if t[0] in ("ret", "tail", "retonly"):
            succ[b] = []
    EXIT = -1
    # iterative dominator algorithm on the reverse graph
    nodes = ids + [EXIT]
    pdom = {n: set(nodes) for n in nodes}
    pdom[EXIT] = {EXIT}
    changed = True
    while changed:
        changed = False
        for n in ids:
            ss = succ[n] or [EXIT]
            new = set.intersection(*[pdom[s] for s in ss]) | {n}
            if new != pdom[n]:
                pdom[n] = new
                changed = True
    ipdom = {}
    for n in ids:
        cands = pdom[n] - {n}
        best = None
        for c in cands:
            if all(d in pdom[c] for d in cands if d != c):
                best = c
        ipdom[n] = best
    return ipdom, succ


class Emitter:
    def __init__(self, f, names, rn):
        self.f, self.names, self.rn = f, names, rn
        self.ipdom, self.succ = postdoms(f)
        self.done = set()
        self.labels = set()
        self.rets_void = False
        self.ended = None
        self.active = []
        self.latch_cond = {}
        self.loop_exit = {}
        self.after_loop = None
        self.ptr_vars = set()
        for b in f.blocks.values():
            for st in b.stmts:
                if st[0] == "call":
                    for a in st[3]:
                        if a[0] == "far" and a[2] == ("dseg",) and a[1][0] in ("phi", "tmp", "in"):
                            self.ptr_vars.add(("phi",) + a[1][1:] if a[1][0] == "in" else a[1])
        self.find_loops()

    # ---- loops
    def find_loops(self):
        f = self.f
        ids = [b for b in f.order if b in f.blocks and not getattr(f.blocks[b], "dead", False)]
        succ = {b: [s for s in self.succ[b] if s in f.blocks] for b in ids}
        pred = {b: [] for b in ids}
        for b in ids:
            for s in succ[b]:
                if s in pred:
                    pred[s].append(b)
        dom = {b: set(ids) for b in ids}
        dom[f.entry] = {f.entry}
        changed = True
        while changed:
            changed = False
            for b in ids:
                if b == f.entry or not pred[b]:
                    continue
                new = set.intersection(*[dom[p] for p in pred[b]]) | {b}
                if new != dom[b]:
                    dom[b] = new
                    changed = True
        self.loop_info = {}
        for u in ids:
            for h in succ[u]:
                if h in dom[u]:                      # back edge u -> h
                    info = self.loop_info.setdefault(h, {"members": {h}, "latches": []})
                    info["latches"].append(u)
                    stack = [u]
                    while stack:
                        n = stack.pop()
                        if n in info["members"]:
                            continue
                        info["members"].add(n)
                        stack.extend(pred.get(n, []))
        for h, info in self.loop_info.items():
            ex = []
            for n in sorted(info["members"]):
                for s in succ.get(n, []):
                    if s not in info["members"] and s not in ex:
                        ex.append(s)
            info["exits"] = ex

    def ind(self, lines):
        return [l if isinstance(l, tuple) else "\t" + l for l in lines]

    def stmts(self, b):
        out = []
        r = self.rn
        for s in b.stmts:
            if s[0] == "set":
                lhs, rhs = s[1], s[2]
                if lhs in self.ptr_vars:
                    out.append("%s = %s;" % (r.R(lhs), r.ptr(rhs)))
                elif lhs[0] == "mem":
                    out.append("%s = %s;" % (r.R(lhs), r.R(rhs)))
                elif rhs[0] == "add" and rhs[1] == lhs and is_k(rhs[2]):
                    out.append("%s += %s;" % (r.R(lhs), r.R(rhs[2])) if rhs[2][1] != 1 else "%s++;" % r.R(lhs))
                elif rhs[0] == "sub" and rhs[1] == lhs and is_k(rhs[2]):
                    out.append("%s -= %s;" % (r.R(lhs), r.R(rhs[2])) if rhs[2][1] != 1 else "%s--;" % r.R(lhs))
                else:
                    out.append("%s = %s;" % (r.R(lhs), r.R(rhs)))
            elif s[0] == "call":
                cid = s[1]
                if cid in r.inline:
                    continue
                txt = r.call_expr(s[2], s[3])
                if cid in r.temps:
                    out.append("r%d_%d = %s;" % (cid[0], cid[1], txt))
                else:
                    out.append(txt + ";")
        return out

    def ret_text(self, ax, dx):
        if self.rets_void or ax is None:
            return "return 0;"
        if getattr(self, "far_return", False) and dx is not None:
            if ax == K(0) and dx == K(0):
                return "return TextRef();"
            p = pack_args([ax, dx])
            if len(p) == 1:
                return "return %s;" % self.rn.R(p[0])
            return "return TextRef::fromWords(%s, %s);" % (self.rn.R(ax), self.rn.R(dx))
        return "return %s;" % self.rn.R(ax)

    # ---- emission
    def leave(self, lines, ended, j):
        """what an arm that stopped at block `ended` needs to say so that control goes there"""
        if ended is None or ended == j:
            return lines
        if self.active and ended == self.active[-1]:
            return lines + ["continue;"]
        if self.active and ended == self.loop_exit.get(self.active[-1]):
            return lines + ["break;"]
        self.labels.add(ended)
        return lines + ["goto L%d;" % ended]

    def emit_loop(self, h, stop, depth):
        f = self.f
        info = self.loop_info[h]
        members = info["members"]
        exits = info["exits"]
        E = None
        if exits:
            ip = self.ipdom.get(h)
            E = ip if ip in exits else exits[0]
        self.active.append(h)
        self.loop_exit[h] = E
        inner = set(stop) | {h} | ({E} if E is not None else set())
        hb = f.blocks[h]
        out = []
        if not hb.stmts and hb.term[0] == "if" and h not in self.done:
            c, tb, fb = hb.term[1], hb.term[2], hb.term[3]
            if (tb in members) != (fb in members) and (fb if tb in members else tb) in exits:
                inside, outside = (tb, fb) if tb in members else (fb, tb)
                self.done.add(h)
                cont = c if inside == tb else negate_cond(c)
                body = self.arm(hb, inside, inner, depth)
                body = self.leave(body, self.ended, None)
                if body and not isinstance(body[-1], tuple) and body[-1] == "continue;":
                    body = body[:-1]
                out.append(("label", h))
                out.append("while (%s) {" % self.rn.cond(cont))
                out += self.ind(body)
                out.append("}")
                self.active.pop()
                self.after_loop = outside
                return out
        # do ... while (c): one latch ending in a test that either repeats or leaves
        latches = info["latches"]
        dowhile = False
        lcond = None
        if len(latches) == 1:
            lb = f.blocks[latches[0]]
            if lb.term[0] == "if" and h in (lb.term[2], lb.term[3]) and (lb.term[2] if lb.term[3] == h else lb.term[3]) in exits:
                c, tb, fb = lb.term[1], lb.term[2], lb.term[3]
                lcond = c if tb == h else negate_cond(c)
                self.latch_cond[latches[0]] = lcond
                dowhile = True
        body = self.emit(h, inner, depth + 1, first=h)
        if not (dowhile and self.ended == h):
            body = self.leave(body, self.ended, None) if self.ended != h else body
        if not dowhile and body and not isinstance(body[-1], tuple) and body[-1] == "continue;":
            body = body[:-1]
        if dowhile:
            out.append("do {")
            out += self.ind(body)
            out.append("} while (%s);" % self.rn.cond(lcond))
        else:
            out.append("for (;;) {")
            out += self.ind(body)
            out.append("}")
        self.active.pop()
        self.after_loop = E
        return out

    def emit(self, bid, stop, depth=0, first=None):
        out = []
        f = self.f
        self.ended = None
        while bid is not None:
            if bid in stop and bid != first:
                self.ended = bid
                return out
            first = None
            b = f.blocks[bid]
            if bid in self.done:
                self.labels.add(bid)
                out.append("goto L%d;" % bid)
                self.ended = None
                return out
            if bid in self.loop_info and bid not in self.active:
                out += self.emit_loop(bid, stop, depth)
                bid = self.after_loop
                continue
            self.done.add(bid)
            out.append(("label", bid))
            out += self.stmts(b)
            if bid in self.latch_cond:
                self.ended = self.active[-1] if self.active else None
                return out
            t = b.term
            k = t[0]
            if k == "goto":
                nb = f.blocks[t[1]]
                if nb.retonly:
                    out.append(self.ret_text(b.exit.regs.get("ax"), b.exit.regs.get("dx")))
                    self.ended = None
                    return out
                bid = t[1]
                continue
            if k == "ret":
                out.append(self.ret_text(t[1], t[2]))
                self.ended = None
                return out
            if k in ("retonly", "end"):
                self.ended = None
                return out
            if k == "tail":
                out.append("/* tail call to %s */ return %s();" % (t[1], self.names.func(t[1])))
                self.ended = None
                return out
            if k == "if":
                cond, tb, fb = t[1], t[2], t[3]
                j = self.ipdom.get(bid)
                if j == -1:
                    j = None
                stop2 = stop | ({j} if j is not None else set())
                then_arm = self.arm(b, fb, stop2, depth)
                then_arm = self.leave(then_arm, self.ended, j)
                else_arm = self.arm(b, tb, stop2, depth)
                else_arm = self.leave(else_arm, self.ended, j)
                c = cond
                if not [l for l in then_arm if not isinstance(l, tuple)]:
                    out += [l for l in then_arm if isinstance(l, tuple)]
                    then_arm = []
                if not [l for l in else_arm if not isinstance(l, tuple)]:
                    out += [l for l in else_arm if isinstance(l, tuple)]
                    else_arm = []
                if not then_arm and else_arm:
                    out.append("if (%s) {" % self.rn.cond(c))
                    out += self.ind(else_arm)
                    out.append("}")
                elif then_arm and not else_arm:
                    out.append("if (%s) {" % self.rn.cond(negate_cond(c)))
                    out += self.ind(then_arm)
                    out.append("}")
                elif not then_arm and not else_arm:
                    pass
                else:
                    ends_t = self.terminates(then_arm)
                    ends_e = self.terminates(else_arm)
                    if ends_t and not ends_e:
                        out.append("if (%s) {" % self.rn.cond(negate_cond(c)))
                        out += self.ind(then_arm)
                        out.append("}")
                        out += else_arm
                    elif ends_e and not ends_t:
                        out.append("if (%s) {" % self.rn.cond(c))
                        out += self.ind(else_arm)
                        out.append("}")
                        out += then_arm
                    else:
                        out.append("if (%s) {" % self.rn.cond(negate_cond(c)))
                        out += self.ind(then_arm)
                        out.append("} else {")
                        out += self.ind(else_arm)
                        out.append("}")
                if j is None:
                    self.ended = None
                    return out
                if j in stop:
                    self.ended = j
                    return out
                bid = j
                continue
            if k == "jtable":
                tname, idx = t[1], t[2]
                targets = self.f.ctx.tables[tname]
                base = 0
                sw = idx
                if idx[0] == "sub" and is_k(idx[2]):
                    sw, base = idx[1], idx[2][1]
                j = self.ipdom.get(bid)
                if j == -1:
                    j = None
                stop2 = stop | ({j} if j is not None else set())
                out.append("switch (%s) {" % self.rn.R(sw))
                seen = {}
                for i, lab in enumerate(targets):
                    tb = f.labels_block[lab]
                    seen.setdefault(tb, []).append(i + base)
                for tb, vals in seen.items():
                    self.ended = None
                    body = self.emit(tb, stop2, depth + 1) if tb not in stop2 else []
                    if tb in stop2:
                        self.ended = tb
                    body = self.leave(body, self.ended, j)
                    out += ["case %d:" % v for v in vals]
                    out += self.ind(body)
                    if not self.terminates(body):
                        out.append("\tbreak;")
                out.append("}")
                if j is None:
                    return out
                if j in stop:
                    self.ended = j
                    return out
                bid = j
                continue
            raise Unsupported("terminator " + k)
        return out

    def arm(self, src, target, stop, depth):
        f = self.f
        tb = f.blocks[target]
        if tb.retonly:
            self.ended = None
            return [self.ret_text(src.exit.regs.get("ax"), src.exit.regs.get("dx"))]
        if target in stop:
            self.ended = target
            return []
        return self.emit(target, stop, depth + 1)

    def terminates(self, lines):
        for l in reversed(lines):
            if isinstance(l, tuple):
                continue
            s = l.strip()
            return s.startswith(("return", "goto", "break", "continue"))
        return False


def render_function(ctx, name, names=None, void=None):
    from decomp_gen import eval_function
    names = names or Names()
    if not hasattr(names, "ctx"):
        names.ctx = ctx
    f = eval_function(ctx, name)
    f.labels_block = {lab: next(b.id for b in f.blocks.values() if b.start == ctx.labels[lab]) for tb in ctx.tables.values() for lab in tb if lab in ctx.labels and any(b.start == ctx.labels[lab] for b in f.blocks.values())}
    prepare(f)
    merge_diamonds(f)
    coalesce(f)
    forward_variables(f)
    repack_calls(f)
    pair_far_sets(f)
    remove_dead_vars(f)
    cnt = count_uses(f)
    f.force_inline, f.folded_calls = set(), {}
    fold_conditions(f, cnt)
    # decide which call results are inlined into the statement that uses them, which get a temporary
    inline, temps = set(f.force_inline), set()
    calls = {cid: (st[2], st[3]) for cid, st in f.folded_calls.items()}
    for bid in f.order:
        b = f.blocks[bid]
        if getattr(b, "dead", False):
            continue
        for i, s in enumerate(b.stmts):
            if s[0] != "call":
                continue
            calls[s[1]] = (s[2], s[3])
        for i, s in enumerate(b.stmts):
            if s[0] != "call":
                continue
            if s[1] in inline:
                continue
            n = cnt.get((s[1], "ax"), 0) + cnt.get((s[1], "dx"), 0)
            if n == 0:
                continue
            me = ("res", s[1], "ax")
            ok = False
            if n == 1 and cnt.get((s[1], "dx"), 0) == 0:
                pure_me = bool(Renderer.PURE.match(str(s[2])))
                k = i + 1
                while True:
                    if k < len(b.stmts):
                        st = b.stmts[k]
                        es = [st[2], st[1]] if st[0] == "set" else list(st[3])
                        if any(x == me for e in es for x in walk(e)):
                            nres = sum(1 for e in es for x in walk(e) if x[0] == "res" and x[1] not in inline)
                            ok = nres == 1 or pure_me
                            break
                        if st[0] == "call" and pure_me and (Renderer.PURE.match(str(st[2])) or st[1] in inline):
                            k += 1
                            continue
                        break
                    else:
                        es = exprs_of(Block_(b.term)) if b.term[0] in ("if", "ret", "jtable") else []
                        if any(x == me for e in es for x in walk(e)):
                            nres = sum(1 for e in es for x in walk(e) if x[0] == "res" and x[1] not in inline)
                            ok = nres == 1 or pure_me
                        break
            (inline if ok else temps).add(s[1])
    rn = Renderer(f, names, inline, temps)
    rn.calls = calls
    em = Emitter(f, names, rn)
    em.far_return = names.returns_far(name)
    rets = [b.term[1] for b in f.blocks.values() if b.term and b.term[0] == "ret" and not getattr(b, "dead", False)]
    defined = any(r is not None and r[0] not in ("res", "in", "undef") for r in rets)
    em.rets_void = void if void is not None else (not defined and not em.far_return and not re.match(r"^(objh_|exit_fn_|event_fn_|conv_cb_|conv_text_|cmd[A-Z])", name) and ctx.ret_used.get(name, 0) == 0)
    body = em.emit(f.entry, set())
    return f, em, rn, body, temps


def flatten(lines, labels):
    """label markers become `Lnn:` where something jumps to them"""
    out = []
    for l in lines:
        if isinstance(l, tuple):
            if l[1] in labels:
                out.append("L%d:;" % l[1])
        else:
            out.append(l)
    return out


def function_text(ctx, name, names=None, void=None):
    names = names or Names()
    f, em, rn, body, temps = render_function(ctx, name, names, void)
    lines = flatten(body, em.labels)
    decl = sorted("r%d_%d" % c for c in temps)
    tmps = sorted({x[1] for b in f.blocks.values() for e in exprs_of(b) for x in walk(e) if x[0] == "tmp"} | {s[1][1] for b in f.blocks.values() for s in b.stmts if s[0] == "set" and s[1][0] == "tmp"})
    phis = sorted({"p%d_%s" % (x[2], x[1]) for b in f.blocks.values() for e in exprs_of(b) for x in walk(e) if x[0] == "phi"})
    params = [names.loc(name, n, 0, 2) for _, n in sorted((v[1], n) for n, v in f.locs.items() if n.startswith("arg_"))]
    used = [int(m) for m in re.findall(r"\barg(\d)\b", chr(10).join(lines))]
    want = max(max(used) + 1 if used else 0, len(params), names.min_params(name))
    if want > len(params):
        params = ["arg%d" % i for i in range(want)]
    TAB = chr(9)
    head = ["// " + name, names.header(name, params) + " {"] + [TAB + l for l in names.prolog()]
    vars_ = decl + ["t%d" % t for t in tmps] + phis
    text = chr(10).join(lines)
    named = {names.loc(name, n, 0, 2) for n in f.locs if not n.startswith(("arg_", "var_"))}
    locs = sorted(set(re.findall(r"\bv[0-9A-F]+(?:_hi)?\b", text)) | {n for n in named | {"si", "di"} if re.search(r"\b%s\b" % re.escape(n), text)})
    locs = [n for n in locs if not n.endswith("_hi")] + [n for n in locs if n.endswith("_hi")]
    allv = locs + vars_
    ptr_src = r'("|ES\(|g\.print_obj\(|g\.ResourceText_GetStr\(|_strlwr\(|_strupr\(|nullptr|number_to_words)'
    strs = {n for n in allv if re.search(r'(^|\s)%s = \(*([^;?]*\? )?' % re.escape(n) + ptr_src, text)}
    strs |= {n for n in allv if re.search(r'(strcpy|strcat)\(%s,' % re.escape(n), text)}
    copies = [(m.group(1), m.group(2)) for m in re.finditer(r'(?:^|\s)(' + "|".join(re.escape(n) for n in allv) + r') = ('  + "|".join(re.escape(n) for n in allv) + r');', text)] if allv else []
    changed = True
    while changed:
        changed = False
        for a, b2 in copies:
            if (a in strs) != (b2 in strs):
                strs |= {a, b2}
                changed = True
    txts = {n for n in allv if re.search(r'(^|\s)%s = .*TextRef\(' % re.escape(n), text)}
    strs -= txts
    ints = [n for n in allv if n not in strs and n not in txts]
    # assignments between the two kinds of variable need a cast
    ptr_like = re.compile(r"^(ES\(|g\.print_obj\(|g\.ResourceText_GetStr\(|_strlwr\(|_strupr\(|nullptr|" + "|".join(re.escape(n) for n in sorted(strs)) + r")") if strs else re.compile(r"^(ES\(|g\.print_obj\(|nullptr)")
    fixed = []
    for ln in lines:
        m = re.match(r"^(\s*)(\w+) = (.*);$", ln)
        if m and m.group(2) in allv and "?" not in m.group(3):
            rhs = m.group(3)
            is_ptr = bool(ptr_like.match(rhs))
            if m.group(2) not in strs and is_ptr:
                ln = "%s%s = (intptr_t)(%s);" % (m.group(1), m.group(2), rhs)
        fixed.append(ln)
    lines = fixed
    if ints:
        head.append(TAB + "intptr_t " + ", ".join(ints) + ";")
    if strs:
        head.append(TAB + "char *" + ", *".join(sorted(strs)) + ";")
    if txts:
        head.append(TAB + "TextRef " + ", ".join(sorted(txts)) + ";")
    return chr(10).join(head + [TAB + l for l in lines] + ["}"])
