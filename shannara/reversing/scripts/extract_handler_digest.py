"""
Builds a compact "digest" of every object handler / verb default in Shannara.asm so the 700+ handlers can be read
(and eventually transpiled) without wading through raw assembly.

For each function (objh_NNN, verbdef_NN_*) it keeps only: labels, compare/test + conditional jump pairs, calls
with the constant register/stack arguments known at the call, and returns.  Immediate string offsets are shown with
their text (the listing's own "; "..."" comment) and SHANSTR message ids are shown as block:index.

    python extract_handler_digest.py ../../Shannara.asm ../analysis/retail_handler_digest.txt
"""
import re
import sys

ARGREGS = ("eax", "edx", "ebx", "ecx")


def parse(path):
    funcs = {}
    cur = None
    for line in open(path, encoding="utf-8", errors="replace"):
        m = re.match(r"^(\S+)\s+proc near", line)
        if m:
            cur = m.group(1)
            funcs[cur] = []
            continue
        if cur and re.match(r"^%s\s+endp" % re.escape(cur), line):
            cur = None
            continue
        if cur:
            funcs[cur].append(line.rstrip("\n"))
    return funcs


def fmt_imm(tok, comment):
    tok = tok.strip()
    m = re.match(r"^(0F[0-9A-F]{7})h$", tok)
    if m and len(tok) == 10:                      # 0xF0BBIIII = SHANSTR id
        v = int(m.group(1), 16)
        return "msg(%d:%d)" % ((v >> 16) & 0xFF, v & 0xFFFF)
    if tok.startswith("offset") and comment.startswith('"'):
        return comment.split(";")[0].strip()
    return tok


def digest(lines):
    out = []
    regs = {}
    stack = []
    for raw in lines:
        code, _, comment = raw.partition(";")
        comment = comment.strip()
        c = re.sub(r"\s+", " ", code).strip()
        if not c:
            continue
        m = re.match(r"^(loc_\w+|\w+):$", c)
        if m:
            out.append(c)
            continue
        m = re.match(r"^mov (e[abcd]x), (.+)$", c)
        if m and not re.match(r"^(e[a-z]{2}|\[|dword|word|byte)", m.group(2)):
            regs[m.group(1)] = fmt_imm(m.group(2), comment)
            continue
        m = re.match(r"^xor (e[abcd]x), \1$", c)
        if m:
            regs[m.group(1)] = "0"
            continue
        m = re.match(r"^push (.+)$", c)
        if m and not re.match(r"^e[a-z]{2}$", m.group(1)):
            stack.append(fmt_imm(m.group(1), comment))
            continue
        if re.match(r"^push e[a-z]{2}$", c):
            stack.append("?")
            continue
        m = re.match(r"^call (\S+)$", c)
        if m:
            args = ["%s=%s" % (r, regs[r]) for r in ARGREGS if r in regs]
            if stack:
                args.append("stack=[%s]" % ", ".join(reversed(stack)))
            out.append("  %s(%s)" % (m.group(1), ", ".join(args)))
            regs, stack = {}, []
            continue
        if re.match(r"^add esp, ", c):
            stack = []
            continue
        m = re.match(r"^(cmp|test) (.+)$", c)
        if m:
            out.append("  %s %s" % (m.group(1), m.group(2)))
            continue
        m = re.match(r"^(j\w+) (?:short )?(\S+)$", c)
        if m:
            out.append("  %s %s" % (m.group(1), m.group(2)))
            continue
        if c == "retn":
            out.append("  return")
            regs, stack = {}, []
    return out


def main():
    funcs = parse(sys.argv[1])
    names = sorted(n for n in funcs if n.startswith(("objh_", "verbdef_")))
    with open(sys.argv[2], "w", encoding="utf-8") as f:
        for n in names:
            f.write("== %s\n" % n)
            f.write("\n".join(digest(funcs[n])) + "\n\n")
    print(len(names), "handlers written")


if __name__ == "__main__":
    main()
