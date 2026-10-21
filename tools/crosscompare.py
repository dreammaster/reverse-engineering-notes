#!/usr/bin/env python3
"""Compares the code of the reconstruction, compiled for Linux x86-64 (tools/crosscompile.py), with the code of the original
executable, function by function. The functions are matched by their mangled names (the original has a full symbol table).

A byte for byte comparison of the executables is not possible (the original is GCC 4.7.2 with wxWidgets, SDL, Lua, Box2D ... linked
in, the reconstruction is clang with stubs, and the members of the classes are not at the offsets of the original), so what is compared is
what a compiler does not change and a wrong source does:
  calls    the functions that are called, in order (the virtual calls by the slot)
  consts   the numbers in the code (ids of fields, events, commands, sizes, loop limits ...)
  strings  the texts that the function refers to
  shape    the sequence of the kinds of instruction, the number of branches
Per function the similarity of each of them is worked out (1.0 the same), and the functions are ranked by what is least alike.

Usage:
  tools/crosscompare.py [game]                      compare everything, write build/cross/<game>-compare.tsv, print the summary
  tools/crosscompare.py [game] --show SYMBOL        both versions of one function side by side, normalized
  tools/crosscompare.py [game] --worst N [FILTER]   the N least alike functions (whose name contains FILTER)
  tools/crosscompare.py [game] --callsdiff NAME     the calls of the functions of the engine (not std, wx, libc) that one has more of than the other, for the
                                                    functions whose name contains NAME
  tools/crosscompare.py [game] --classes            per class: the functions in both, the mean score, how many are under 0.5
  tools/crosscompare.py [game] --missing [FILTER]   the original functions of the classes of the reconstruction that it has no function for
  --tag NAME                                        compare the objects of build/cross/<game>-NAME (tools/crosscompile.py --tag NAME)
  --original PATH                                   the executable to compare with (default: the Deponia 1 Linux build in the games folder)"""
import collections
import difflib
import os
import pickle
import re
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJDUMP = os.environ.get("VIS_OBJDUMP", r"C:\mingw64\bin\objdump.exe")
DEFAULT_ORIGINAL = r"D:\Games\ScummVM\Dev\games\Visionnaire\Deponia1-Linux\data\noarch\game\Deponia"

args = sys.argv[1:]
original_path = DEFAULT_ORIGINAL
show = None
worst = None
missing = False
by_class = False
callsdiff = None
tag = ""
filters = []
game = "deponia1"
i = 0
while i < len(args):
    a = args[i]
    if a == "--original":
        original_path = args[i + 1]
        i += 1
    elif a == "--show":
        show = args[i + 1]
        i += 1
    elif a == "--worst":
        worst = int(args[i + 1])
        i += 1
    elif a == "--missing":
        missing = True
    elif a == "--classes":
        by_class = True
    elif a == "--callsdiff":
        callsdiff = args[i + 1]
        i += 1
    elif a == "--tag":
        tag = "-" + args[i + 1]
        i += 1
    elif a.startswith("-"):
        sys.exit("unknown option " + a)
    elif a in ("deponia1", "deponia2") or os.path.isdir(os.path.join(ROOT, "src", a)):
        game = a
    else:
        filters.append(a)
    i += 1

OBJ = os.path.join(ROOT, "build", "cross", game + tag)
CACHE = os.path.join(ROOT, "build", "cross", "original-%s.pkl" % os.path.basename(original_path))


# ---------------------------------------------------------------------------------------------------- ELF (just enough)

