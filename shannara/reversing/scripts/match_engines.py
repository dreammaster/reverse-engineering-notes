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
    return F


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


def main():
    S, X = load(sys.argv[1]), load(sys.argv[2])
    add_keys(S)
    add_keys(X)
    out = sys.argv[3]
    holdout = float(sys.argv[sys.argv.index("--holdout") + 1]) if "--holdout" in sys.argv else 0.0
    pairs, held = seeds(S, X, holdout)
    n0 = len(pairs)
    pairs = propagate(S, X, pairs)
    for _ in range(5):
        g = fill_gaps(S, X, pairs)
        g += data_anchors(S, X, pairs)
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
    json.dump([dict(s=a, x=b, via=v, s_name=S[a]["name"], x_name=X[b]["name"], s_size=S[a]["size"], x_size=X[b]["size"]) for a, (b, v) in sorted(pairs.items())], open(out, "w"), indent=0)


if __name__ == "__main__":
    main()
