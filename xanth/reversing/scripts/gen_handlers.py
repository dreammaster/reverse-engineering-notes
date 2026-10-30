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

IRREG = re.compile(r"\[b[px]|\[[a-z]{2}\]|\bloop\b|\btail\b|\bes:|\bds =|\bcs:|\bds:|\bss:|\bvar_|\bcall\b")
PARAMS = {"phase", "cmd", "dobj", "iobj", "arg_2", "arg_4"}
KEYWORDS = {"if", "int", "return", "msg", "MSG", "other"}

enums = [r for r in csv.DictReader(open(os.path.join(A, "xanth_enums.tsv")), delimiter="\t")]
cmds = {r["name"]: int(r["value"]) for r in enums if r["enum"] == "CMD"}


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


idents = {}   # name -> kind ('var' or 'func')


def expr(e):
    """translate an expression of the digest to C++, registering identifiers"""
    e = e.strip()
    e = re.sub(r"msg\((\w+):", r"MSG(\1, ", e)
    e = re.sub(r"\bseg (\w+)", r"SEG_\1", e)
    e = re.sub(r"&([A-Za-z_][A-Za-z_0-9?]*)", r"ADDR(\1)", e)
    e = re.sub(r"(?<![\w?])\?(?![\w?])", "UNKNOWN_ARG", e)

    def rep(m):
        t = m.group(0)
        if re.fullmatch(r"0x[0-9A-Fa-f]+|\d+", t):
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


def emit(nodes, ind, out):
    pad = "\t" * ind
    i = 0
    while i < len(nodes):
        n = nodes[i]
        if not n.text.startswith("if "):
            out.append(pad + stmt(n.text))
            i += 1
            continue
        # group of consecutive `if V==const:` siblings with a `V:other:` default
        j = i
        while j < len(nodes) and nodes[j].text.startswith("if "):
            j += 1
        group = nodes[i:j]
        other = [g for g in group if g.text.endswith(":other:")]
        regular = [g for g in group if not g.text.endswith(":other:")]
        eq = all(re.match(r"^if [\w?\[\]+\-. ]+==\w+:$", g.text) for g in regular)
        if other and regular and eq:
            first = True
            for g in regular:
                c = cond(g.text[3:-1])
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
            for g in group:
                if g.text.endswith(":other:"):
                    out.append(pad + "{ /* %s */" % g.text[3:-1])
                    emit(g.kids, ind + 1, out)
                    out.append(pad + "}")
                else:
                    out.append(pad + "if (%s) {" % cond(g.text[3:-1]))
                    emit(g.kids, ind + 1, out)
                    out.append(pad + "}")
        i = j


def main():
    fn, order = load()
    body, todo, done = [], [], 0
    for name in order:
        owners, lines = fn[name]
        if not (name.startswith("objh_") or name.startswith("exit_fn_") or name.startswith("conv_cb_") or name.startswith("conv_text_")):
            continue
        bad = [l.strip() for l in lines if IRREG.search(l)]
        if bad or not lines:
            todo.append("%s\t%d irregular line(s), e.g. %s" % (name, len(bad), bad[0] if bad else "empty"))
            continue
        saved = dict(idents)
        out = []
        try:
            emit(parse(lines).kids, 1, out)
        except Exception as e:  # noqa: BLE001
            idents.clear()
            idents.update(saved)
            todo.append("%s\t%s" % (name, e))
            continue
        done += 1
        body.append("// %s%s\nintptr_t %s(int phase, int arg_2, int arg_4) {\n%s\n\treturn 0;\n}\n" % (name, "  [" + owners + "]" if owners else "", name, "\n".join(out)))
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
