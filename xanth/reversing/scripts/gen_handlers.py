"""
Prototype translator: analysis/xanth_handler_digest.txt (symbolic path tries of the compiled object handlers, exit functions and conversation
callbacks) -> C++ in ../../engine/gen/.

    python gen_handlers.py            # writes handlers_gen.cpp, handlers_api.h, handlers_todo.txt

Only handlers whose digest uses nothing but the regular constructs are translated (no stack-frame locals `[bp+..]`, `[bx]`, `es:`, loops, tail
jumps).  The translation keeps the digest's own names: unresolved `sub_XXXX` calls and every global variable are declared in handlers_api.h so
that the host (the future engine) has to provide them; `?` in a name becomes `_q`.  Whether it is faithful is checked structurally (g++ -fsyntax-only).
"""
import csv
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
A = os.path.join(HERE, "..", "analysis")
OUT = os.path.join(HERE, "..", "..", "engine", "gen")
os.makedirs(OUT, exist_ok=True)

IRREG = re.compile(r"\[bp\+\w+[+-]|\[bx|\[[a-z]{2}\]|\bloop\b|\bes:|\bds =|\bcs:|\bds:|\bss:|\bcall\b")
PARAMS = {"phase", "cmd", "dobj", "iobj", "arg_2", "arg_4"}
KEYWORDS = {"if", "int", "return", "msg", "MSG", "other"}

enums = [r for r in csv.DictReader(open(os.path.join(A, "xanth_enums.tsv")), delimiter="\t")]
cmds = {r["name"][4:] if r["name"].startswith("CMD_") else r["name"]: int(r["value"]) for r in enums if r["enum"] == "CMD"}   # the digest prints verbs without the CMD_ prefix


def load():
    fn, order, cur = {}, [], None
    for l in open(os.path.join(A, "xanth_handler_digest.txt"), encoding="utf-8").read().splitlines():
        if l and not l.startswith(" "):
            cur = l.split()[0]
            fn[cur] = (l.split("# objects:", 1)[1].strip() if "# objects:" in l else "", [])
            order.append(cur)
        elif cur and l.strip():
            fn[cur][1].append(l)
    return fn, order


def ident(s):
    return s.replace("?", "_q")


LOCALS = set()   # stack-frame locals of the function being translated
idents = {}   # name -> kind ('var' or 'func')


def expr(e):
    """translate an expression of the digest to C++, registering identifiers"""
    e = e.strip().replace("switch==", "switch_index==")
    e = re.sub(r"\[bp\+(\w+)\]", r"\1", e)
    e = re.sub(r"([\w?]+)==\{([0-9,]+)\}", lambda m: "(" + " || ".join("%s==%s" % (m.group(1), v) for v in m.group(2).split(",")) + ")", e)
    e = re.sub(r"msg\((\w+):", r"MSG(\1, ", e)
    e = re.sub(r"\bseg (\w+)", r"SEG_\1", e)
    e = re.sub(r"&([A-Za-z_][A-Za-z_0-9?]*)", r"ADDR(\1)", e)
    e = re.sub(r"(?<![\w?])\?(?![\w?])", "UNKNOWN_ARG", e)

    def rep(m):
        t = m.group(0)
        if re.fullmatch(r"0x[0-9A-Fa-f]+|\d+", t):
            return t
        if re.match(r"(var|arg)_\w+$", t) and t not in PARAMS:
            LOCALS.add(t)
            return t
        if re.match(r"_t\d+$", t):
            return t
        if t in KEYWORDS or t in PARAMS or t in ("UNKNOWN_ARG", "ADDR"):
            return t
        if t == "MSG":
            return t
        i = ident(t)
        if t in cmds:
            return t
        nxt = e[m.end():m.end() + 1]
        idents.setdefault(i, "func" if nxt == "(" else "var")
        return i
    return re.sub(r"0x[0-9A-Fa-f]+h?|\d+h?|[A-Za-z_][\w?]*", lambda m: rep(m), e)


def cond(c):
    c = c.strip()
    return expr(c)


class Node:
    def __init__(self, text, ind):
        self.text, self.ind, self.kids = text, ind, []


