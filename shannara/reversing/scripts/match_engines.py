"""
Matches functions between two builds of the Legend engine whose code differs completely (retail Shannara: 32-bit Watcom;
Companions of Xanth: 16-bit Microsoft C), where instruction-level BinDiff matching does not work.  Uses what survives a
recompilation: referenced string literals, rare constants and the call graph.

  1. seeds: functions referencing the same string(s) in both binaries (unique-to-unique, majority vote when a function holds several)
  2. propagation: align the ordered call lists of every matched pair; callees that fall between already aligned anchors are paired
     when the gap has the same length on both sides, and callers are paired when both sides have exactly one unmatched caller
  3. every pair keeps the evidence that produced it ("via") and a confidence tier

    python match_engines.py sfeat.json xfeat.json out.json [--holdout 0.3]
(feature files come from extract_func_features.py)
"""
import collections
import difflib
import json
import random
import sys


def load(p):
    F = {f["ea"]: f for f in json.load(open(p))}
    resolve_stubs(F)
    return F


def resolve_stubs(F):
    """RTLink overlay calls go through tiny stubs (`call rtlink_thunk; jmp target`): replace each stub in every call list by its target"""
    stubs = {ea: f["tail"] for ea, f in F.items() if f.get("tail") and f["size"] <= 16}
    if not stubs:
        return
    def res(c, depth=0):
        while c in stubs and depth < 4:
            c = stubs[c]
            depth += 1
        return c
    for f in F.values():
        f["calls"] = [res(c) for c in f["calls"]]
    callers = {}
    for ea, f in F.items():
        for c in set(f["calls"]):
            callers.setdefault(c, set()).add(ea)
    for ea, f in F.items():
        f["n_callers"] = len(callers.get(ea, ()))
    for ea in stubs:
        F[ea]["name"] = F[ea]["name"]  # keep; stubs stay in the table but are no longer referenced


LIB_PREFIX = ("_", "__", "j_", "nullsub", "unknown_libname")


def is_game(f):
    n = f["name"]
    return n.startswith("sub_") or not n.startswith(LIB_PREFIX)


def seeds(S, X, holdout=0.0, rng=None):
    si, xi = collections.defaultdict(set), collections.defaultdict(set)
    for f in S.values():
        for s in f["strings"]:
            si[s].add(f["ea"])
    for f in X.values():
        for s in f["strings"]:
            xi[s].add(f["ea"])
    votes = collections.defaultdict(collections.Counter)
    used = []
    for s in si:
        if s in xi and len(si[s]) == 1 and len(xi[s]) == 1 and len(s) >= 6:
            a, b = next(iter(si[s])), next(iter(xi[s]))
            used.append((s, a, b))
    held = set()
    if holdout:
        rng = rng or random.Random(1)
        rng.shuffle(used)
        k = int(len(used) * holdout)
        held = set(u[0] for u in used[:k])
    for s, a, b in used:
        if s in held:
            continue
        votes[a][b] += 1
    pairs = {}
    for a, c in votes.items():
        b, n = c.most_common(1)[0]
        if len(c) == 1 or n > sum(c.values()) - n:
            pairs[a] = (b, "string")
    # drop conflicts (two S functions claiming the same X function)
    inv = collections.defaultdict(list)
    for a, (b, _) in pairs.items():
        inv[b].append(a)
    for b, aa in inv.items():
        if len(aa) > 1:
            for a in aa:
                pairs.pop(a, None)
    return pairs, [u for u in used if u[0] in held]


def uniq(seq):
    seen, out = set(), []
    for x in seq:
        if x not in seen:
            seen.add(x)
            out.append(x)
    return out