class Elf:
    """Sections and symbols of an ELF64 file (an executable or an object)."""

    def __init__(self, path):
        self.data = open(path, "rb").read()
        d = self.data
        shoff, = struct.unpack_from("<Q", d, 0x28)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", d, 0x3A)
        self.sections = []
        for n in range(shnum):
            name, typ, flags, addr, off, size, link, info, align, entsize = struct.unpack_from("<IIQQQQIIQQ", d, shoff + n * shentsize)
            self.sections.append([name, typ, flags, addr, off, size, link, info, align, entsize])
        stroff = self.sections[shstrndx][4]
        self.names = [self._cstr(stroff + s[0]) for s in self.sections]
        self.symbols = []   # (name, value, size, type, section index)
        for n, s in enumerate(self.sections):
            if s[1] == 2:   # SHT_SYMTAB
                strtab = self.sections[s[6]][4]
                for k in range(s[5] // 24):
                    nm, info, other, shndx, value, size = struct.unpack_from("<IBBHQQ", d, s[4] + k * 24)
                    self.symbols.append((self._cstr(strtab + nm), value, size, info & 15, shndx))

    def _cstr(self, off):
        end = self.data.index(b"\0", off)
        return self.data[off:end].decode("latin1")

    def section_named(self, name):
        for n, nm in enumerate(self.names):
            if nm == name:
                return n
        return -1

    def read_at(self, vaddr, length=256):
        """Bytes at a virtual address of an executable."""
        for s in self.sections:
            if s[3] and s[3] <= vaddr < s[3] + s[5] and s[1] != 8:
                off = s[4] + vaddr - s[3]
                return self.data[off:off + min(length, s[3] + s[5] - vaddr)]
        return b""


def text_at(raw):
    """The string at the start of raw bytes: a narrow one (printable ASCII/UTF-8) or a wide one (UTF-32LE); or None."""
    nul = raw.find(b"\0")
    if nul >= 3:
        try:
            s = raw[:nul].decode("utf-8")
            if all(c.isprintable() or c in "\n\r\t" for c in s):
                return s
        except UnicodeDecodeError:
            pass
    if len(raw) >= 8:
        chars = []
        for k in range(0, len(raw) - 3, 4):
            c, = struct.unpack_from("<I", raw, k)
            if c == 0:
                break
            if c > 0x10FFFF or (c < 32 and c not in (9, 10, 13)):
                return None
            chars.append(chr(c))
        if len(chars) >= 3 and all(ch.isprintable() or ch in "\n\r\t" for ch in chars):
            return "".join(chars)
    return None


# -------------------------------------------------------------------------------------------------------- disassembly

INSN = re.compile(r"^\s*([0-9a-f]+):\t([^\t]*)(?:\t(.*))?$")
LABEL = re.compile(r"^([0-9a-f]+) <(.+)>:$")
TARGET = re.compile(r"^([0-9a-f]+) <([^>+]+)(?:\+0x([0-9a-f]+))?>$")
SUFFIXED = {"mov", "add", "sub", "cmp", "and", "or", "xor", "test", "lea", "push", "pop", "shl", "shr", "sar", "inc", "dec", "neg", "not",
            "imul", "adc", "sbb", "xchg", "cmov", "movs", "stos", "call", "ret", "nop", "jmp", "cmpxchg", "xadd", "bt", "bts", "btr"}
CTOR_DTOR = re.compile(r"(C[0-2]|D[0-2])E")


def base_mnemonic(m):
    m = m.strip()
    if m.startswith(("rep", "lock", "notrack", "bnd", "data16", "repz", "repnz")) and " " in m:
        m = m.split(None, 1)[1]
    if m.endswith(("b", "w", "l", "q")) and m[:-1] in SUFFIXED and m not in ("sub", "shl", "sbb"):
        m = m[:-1]
    return m


def kind_of(m):
    """The kind of an instruction for the shape: all the conditional jumps and all the conditional sets/moves are alike."""
    if m.startswith("j") and m != "jmp":
        return "jcc"
    if m.startswith("set"):
        return "setcc"
    if m.startswith("cmov"):
        return "cmov"
    if m in ("movzbl", "movzwl", "movsbl", "movswl", "movslq", "movzbq", "movsbq", "movzx", "movsx", "movsbw", "movzbw", "cltq", "cwtl", "cltd", "cqto"):
        return "ext"
    if m.startswith(("movs", "movz")) and m not in ("movss", "movsd", "movsq", "movsl", "movsw", "movsb"):
        return "ext"
    if m in ("nop", "nopw", "nopl", "xchg", "data16", "ud2"):
        return "nop"
    return m


def normalize_name(name):
    name = name.split("@")[0]
    return CTOR_DTOR.sub(lambda m: m.group(1)[0] + "?E", name)


class Func:
    """One function: its instructions as (mnemonic, call target, constants, string, jump target, operand text)."""
    __slots__ = ("name", "start", "size", "insns")

    def __init__(self, name, start):
        self.name = name
        self.start = start
        self.size = 0
        self.insns = []

    # derived (not stored)
    def calls(self):
        return [normalize_name(t) for (m, t, c, s, j, o) in self.insns if t is not None and m.startswith(("call", "jmp"))
                and not (m.startswith("jmp") and t.startswith("<L"))]

    def consts(self):
        out = []
        for (m, t, c, s, j, o) in self.insns:
            out.extend(c)
        return out

    def strings(self):
        return [s for (m, t, c, s, j, o) in self.insns if s]

    def kinds(self):
        return [kind_of(m) for (m, t, c, s, j, o) in self.insns]


IMM = re.compile(r"\$(-?0x[0-9a-f]+)")


def parse_operands(mnem, ops, func_start, is_object):
    """(call target or None, constants, jump target offset or None)."""
    target = None
    jump = None
    consts = []
    if mnem.startswith(("call", "jmp")) or (mnem.startswith("j") and len(mnem) <= 4):
        if ops.startswith("*"):
            # an indirect call: through the table of the object (a virtual call) or a register
            m = re.match(r"\*(-?0x[0-9a-f]+)\(%r[a-z0-9]+\)", ops)
            if m and mnem.startswith("call"):
                target = "virtual:%d" % (int(m.group(1), 16) & 0xFFFFFFFF)
            else:
                target = "indirect"
        else:
            m = TARGET.match(ops.split("\t")[0].strip())
            if m:
                addr, name, off = int(m.group(1), 16), m.group(2), m.group(3)
                if off is not None and name_is_same(name, func_start):
                    jump = int(off, 16)
                else:
                    target = name if not off else None
                    if target is None:
                        jump = None
    for v in IMM.findall(ops):
        value = int(v, 16)
        if value < 0:
            value &= 0xFFFFFFFF
        # (the offsets that are added to a pointer are where the members are in the classes, not what the source says)
        if 2 <= value <= 0xFFFFF and not (mnem.startswith(("sub", "add")) and ("%rsp" in ops or value >= 0x40)):
            consts.append(value)
        elif value > 0xFFFFF and value < 0xFFFFFFF0 and (value & 0xFFFF) not in (0,) and not is_object and False:
            pass
    return target, consts, jump


_current_name = [None]


def name_is_same(name, func_start):
    return name == _current_name[0]


def read_disassembly(path, relocs, wanted=None):
    """Runs objdump on a file and returns {name: Func}. `wanted` limits the functions (a set of names) for a big file."""
    cmd = [OBJDUMP, "-d", "-w", "--no-show-raw-insn"]
    if relocs:
        cmd.append("-r")
    cmd.append(path)
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True, encoding="latin1", bufsize=1 << 20)
    funcs = {}
    cur = None
    for line in proc.stdout:
        line = line.rstrip("\n")
        m = LABEL.match(line)
        if m:
            name = m.group(2)
            if wanted is not None and name not in wanted:
                cur = None
                continue
            cur = Func(name, int(m.group(1), 16))
            funcs[name] = cur
            _current_name[0] = name
            continue
        if cur is None:
            continue
        m = INSN.match(line)
        if not m:
            continue
        addr = int(m.group(1), 16)
        text = m.group(2).strip()
        extra = m.group(3) or ""
        reloc_symbol = None
        if relocs and "R_X86_64_" in line:
            # the relocation of the instruction: where the symbol of a call or of a string is
            rm = re.search(r"R_X86_64_(\w+)\t(\S+)", line)
            if rm:
                reloc_symbol = (rm.group(1), rm.group(2))
        parts = text.split(None, 1)
        mnem = base_mnemonic(parts[0] if parts else "")
        ops = (parts[1] if len(parts) > 1 else "") + (("\t" + extra) if extra else "")
        target, consts, jump = parse_operands(mnem, text[len(parts[0]):].strip() if parts else "", cur.start, relocs)
        if reloc_symbol and mnem.startswith(("call", "jmp")):
            sym = re.sub(r"[-+]0x[0-9a-f]+$", "", reloc_symbol[1])
            if reloc_symbol[0] in ("PLT32", "PC32", "GOTPCREL", "32", "64"):
                target = sym
                jump = None
        string = None
        if mnem in ("nop", "nopw", "nopl", "data16", "xchg") and (mnem != "xchg" or "%ax,%ax" in text):
            continue   # (padding between the functions and the loops)
        cur.insns.append([mnem, target, consts, string, jump, (addr - cur.start, ops, reloc_symbol)])
        cur.size = addr - cur.start + 1
    proc.wait()
    return funcs