def parse(lines):
    root = Node("", -1)
    st = [root]
    for l in lines:
        ind = len(l) - len(l.lstrip(" "))
        n = Node(l.strip(), ind)
        while st[-1].ind >= ind:
            st.pop()
        st[-1].kids.append(n)
        st.append(n)
    return root


def stmt(t):
    m = re.match(r"^-> (.*)$", t)
    if m and m.group(1).strip() == "falls off":
        return "return 0;   // falls off the end"
    if m:
        return "return %s;" % expr(m.group(1))
    m = re.match(r"^(\S+) = (.*)$", t)
    if m and not t.startswith("if "):
        return "%s = %s;" % (expr(m.group(1)), expr(m.group(2)))
    m = re.match(r"^(inc|dec) (\S+)$", t)
    if m:
        return "%s%s;" % (expr(m.group(2)), "++" if m.group(1) == "inc" else "--")
    m = re.match(r"^(\S+) (add|sub|or|and|xor|shl|shr) (.*)$", t)
    if m and "(" not in m.group(1):
        op = {"add": "+=", "sub": "-=", "or": "|=", "and": "&=", "xor": "^=", "shl": "<<=", "shr": ">>="}[m.group(2)]
        return "%s %s %s;" % (expr(m.group(1)), op, expr(m.group(3)))
    if re.match(r"^[\w?]+\(.*\)$", t):
        return expr(t) + ";"
    raise ValueError("untranslatable: " + t)


TEMP = 0


def split_cond(c):
    """`lhs OP rhs` at parenthesis depth 0 (rightmost operator) -> (lhs, op, rhs) or None"""
    depth, k = 0, len(c) - 1
    while k > 0:
        ch = c[k]
        if ch == ")":
            depth += 1
        elif ch == "(":
            depth -= 1
        elif depth == 0 and c[k - 1:k + 1] in ("==", "!=", "<=", ">="):
            return c[:k - 1], c[k - 1:k + 1], c[k + 1:]
        elif depth == 0 and ch in "<>" and c[k - 1] not in "<>=!":
            return c[:k], ch, c[k + 1:]
        k -= 1
    return None


def drop_inner_calls(out, text):
    """the digest lists a nested call (e.g. print_obj(..) used as an argument, or an `if` tested call) as a statement just before its user: keep only the nested one"""
    while out:
        prev = out[-1].strip()
        if prev.endswith(";") and prev[:-1] and prev[:-1] in text and re.match(r"^[\w]+\(.*\)$", prev[:-1]) and not prev.startswith(("return", "{", "}")):
            out.pop()
        else:
            break


def emit(nodes, ind, out):
    pad = "\t" * ind
    i = 0
    while i < len(nodes):
        n = nodes[i]
        if not n.text.startswith("if "):
            st = stmt(n.text)
            # the digest shows the last call of a path both as a statement and as `-> call(...)` (its result is the return value): call it once
            if st.startswith("return ") and out and out[-1].strip() == st[len("return "):] and not out[-1].strip().startswith(("}", "{", "if ")):
                out.pop()
            elif st.startswith("return ") or re.match(r"^[\w]+\(", st):
                drop_inner_calls(out, st)
            out.append(pad + st)
            i += 1
            continue
        # group of consecutive `if V==const:` siblings with a `V:other:` default
        j = i
        while j < len(nodes) and nodes[j].text.startswith("if "):
            j += 1
        group = nodes[i:j]
        other = [g for g in group if g.text.endswith(":other:")]
        regular = [g for g in group if not g.text.endswith(":other:")]
        # sibling tests of the same call result (`f(x)!=0` / `f(x)==0`, or `f(x)==a` / `f(x)==b`) are one test in the original: evaluate the call once
        hoisted = False
        if len(regular) >= 2:
            parts = [split_cond(g.text[3:-1]) for g in regular]
            if all(parts) and len({p[0] for p in parts}) == 1 and "(" in parts[0][0]:
                global TEMP
                TEMP += 1
                out.append(pad + "{ int _t%d = (int)(%s);" % (TEMP, expr(parts[0][0])))
                for g, p in zip(regular, parts):
                    g.text = "if _t%d%s%s:" % (TEMP, p[1], p[2])
                hoisted = True
        eq = all(re.match(r"^if [\w?\[\]+\-. ]+==(\w+|\{[0-9,]+\}):$", g.text) for g in regular)
        if other and regular and eq:
            first = True
            for g in regular:
                c = cond(g.text[3:-1])
                if first:
                    drop_inner_calls(out, c)
                out.append(pad + ("if (%s) {" if first else "} else if (%s) {") % c)
                emit(g.kids, ind + 1, out)
                first = False
            out.append(pad + "} else {")
            emit(other[0].kids, ind + 1, out)
            out.append(pad + "}")
        elif other and not regular:
            out.append(pad + "{")
            emit(other[0].kids, ind + 1, out)
            out.append(pad + "}")
        else:
            for g in regular + other:
                if g.text.endswith(":other:"):
                    out.append(pad + "{ /* %s */" % g.text[3:-1])
                    emit(g.kids, ind + 1, out)
                    out.append(pad + "}")
                else:
                    c_ = cond(g.text[3:-1])
                    drop_inner_calls(out, c_)
                    out.append(pad + "if (%s) {" % c_)
                    emit(g.kids, ind + 1, out)
                    out.append(pad + "}")
        if hoisted:
            out.append(pad + "}")
        i = j