def propagate(S, X, pairs, rounds=12):
    inv = {b: a for a, (b, _) in pairs.items()}
    s_callers = collections.defaultdict(set)
    x_callers = collections.defaultdict(set)
    for f in S.values():
        for c in set(f["calls"]):
            s_callers[c].add(f["ea"])
    for f in X.values():
        for c in set(f["calls"]):
            x_callers[c].add(f["ea"])

    def ok_size(a, b):
        sa, sb = S[a]["size"], X[b]["size"]
        return 0.3 <= (sa + 8) / (sb + 8) <= 3.5

    def add(a, b, via):
        if a in pairs or b in inv or a not in S or b not in X:
            return False
        if not is_game(S[a]) or not is_game(X[b]):
            return False
        if not ok_size(a, b):
            return False
        pairs[a] = (b, via)
        inv[b] = a
        return True

    for rnd in range(rounds):
        added = 0
        for a, (b, via) in list(pairs.items()):
            ca, cb = uniq(S[a]["calls"]), uniq(X[b]["calls"])
            ka = [pairs[c][0] if c in pairs else None for c in ca]
            # align by anchored partners
            sm = difflib.SequenceMatcher(None, [k if k is not None else ("?", i) for i, k in enumerate(ka)], cb, autojunk=False)
            for tag, i1, i2, j1, j2 in sm.get_opcodes():
                if tag in ("replace", "delete", "insert") and (i2 - i1) == (j2 - j1) and i2 > i1:
                    for da, db in zip(ca[i1:i2], cb[j1:j2]):
                        if ka[ca.index(da)] is None:
                            added += add(da, db, "callee-of:%x" % a)
            # whole-list equal length shortcut
            if len(ca) == len(cb):
                for da, db in zip(ca, cb):
                    if da not in pairs and db not in inv:
                        added += add(da, db, "callee-order:%x" % a)
            # unique callers
            ua = [c for c in s_callers[a] if c not in pairs]
            ub = [c for c in x_callers[b] if c not in inv]
            if len(s_callers[a]) <= 3 and len(x_callers[b]) <= 3 and len(ua) == 1 and len(ub) == 1:
                added += add(ua[0], ub[0], "sole-caller-of:%x" % a)
        if not added:
            break
    return pairs


def chain(S, X, pairs):
    """longest chain of pairs that is increasing in both addresses (the two builds link the shared source files in the same order within a block)"""
    items = sorted((a, b) for a, (b, _) in pairs.items())
    n = len(items)
    best = [1] * n
    prev = [-1] * n
    for i in range(n):
        for j in range(i):
            if items[j][1] < items[i][1] and best[j] + 1 > best[i]:
                best[i], prev[i] = best[j] + 1, j
    if not n:
        return []
    k = max(range(n), key=lambda i: best[i])
    out = []
    while k != -1:
        out.append(items[k])
        k = prev[k]
    return out[::-1]


def jac(a, b):
    if not a or not b:
        return 0.0
    return len(a & b) / float(len(a | b))


def norm(n):
    return n.strip("_").lower().rstrip("?")


def add_keys(F):
    """derived features: large constants and normalised names of called library functions (Watcom 'memmove_' vs Microsoft '_memmove')"""
    for f in F.values():
        f["_k"] = set(c for c in f["consts"] if c >= 0x100)
        f["_l"] = set(norm(F[c]["name"]) for c in f["calls"] if c in F and not F[c]["name"].startswith(("sub_", "nullsub", "loc_")))


def fill_gaps(S, X, pairs, max_gap=14):
    """between consecutive chained anchors, align the unmatched functions of both builds by order and size"""
    import math
    ch = chain(S, X, pairs)
    s_sorted = sorted(a for a in S if is_game(S[a]))
    x_sorted = sorted(b for b in X if is_game(X[b]))
    inv = {b: a for a, (b, _) in pairs.items()}
    added = 0
    for (a1, b1), (a2, b2) in zip(ch, ch[1:]):
        sg = [a for a in s_sorted if a1 < a < a2 and a not in pairs]
        xg = [b for b in x_sorted if b1 < b < b2 and b not in inv]
        if not sg or not xg or len(sg) > max_gap or len(xg) > max_gap:
            continue
        # Needleman-Wunsch on size similarity
        n, m = len(sg), len(xg)
        GAP = 1.2
        sc = [[0.0] * (m + 1) for _ in range(n + 1)]
        bt = [[None] * (m + 1) for _ in range(n + 1)]
        for i in range(1, n + 1):
            sc[i][0] = -GAP * i
            bt[i][0] = "u"
        for j in range(1, m + 1):
            sc[0][j] = -GAP * j
            bt[0][j] = "l"
        def sim(a, b):
            r = (S[a]["size"] + 6) / (X[b]["size"] + 6)
            v = 1.0 - abs(math.log(r)) * 1.3
            v += 1.2 * jac(S[a].get("_k"), X[b].get("_k"))
            v += 0.8 * jac(S[a].get("_l"), X[b].get("_l"))
            return v
        for i in range(1, n + 1):
            for j in range(1, m + 1):
                d = sc[i - 1][j - 1] + sim(sg[i - 1], xg[j - 1])
                u = sc[i - 1][j] - GAP
                l = sc[i][j - 1] - GAP
                best = max(d, u, l)
                sc[i][j] = best
                bt[i][j] = "d" if best == d else ("u" if best == u else "l")
        i, j = n, m
        al = []
        while i > 0 or j > 0:
            t = bt[i][j]
            if t == "d":
                al.append((sg[i - 1], xg[j - 1]))
                i -= 1
                j -= 1
            elif t == "u":
                i -= 1
            else:
                j -= 1
        for a, b in al:
            r = (S[a]["size"] + 6) / (X[b]["size"] + 6)
            if 0.5 <= r <= 2.0 and a not in pairs and b not in inv:
                pairs[a] = (b, "order-gap")
                inv[b] = a
                added += 1
    return added