# ---------------------------------------------------------------------------------------------------- loading both sides

def load_original():
    if os.path.exists(CACHE) and os.path.getmtime(CACHE) > os.path.getmtime(original_path):
        return pickle.load(open(CACHE, "rb"))
    print("reading the original executable (once) ...", file=sys.stderr)
    elf = Elf(original_path)
    text_index = elf.section_named(".text")
    symbols = {}
    for name, value, size, typ, shndx in elf.symbols:
        if typ == 2 and shndx == text_index and size and name not in symbols:
            symbols[name] = (value, size)
    funcs = read_disassembly(original_path, False, set(symbols))
    for name, f in funcs.items():
        f.size = symbols[name][1]
        find_strings(f, elf)
    pickle.dump((funcs, symbols), open(CACHE, "wb"), protocol=4)
    return funcs, symbols


ADDRESS = re.compile(r"(?:\$0x|0x|^|\s)([0-9a-f]{6,7})(?:\(%rip\))?\b")
RIP = re.compile(r"0x([0-9a-f]+)\(%rip\)\s+#\s+([0-9a-f]+)")


def find_strings(f, elf):
    """The texts that the function refers to by the address of a constant (mov $0xd3a0f0,%esi) or by a rip relative lea."""
    rodata = elf.section_named(".rodata")
    if rodata < 0:
        return
    lo, hi = elf.sections[rodata][3], elf.sections[rodata][3] + elf.sections[rodata][5]
    for insn in f.insns:
        ops = insn[5][1]
        addr = None
        m = re.search(r"\$0x([0-9a-f]+),%[a-z0-9]+", ops)
        if m and insn[0] in ("mov", "push", "movl", "movq", "lea"):
            v = int(m.group(1), 16)
            if lo <= v < hi:
                addr = v
        m = RIP.search(ops)
        if m and insn[0] == "lea":
            v = int(m.group(2), 16)
            if lo <= v < hi:
                addr = v
        if addr is not None:
            t = text_at(elf.read_at(addr, 400))
            if t:
                insn[3] = t