def main():
    fn, order = load()
    body, todo, done = [], [], 0
    mm = os.path.join(OUT, "handlers_mismatch.txt")
    mismatch = set(open(mm).read().split()) if os.path.exists(mm) else set()   # written by make_trace_test.py: the digest version disagrees with the lifted one
    for name in order:
        owners, lines = fn[name]
        if name in mismatch:
            todo.append("%s	differs from the lifted version in the trace test (digest cannot express it)" % name)
            continue
        if not (name.startswith("objh_") or name.startswith("exit_fn_") or name.startswith("event_fn_") or name.startswith("conv_cb_") or name.startswith("conv_text_")):
            continue
        bad = [l.strip() for l in lines if IRREG.search(l)]
        if bad or not lines:
            todo.append("%s\t%d irregular line(s), e.g. %s" % (name, len(bad), bad[0] if bad else "empty"))
            continue
        saved = dict(idents)
        LOCALS.clear()
        out = []
        try:
            emit(parse(lines).kids, 1, out)
        except Exception as e:  # noqa: BLE001
            idents.clear()
            idents.update(saved)
            todo.append("%s\t%s" % (name, e))
            continue
        done += 1
        body.append("// %s%s\nintptr_t %s(int phase, int arg_2, int arg_4) {\n%s%s\n\treturn 0;\n}\n" % (name, "  [" + owners + "]" if owners else "", name, "".join("\tint %s = 0;\n" % v for v in sorted(LOCALS)), "\n".join(out)))
    api = ["// generated by gen_handlers.py: everything the translated handlers expect the host to provide", "#pragma once", "#include <cstdint>", "",
           "enum Command {"]
    api += ["\t%s = %d," % (k, v) for k, v in sorted(cmds.items(), key=lambda kv: kv[1])]
    api += ["};", "", "#define MSG(block, index) (0xF0000000u | ((block) << 16) | (index))", "#define ADDR(x) ((intptr_t)&(x))   /* address of a data item */", "#define UNKNOWN_ARG 0   /* argument the digest could not recover */", "#define SEG_(x) 0", "",
            "extern int cmd, dobj, iobj;   // current command (set by run_command)"]
    names = sorted(k for k, v in idents.items() if v == "var" and k not in PARAMS and k not in cmds)
    api += ["extern int %s;" % k for k in names]
    api += [""] + ["intptr_t %s(...);" % k for k in sorted(k for k, v in idents.items() if v == "func")]
    api += [""] + ["intptr_t %s(int phase, int arg_2 = 0, int arg_4 = 0);" % re.search(r"intptr_t (\w+)\(int phase", b).group(1) for b in body]
    open(os.path.join(OUT, "handlers_api.h"), "w").write("\n".join(api) + "\n")
    open(os.path.join(OUT, "handlers_gen.cpp"), "w").write(
        "// generated by gen_handlers.py from analysis/xanth_handler_digest.txt - do not edit\n#include \"handlers_api.h\"\n\n" + "\n".join(body))
    open(os.path.join(OUT, "handlers_todo.txt"), "w").write("\n".join(todo) + "\n")
    print("translated", done, "left", len(todo), "variables", len(names), "functions", sum(1 for v in idents.values() if v == "func"))


main()