def vote_data(S, X, pairs):
    """pair global variables by position in the ordered data-reference lists of matched functions"""
    votes = collections.defaultdict(collections.Counter)
    for a, (b, _) in pairs.items():
        da, db = S[a]["data"], X[b]["data"]
        if len(da) == len(db) and da:
            w = 1 if len(da) < 3 else 2
            for x, y in zip(da, db):
                votes[x][y] += w
    inv = collections.defaultdict(list)
    for x, c in votes.items():
        y, n = c.most_common(1)[0]
        second = c.most_common(2)[1][1] if len(c) > 1 else 0
        if n >= 2 and n > second:
            inv[y].append((x, n))
    out = {}
    for y, lst in inv.items():
        lst.sort(key=lambda t: -t[1])
        if len(lst) == 1 or lst[0][1] > lst[1][1]:
            out[lst[0][0]] = (y, lst[0][1])
    return out


def data_anchors(S, X, pairs):
    """a matched global used by exactly one not-yet-matched function on each side pairs those two functions"""
    dp = vote_data(S, X, pairs)
    s_users, x_users = collections.defaultdict(set), collections.defaultdict(set)
    for f in S.values():
        for d in f["data"]:
            s_users[d].add(f["ea"])
    for f in X.values():
        for d in f["data"]:
            x_users[d].add(f["ea"])
    inv = {b: a for a, (b, _) in pairs.items()}
    added = 0
    for x, (y, n) in dp.items():
        ua = [a for a in s_users[x] if a not in pairs and is_game(S[a])]
        ub = [b for b in x_users[y] if b not in inv and is_game(X[b])]
        if len(ua) == 1 and len(ub) == 1 and 0.4 <= (S[ua[0]]["size"] + 8) / (X[ub[0]]["size"] + 8) <= 2.5:
            pairs[ua[0]] = (ub[0], "data-anchor:%x" % x)
            inv[ub[0]] = ua[0]
            added += 1
    return added


COOC_VOTES, COOC_DICE = 3, 0.6


def cooc_round(S, X, pairs, min_votes=None, min_dice=None):
    min_votes = COOC_VOTES if min_votes is None else min_votes
    min_dice = COOC_DICE if min_dice is None else min_dice
    """co-occurrence voting: a function pair is supported by every matched caller pair (a,b) that call c and x respectively, and by every matched
    callee pair (q,r) that are called by c and x respectively.  Accept mutually best candidates with enough support."""
    inv = {b: a for a, (b, _) in pairs.items()}
    s_calls = {a: set(S[a]["calls"]) for a in S}
    x_calls = {b: set(X[b]["calls"]) for b in X}
    s_callers = collections.defaultdict(set)
    x_callers = collections.defaultdict(set)
    for a, cs in s_calls.items():
        for c in cs:
            s_callers[c].add(a)
    for b, cs in x_calls.items():
        for c in cs:
            x_callers[c].add(b)
    votes = collections.defaultdict(collections.Counter)
    for a, (b, _) in pairs.items():
        for c in s_calls[a]:
            for x in x_calls[b]:
                votes[c][x] += 1
        for c in s_callers[a] if False else ():
            pass
    # callee-side evidence: matched callees (q,r) vote for their callers
    for q, (r, _) in pairs.items():
        for c in s_callers[q]:
            for x in x_callers[r]:
                votes[c][x] += 1
    cand = []
    for c, cnt in votes.items():
        if c in pairs or not is_game(S[c]):
            continue
        nc = len(set(a for a in s_callers[c] if a in pairs)) + len(set(q for q in s_calls[c] if q in pairs))
        for x, v in cnt.items():
            if x in inv or not is_game(X[x]) or v < min_votes:
                continue
            nx = len(set(b for b in x_callers[x] if b in inv)) + len(set(r for r in x_calls[x] if r in inv))
            dice = 2.0 * v / max(1, nc + nx)
            r = (S[c]["size"] + 8) / (X[x]["size"] + 8)
            if dice >= min_dice and 0.35 <= r <= 2.8:
                cand.append((dice, v, c, x))
    cand.sort(reverse=True)
    taken_s, taken_x, added = set(), set(), 0
    for dice, v, c, x in cand:
        if c in taken_s or x in taken_x:
            continue
        # mutual best: no better competing candidate for either side was skipped (greedy order guarantees this)
        pairs[c] = (x, "cooc:%d/%.2f" % (v, dice))
        inv[x] = c
        taken_s.add(c)
        taken_x.add(x)
        added += 1
    return added