def load_ours():
    """Our functions, from the objects of build/cross/<game>."""
    paths = []
    for root, _, names in os.walk(OBJ):
        for n in names:
            if n.endswith(".o"):
                paths.append(os.path.join(root, n))
    paths.sort()

    def one(path):
        elf = Elf(path)
        funcs = read_disassembly(path, True)
        # the strings: the relocation of an instruction names a section of the object and the place in it
        for f in funcs.values():
            for insn in f.insns:
                reloc = insn[5][2]
                if reloc and reloc[1].startswith(".rodata"):
                    sec_name, _, add = reloc[1].partition("+")
                    add = add.strip()
                    sec_name = re.sub(r"[-+]0x[0-9a-f]+$", "", reloc[1])
                    mm = re.search(r"([-+])0x([0-9a-f]+)$", reloc[1])
                    addend = 0
                    if mm:
                        addend = int(mm.group(2), 16) * (1 if mm.group(1) == "+" else -1)
                    n = elf.section_named(sec_name)
                    if n >= 0:
                        s = elf.sections[n]
                        # (for a 32 bit absolute or a pc relative relocation the addend is the place in the section, less 4 if relative)
                        for delta in (0, 4):
                            off = s[4] + addend + delta
                            if s[4] <= off < s[4] + s[5]:
                                t = text_at(elf.data[off:off + 400])
                                if t:
                                    insn[3] = t
                                    break
                # the constant of a mov $imm,reg that is only an address to be relocated is not a number
        return funcs

    ours = {}
    with ThreadPoolExecutor(8) as pool:
        for funcs in pool.map(one, paths):
            for k, v in funcs.items():
                if k not in ours:
                    ours[k] = v
    return ours


# --------------------------------------------------------------------------------------------------------- comparison

