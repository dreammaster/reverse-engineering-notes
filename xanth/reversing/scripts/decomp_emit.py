"""Statement preparation and structured emission for decomp.py."""
import re

from decomp import K, is_k, simp, negate_cond, walk, Unsupported
from decomp_gen import exprs_of, count_uses
from decomp_out import Renderer, Names, CMPOP


# --------------------------------------------------------------------------------------------------------------------------------- argument packing
def pack_args(args):
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

    def ind(self, lines):
        return [l if isinstance(l, tuple) else "\t" + l for l in lines]

    def stmts(self, b):
        out = []
        r = self.rn
        for s in b.stmts:
            if s[0] == "set":
                lhs, rhs = s[1], s[2]
                if lhs[0] in ("loc", "glob", "tmp", "phi", "mem") or True:
                    if lhs[0] == "mem":
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
            return "return;" if self.rets_void else "return 0;"
        return "return %s;" % self.rn.R(ax)

    def emit(self, bid, stop, depth=0):
        out = []
        f = self.f
        while bid is not None and bid not in stop:
            b = f.blocks[bid]
            if bid in self.done:
                self.labels.add(bid)
                out.append("goto L%d;" % bid)
                return out
            self.done.add(bid)
            if bid in self.labels:
                pass
            out.append(("label", bid))
            out += self.stmts(b)
            t = b.term
            k = t[0]
            if k == "goto":
                nb = f.blocks[t[1]]
                if nb.retonly:
                    out.append(self.ret_text(b.exit.regs.get("ax"), b.exit.regs.get("dx")))
                    return out
                bid = t[1]
                continue
            if k == "ret":
                out.append(self.ret_text(t[1], t[2]))
                return out
            if k == "retonly":
                return out
            if k == "tail":
                out.append("/* tail call to %s */ return %s();" % (t[1], self.names.func(t[1])))
                return out
            if k == "end":
                return out
            if k == "if":
                cond, tb, fb = t[1], t[2], t[3]
                j = self.ipdom.get(bid)
                stop2 = stop | ({j} if j is not None and j != -1 else set())
                then_arm = self.arm(b, fb, stop2, depth)
                else_arm = self.arm(b, tb, stop2, depth)
                c = cond
                if not [l for l in then_arm if not isinstance(l, tuple)]:
                    out += [l for l in then_arm if isinstance(l, tuple)]
                    then_arm = []
                if not [l for l in else_arm if not isinstance(l, tuple)]:
                    out += [l for l in else_arm if isinstance(l, tuple)]
                    else_arm = []
                # prefer the shorter arm first, and put an arm that ends in a return first so the rest can follow without nesting
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
                if j is None or j == -1 or j in stop:
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
                stop2 = stop | ({j} if j is not None and j != -1 else set())
                out.append("switch (%s) {" % self.rn.R(sw))
                seen = {}
                for i, lab in enumerate(targets):
                    tb = f.labels_block[lab]
                    seen.setdefault(tb, []).append(i + base)
                for tb, vals in seen.items():
                    body = self.emit(tb, stop2, depth + 1) if tb not in stop2 else []
                    lines = ["case %s:" % self.rn.names.case(sw, v) if hasattr(self.rn.names, "case") else "case %d:" % v for v in vals]
                    out += lines
                    out += self.ind(body)
                    if not self.terminates(body):
                        out.append("\tbreak;")
                out.append("}")
                if j is None or j == -1 or j in stop:
                    return out
                bid = j
                continue
            raise Unsupported("terminator " + k)
        return out

    def arm(self, src, target, stop, depth):
        f = self.f
        tb = f.blocks[target]
        if tb.retonly:
            return [self.ret_text(src.exit.regs.get("ax"), src.exit.regs.get("dx"))]
        if target in stop:
            return []
        return self.emit(target, stop, depth + 1)

    def terminates(self, lines):
        for l in reversed(lines):
            if isinstance(l, tuple):
                continue
            s = l.strip()
            return s.startswith(("return", "goto", "break", "continue")) or s.endswith("*/ return " + "")
        return False


def render_function(ctx, name, names=None, void=None):
    from decomp_gen import eval_function
    names = names or Names()
    if not hasattr(names, "ctx"):
        names.ctx = ctx
    f = eval_function(ctx, name)
    f.labels_block = {lab: next(b.id for b in f.blocks.values() if b.start == ctx.labels[lab]) for tb in ctx.tables.values() for lab in tb if lab in ctx.labels and any(b.start == ctx.labels[lab] for b in f.blocks.values())}
    prepare(f)
    coalesce(f)
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
    em.rets_void = void if void is not None else (not re.match(r"^(objh_|exit_fn_|event_fn_|conv_cb_|conv_text_|cmd[A-Z])", name) and ctx.ret_used.get(name, 0) == 0)
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
    params = sorted((v[1], n) for n, v in f.locs.items() if n.startswith("arg_"))
    head = ["// %s" % name, "int %s(%s) {" % (names.func(name) if names else name, ", ".join("int " + n for _, n in params))]
    vars_ = decl + ["t%d" % t for t in tmps] + phis
    locs = sorted(n for n in f.locs if n.startswith("var_"))
    if vars_ or locs:
        head.append("\tint %s;" % ", ".join(locs + vars_))
    return "\n".join(head + ["\t" + l for l in lines] + ["}"])