def support(S, X, pairs):
    """for every pair: (votes, dice) from co-occurring matched callers and callees; also returns the vote tables for alternatives"""
    inv = {b: a for a, (b, _) in pairs.items()}
    s_calls = {a: set(S[a]["calls"]) for a in S}
    x_calls = {b: set(X[b]["calls"]) for b in X}
    s_callers, x_callers = collections.defaultdict(set), collections.defaultdict(set)
    for a, cs in s_calls.items():
        for c in cs:
            s_callers[c].add(a)
    for b, cs in x_calls.items():
        for c in cs:
            x_callers[c].add(b)
    votes = collections.defaultdict(collections.Counter)
    for a, (b, _) in pairs.items():
        for c in s_calls[a]:
            for x in x_calls[b]:
                votes[c][x] += 1
    for q, (r, _) in pairs.items():
        for c in s_callers[q]:
            for x in x_callers[r]:
                votes[c][x] += 1
    def mdeg_s(c):
        return len(set(a for a in s_callers[c] if a in pairs)) + len(set(q for q in s_calls[c] if q in pairs))
    def mdeg_x(x):
        return len(set(b for b in x_callers[x] if b in inv)) + len(set(r for r in x_calls[x] if r in inv))
    return votes, mdeg_s, mdeg_x


def refine(S, X, pairs, rounds=4):
    """replace a pair when a clearly better-supported alternative exists for its Shannara function"""
    changed = 0
    for _ in range(rounds):
        votes, ds, dx = support(S, X, pairs)
        inv = {b: a for a, (b, _) in pairs.items()}
        swaps = []
        for a, (b, via) in list(pairs.items()):
            if via == "string":
                continue
            cnt = votes.get(a, {})
            v0 = cnt.get(b, 0)
            d0 = 2.0 * v0 / max(1, ds(a) + dx(b))
            best = None
            for x, v in cnt.items():
                if x == b or v < 3 or not is_game(X[x]):
                    continue
                d = 2.0 * v / max(1, ds(a) + dx(x))
                r = (S[a]["size"] + 8) / (X[x]["size"] + 8)
                if d >= 0.8 and d > d0 + 0.3 and 0.35 <= r <= 2.8 and (best is None or d > best[0]):
                    best = (d, v, x)
            if best:
                swaps.append((a, b, best))
        done = 0
        for a, b, (d, v, x) in sorted(swaps, key=lambda t: -t[2][0]):
            holder = inv.get(x)
            if holder is not None and holder != a:
                if pairs[holder][1] == "string":
                    continue
                del pairs[holder]
                del inv[x]
            if inv.get(b) == a:
                del inv[b]
            pairs[a] = (x, "refined:%d/%.2f" % (v, d))
            inv[x] = a
            done += 1
        changed += done
        if not done:
            break
    return changed