def ratio(a, b):
    if not a and not b:
        return 1.0
    if not a or not b:
        return 0.0
    if len(a) * len(b) > 4_000_000:
        # long functions: the sequences are cut (the beginning decides how alike they are)
        a, b = a[:2000], b[:2000]
    return difflib.SequenceMatcher(None, a, b, autojunk=False).ratio()


def bag_ratio(a, b):
    if not a and not b:
        return 1.0
    ca, cb = collections.Counter(a), collections.Counter(b)
    common = sum((ca & cb).values())
    return 2.0 * common / (len(a) + len(b))


def compare_one(of, mf):
    oc, mc = of.calls(), mf.calls()
    ok = of.kinds()
    mk = mf.kinds()
    return {
        "calls": ratio(oc, mc),
        "consts": bag_ratio(of.consts(), mf.consts()),
        "strings": bag_ratio(of.strings(), mf.strings()),
        "shape": ratio(ok, mk),
        "ncalls": (len(oc), len(mc)),
        "ninsn": (len(of.insns), len(mf.insns)),
        "jcc": (sum(1 for k in ok if k == "jcc"), sum(1 for k in mk if k == "jcc")),
    }


def score(r):
    # what a wrong source shows first: other calls, other numbers, other texts; the shape follows
    weights = {"calls": 0.4, "consts": 0.25, "strings": 0.15, "shape": 0.2}
    # (a part with nothing in it on both sides says nothing)
    return sum(weights[k] * r[k] for k in weights)


def format_func(f, with_addr=False):
    lines = []
    for (m, t, c, s, j, o) in f.insns:
        desc = m
        if t is not None:
            desc += " -> " + t
        if j is not None:
            desc += " -> +0x%x" % j
        if c:
            desc += "  consts " + ",".join("0x%x" % v for v in c)
        if s:
            desc += "  string %r" % (s[:60],)
        lines.append(desc)
    return lines