def sig_round(S, X, pairs, replace=True):
    """structural signature: the set of callees (mapped through the current pairs) plus the set of mapped data references.  A function whose
    callees/data are all matched is identified when exactly one function on the other side has the identical signature."""
    dp = vote_data(S, X, pairs)
    dmap = {x: y for x, (y, n) in dp.items()}
    inv = {b: a for a, (b, _) in pairs.items()}

    def ssig(a):
        f = S[a]
        calls = set()
        for c in f["calls"]:
            if c in pairs:
                calls.add(pairs[c][0])
            elif c != a:
                return None
        data = set()
        for d in f["data"]:
            if d in dmap:
                data.add(dmap[d])
            else:
                return None
        if not calls and not data:
            return None
        return (frozenset(calls), frozenset(data))

    def xsig(b):
        f = X[b]
        return (frozenset(c for c in f["calls"] if c != b), frozenset(f["data"]))

    xs = collections.defaultdict(list)
    for b in X:
        if is_game(X[b]):
            sg = xsig(b)
            if sg[0] or sg[1]:
                xs[sg].append(b)
    ss = collections.defaultdict(list)
    for a in S:
        if is_game(S[a]):
            sg = ssig(a)
            if sg:
                ss[sg].append(a)
    added = 0
    import math
    for sg, al in ss.items():
        bl = xs.get(sg, [])
        if not bl or len(al) > 4 or len(bl) > 4:
            continue
        cands = []
        if len(al) == len(bl):                       # same count: the two builds link the shared sources in the same order
            for a, b in zip(sorted(al), sorted(bl)):
                r = (S[a]["size"] + 8) / (X[b]["size"] + 8)
                if 0.4 <= r <= 2.5:
                    cands.append((0.0, a, b))
        else:
            for a in al:
                for b in bl:
                    r = (S[a]["size"] + 8) / (X[b]["size"] + 8)
                    if 0.5 <= r <= 2.0:
                        cands.append((abs(math.log(r)), a, b))
        cands.sort()
        ua, ub = set(), set()
        for d, a, b in cands:
            if a in ua or b in ub:
                continue
            # when the group is ambiguous require a clear size winner for this pair
            if (len(al) > 1 or len(bl) > 1):
                rivals = [c for c in cands if (c[1] == a) != (c[2] == b) and (c[1] == a or c[2] == b)]
                if rivals and min(c[0] for c in rivals) < d + 0.15:
                    continue
            ua.add(a)
            ub.add(b)
            cur = pairs.get(a)
            if cur and cur[0] == b:
                continue
            if cur:
                if not replace or cur[1] == "string":
                    continue
                inv.pop(cur[0], None)
            holder = inv.get(b)
            if holder is not None and holder != a:
                if pairs[holder][1] == "string":
                    continue
                del pairs[holder]
            pairs[a] = (b, "signature")
            inv[b] = a
            added += 1
    return added


def main():
    S, X = load(sys.argv[1]), load(sys.argv[2])
    add_keys(S)
    add_keys(X)
    out = sys.argv[3]
    holdout = float(sys.argv[sys.argv.index("--holdout") + 1]) if "--holdout" in sys.argv else 0.0
    global COOC_VOTES, COOC_DICE
    if "--cooc" in sys.argv:
        i = sys.argv.index("--cooc")
        COOC_VOTES, COOC_DICE = int(sys.argv[i + 1]), float(sys.argv[i + 2])
    pairs, held = seeds(S, X, holdout)
    n0 = len(pairs)
    for _ in range(8):
        if not cooc_round(S, X, pairs):
            break
    pairs = propagate(S, X, pairs)
    for _ in range(5):
        g = fill_gaps(S, X, pairs)
        g += data_anchors(S, X, pairs)
        g += cooc_round(S, X, pairs)
        g += refine(S, X, pairs)
        g += sig_round(S, X, pairs)
        pairs = propagate(S, X, pairs)
        if not g:
            break
    print("seed pairs", n0, "after propagation", len(pairs))
    if holdout:
        # evaluate on the held-out string pairs: a pair is "confirmed" if propagation matched the S function to the right X function
        right = wrong = missing = 0
        for s, a, b in held:
            if a in pairs:
                if pairs[a][0] == b:
                    right += 1
                else:
                    wrong += 1
            else:
                missing += 1
        print("held-out string pairs: correct %d, wrong %d, not matched %d" % (right, wrong, missing))
    refine(S, X, pairs)
    votes, ds, dx = support(S, X, pairs)
    json.dump([dict(votes=votes.get(a, {}).get(b, 0), dice=round(2.0 * votes.get(a, {}).get(b, 0) / max(1, ds(a) + dx(b)), 2), s=a, x=b, via=v, s_name=S[a]["name"], x_name=X[b]["name"], s_size=S[a]["size"], x_size=X[b]["size"]) for a, (b, v) in sorted(pairs.items())], open(out, "w"), indent=0)


if __name__ == "__main__":
    main()