def main():
    original, symbols = load_original()
    ours = load_ours()
    common = sorted(set(original) & set(ours))

    if show:
        names = [n for n in common if show == n or (show in n and not any(show == x for x in common))]
        if not names:
            print("no function like %s in both (ours: %s, original: %s)" % (show, show in ours, show in original))
            return
        for n in names[:3]:
            of, mf = original[n], ours[n]
            r = compare_one(of, mf)
            print("== %s   calls %.2f consts %.2f strings %.2f shape %.2f   instructions %d / %d, size %d / %d" % (
                n, r["calls"], r["consts"], r["strings"], r["shape"], len(of.insns), len(mf.insns), of.size, mf.size))
            a, b = format_func(of), format_func(mf)
            sm = difflib.SequenceMatcher(None, [x.split("  ")[0] for x in a], [x.split("  ")[0] for x in b], autojunk=False)
            for op, i1, i2, j1, j2 in sm.get_opcodes():
                if op == "equal":
                    for k in range(i2 - i1):
                        if a[i1 + k] != b[j1 + k]:
                            print("   %-50s | %s" % (a[i1 + k], b[j1 + k]))
                    continue
                for k in range(max(i2 - i1, j2 - j1)):
                    left = a[i1 + k] if i1 + k < i2 else ""
                    right = b[j1 + k] if j1 + k < j2 else ""
                    print("%s  %-50s | %s" % ("-+"[0] if op != "insert" else "+", left, right))
            print("   (original: calls %s)" % ", ".join(of.calls()))
            print("   (ours:     calls %s)" % ", ".join(mf.calls()))
        return

    if missing:
        classes = collections.Counter()
        for n in ours:
            m = re.match(r"_ZN?K?(\d+)([A-Za-z_0-9]+)", n)
            if m:
                length = int(m.group(1))
                classes[m.group(2)[:length]] += 1
        rows = []
        for n in original:
            if n in ours:
                continue
            m = re.match(r"_ZN?K?(\d+)([A-Za-z_0-9]+)", n)
            if not m:
                continue
            cls = m.group(2)[:int(m.group(1))]
            if cls in classes and (not filters or any(f in n for f in filters)):
                rows.append((cls, n, original[n].size))
        rows.sort()
        for cls, n, size in rows:
            print("%-28s %6d  %s" % (cls, size, n))
        print("%d original functions of the classes of the reconstruction that it has no function for" % len(rows))
        return

    results = {}
    for n in common:
        results[n] = compare_one(original[n], ours[n])

    out = os.path.join(ROOT, "build", "cross", "%s%s-compare.tsv" % (game, tag))
    ranked = sorted(common, key=lambda n: (score(results[n]), -original[n].size))
    with open(out, "w") as fh:
        fh.write("name\tscore\tcalls\tconsts\tstrings\tshape\tinsns_orig\tinsns_ours\tcalls_orig\tcalls_ours\tjcc_orig\tjcc_ours\n")
        for n in ranked:
            r = results[n]
            fh.write("%s\t%.3f\t%.2f\t%.2f\t%.2f\t%.2f\t%d\t%d\t%d\t%d\t%d\t%d\n" % (
                n, score(r), r["calls"], r["consts"], r["strings"], r["shape"], r["ninsn"][0], r["ninsn"][1], r["ncalls"][0],
                r["ncalls"][1], r["jcc"][0], r["jcc"][1]))

    if callsdiff:
        library = re.compile(r"_ZNS|_ZSt|_ZNK?St|_ZN9__gnu|_Zn|_Zd|_ZNK?8wx|_ZN8wx|_Z.*wx|virtual|indirect|_Unwind|__cxa|memcpy|memmove|strlen|strcmp|free|malloc")
        for n in common:
            if callsdiff not in n:
                continue
            a = collections.Counter(c for c in original[n].calls() if not library.match(c))
            b = collections.Counter(c for c in ours[n].calls() if not library.match(c))
            if a == b:
                continue
            print("== %s   (%d / %d instructions)" % (n, len(original[n].insns), len(ours[n].insns)))
            for c, k in sorted((a - b).items()):
                print("   original calls %s %d more time(s)" % (c, k))
            for c, k in sorted((b - a).items()):
                print("   ours calls     %s %d more time(s)" % (c, k))
        return

    if by_class:
        rows = collections.defaultdict(list)
        for n in common:
            m = re.match(r"_ZN?K?(\d+)([A-Za-z_0-9]+)", n)
            cls = m.group(2)[:int(m.group(1))] if m else "(functions)"
            rows[cls].append(n)
        table = []
        for cls, names in rows.items():
            total = sum(original[n].size for n in names)
            mean = sum(score(results[n]) * original[n].size for n in names) / max(total, 1)
            table.append((mean, cls, len(names), sum(1 for n in names if score(results[n]) < 0.5), total))
        table.sort()
        print("%-30s %5s %5s %8s %6s" % ("class", "funcs", "<0.5", "bytes", "mean"))
        for mean, cls, count, low, total in table:
            print("%-30s %5d %5d %8d %6.2f" % (cls, count, low, total, mean))
        return

    if worst:
        shown = 0
        for n in ranked:
            if filters and not any(f in n for f in filters):
                continue
            if original[n].size < 40:
                continue
            r = results[n]
            print("%.3f  calls %.2f consts %.2f strings %.2f shape %.2f  insns %4d/%-4d  %s" % (
                score(r), r["calls"], r["consts"], r["strings"], r["shape"], r["ninsn"][0], r["ninsn"][1], n))
            shown += 1
            if shown >= worst:
                break
        return

    print("original: %d functions, reconstruction: %d, in both: %d" % (len(original), len(ours), len(common)))
    print("only in the reconstruction: %d (helpers, stubs, inlined or renamed in the original)" % len(set(ours) - set(original)))
    buckets = collections.Counter()
    exact_calls = 0
    for n in common:
        r = results[n]
        s = score(r)
        buckets["0.95-1.00" if s >= 0.95 else "0.85-0.95" if s >= 0.85 else "0.70-0.85" if s >= 0.70 else "0.50-0.70" if s >= 0.50 else "0.00-0.50"] += 1
        if r["calls"] == 1.0 and r["ncalls"][0] > 0:
            exact_calls += 1
    for k in sorted(buckets, reverse=True):
        print("  score %s  %5d functions" % (k, buckets[k]))
    print("  with the same calls in the same order (and at least one call): %d" % exact_calls)
    same_consts = sum(1 for n in common if results[n]["consts"] == 1.0)
    print("  with the same numbers: %d; with the same texts: %d" % (same_consts, sum(1 for n in common if results[n]["strings"] == 1.0)))
    print("written %s" % out)


main()
