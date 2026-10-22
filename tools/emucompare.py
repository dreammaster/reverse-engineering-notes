#!/usr/bin/env python3
"""Runs the same functions of two original executables (e.g. Deponia and Deponia4) on the same arguments in an x86-64 emulator
(Unicorn) and compares what they return. A condition or a constant that was changed shows up as another result, however the
compilers wrote it (-O0 against -O2 does not matter).

Only functions whose arguments are numbers can be run: int, unsigned, long, bool (0 or 1), char, enum (names like eFoo, FooEnum, TypeFoo),
float and double. Free functions and the static methods named in the sources (`static` in the headers of --headers) are tried;
a function that calls out of the image (a PLT stub to a library), reads a pointer it was not given or loops for ever is skipped.

Usage: tools/emucompare.py <first exe> <second exe> [--headers DIR] [--filter TEXT] [--list] [--methods]
  --methods       the non-static methods of the T* classes instead: `this` is a block of 4 KB of random small numbers (the same in both
                  runs), the results and what the method wrote into the block are compared. A class whose members moved between the
                  builds differs everywhere; for the others a difference is a difference of the code. Methods that call through a
                  pointer in the block (a virtual call, another object) fault and are skipped.
  --headers DIR   the sources whose `static` methods are tried too (default src/deponia4)
  --filter TEXT   only the functions whose demangled name contains TEXT
Prints per function: same results on all inputs, or the first inputs that differ. Needs nm and c++filt on the PATH, and unicorn."""
import os
import random
import re
import struct
import subprocess
import sys

from unicorn import UC_ARCH_X86, UC_MODE_64, Uc, UcError
from unicorn.x86_const import (UC_X86_REG_EAX, UC_X86_REG_R8, UC_X86_REG_R9, UC_X86_REG_RAX, UC_X86_REG_RCX, UC_X86_REG_RDI,
                               UC_X86_REG_RDX, UC_X86_REG_RIP, UC_X86_REG_RSI, UC_X86_REG_RSP, UC_X86_REG_XMM0, UC_X86_REG_XMM1,
                               UC_X86_REG_XMM2, UC_X86_REG_XMM3, UC_X86_REG_XMM4, UC_X86_REG_XMM5, UC_X86_REG_XMM6, UC_X86_REG_XMM7,
                               UC_X86_REG_FS_BASE)

INT_REGS = [UC_X86_REG_RDI, UC_X86_REG_RSI, UC_X86_REG_RDX, UC_X86_REG_RCX, UC_X86_REG_R8, UC_X86_REG_R9]
XMM_REGS = [UC_X86_REG_XMM0, UC_X86_REG_XMM1, UC_X86_REG_XMM2, UC_X86_REG_XMM3, UC_X86_REG_XMM4, UC_X86_REG_XMM5, UC_X86_REG_XMM6, UC_X86_REG_XMM7]
STACK = 0x7FFF00000000
STACK_SIZE = 0x100000
SENTINEL = 0x7FFE00000000
TLS = 0x7FFD00000000
HEAP = 0x7FFC00000000
HEAP_SIZE = 0x10000
OBJECT_BYTES = 0x1000
ENUM = re.compile(r"^(e[A-Z]\w*|\w*Enum|Type\w+|TypeOrder)$")
INT_TYPES = {"int", "unsigned int", "unsigned", "long", "unsigned long", "short", "unsigned short", "char", "unsigned char", "signed char",
             "bool", "long long", "unsigned long long"}


class Image:
    """One executable in an emulator: its PT_LOAD segments mapped at their addresses, a stack and a thread block."""

    def __init__(self, path):
        data = open(path, "rb").read()
        phoff, = struct.unpack_from("<Q", data, 0x20)
        phentsize, phnum = struct.unpack_from("<HH", data, 0x36)
        segs = []
        for i in range(phnum):
            p_type, flags, off, vaddr, paddr, filesz, memsz, align = struct.unpack_from("<IIQQQQQQ", data, phoff + i * phentsize)
            if p_type == 1:
                segs.append((vaddr, off, filesz, memsz))
        low = min(s[0] for s in segs) & ~0xFFF
        high = (max(s[0] + s[3] for s in segs) + 0xFFF) & ~0xFFF
        self.uc = Uc(UC_ARCH_X86, UC_MODE_64)
        self.uc.mem_map(low, high - low)
        for vaddr, off, filesz, memsz in segs:
            self.uc.mem_write(vaddr, data[off:off + filesz])
        self.uc.mem_map(STACK, STACK_SIZE)
        self.uc.mem_map(SENTINEL, 0x1000)
        self.uc.mem_map(TLS, 0x2000)
        self.uc.mem_map(HEAP, HEAP_SIZE)
        self.uc.mem_write(TLS + 0x28, b"\x11\x22\x33\x44\x55\x66\x77\x88")  # (the stack protector's canary)
        self.uc.mem_write(TLS, struct.pack("<Q", TLS))
        self.low, self.high = low, high

    def call(self, address, ints, floats, limit=200000, obj=None):
        """Calls the function: `ints` in the integer registers, `floats` as (value, is_double) in the xmm registers.
        Returns (rax, xmm0 bytes) or None when it could not be run."""
        uc = self.uc
        uc.reg_write(UC_X86_REG_FS_BASE, TLS)
        sp = STACK + STACK_SIZE - 0x1000
        uc.mem_write(sp, struct.pack("<Q", SENTINEL))
        uc.reg_write(UC_X86_REG_RSP, sp)
        for reg, v in zip(INT_REGS, ints):
            uc.reg_write(reg, v & 0xFFFFFFFFFFFFFFFF)
        for reg, (v, is_double) in zip(XMM_REGS, floats):
            raw = struct.pack("<d", v) if is_double else struct.pack("<f", v)
            uc.reg_write(reg, int.from_bytes(raw.ljust(16, b"\0"), "little"))
        uc.reg_write(UC_X86_REG_RAX, 0)
        if obj is not None:
            uc.mem_write(HEAP, bytes(HEAP_SIZE))
            uc.mem_write(HEAP, obj)
        try:
            uc.emu_start(address, SENTINEL, count=limit)
        except UcError:
            return None
        if uc.reg_read(UC_X86_REG_RIP) != SENTINEL:
            return None
        after = bytes(uc.mem_read(HEAP, len(obj))) if obj is not None else b""
        return uc.reg_read(UC_X86_REG_RAX), uc.reg_read(UC_X86_REG_XMM0).to_bytes(16, "little")[:8], after


def symbols(path):
    out = subprocess.run(["nm", "--defined-only", path], capture_output=True, text=True).stdout
    addr = {}
    for line in out.splitlines():
        p = line.split()
        if len(p) == 3 and p[1] in "TtWw":
            addr.setdefault(p[2], int(p[0], 16))
    return addr


def demangle(names):
    names = sorted(names)
    out = subprocess.run(["c++filt"], input="\n".join(names), capture_output=True, text=True).stdout.splitlines()
    return dict(zip(names, out))


def split_args(text):
    parts, depth, cur = [], 0, ""
    for ch in text:
        if ch in "<(":
            depth += 1
        elif ch in ">)":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        parts.append(cur.strip())
    return parts


def classify(demangled):
    """The kinds of the arguments ('i', 'f', 'd') or None when one is not a number."""
    m = re.match(r"^(?:[\w:<>~, *&]+\s)?([\w:~<>]+)\((.*)\)(?: const)?$", demangled)
    if not m:
        return None
    kinds = []
    for a in split_args(m.group(2)):
        a = re.sub(r"\bconst\b", "", a).strip()
        if a == "void" or a == "":
            continue
        if a == "float":
            kinds.append("f")
        elif a == "double":
            kinds.append("d")
        elif a == "bool":
            kinds.append("b")
        elif a in INT_TYPES or ENUM.match(a):
            kinds.append("i")
        else:
            return None
    return kinds


def statics(headers):
    """The `static` methods of the classes in the sources: 'Class::name'."""
    found = set()
    for root, _, names in os.walk(headers):
        for n in names:
            if not n.endswith(".h"):
                continue
            text = open(os.path.join(root, n), encoding="utf8", errors="replace").read()
            for cls in re.finditer(r"\b(?:class|struct)\s+(\w+)[^;{]*\{", text):
                end = text.find("\n};", cls.end())
                body = text[cls.end(): end if end > 0 else len(text)]
                for m in re.finditer(r"^\s*static\s+[\w:<>*& ]+?\s+\*?&?(\w+)\s*\(", body, re.M):
                    found.add(cls.group(1) + "::" + m.group(1))
    return found


def return_types(headers):
    """{(class or '', name): return type} from the declarations and definitions in the sources (the executables do not say)."""
    found = {}
    rx = re.compile(r"(?m)^\s*(?:static\s+|inline\s+|extern\s+|virtual\s+|constexpr\s+)*([A-Za-z_][\w:<>]*(?:\s+[A-Za-z_]\w*)?(?:\s*[*&])?)\s+(?:(\w+)::)?(\w+)\s*\(")
    for root, _, names in os.walk(headers):
        for n in names:
            if not n.endswith((".h", ".cpp")):
                continue
            text = open(os.path.join(root, n), encoding="utf8", errors="replace").read()
            for m in rx.finditer(text):
                t = re.sub(r"\s+", " ", m.group(1)).strip()
                if t.split()[0] in ("return", "else", "new", "delete", "if", "while", "for", "case", "throw", "using", "typedef"):
                    continue
                found.setdefault((m.group(2) or "", m.group(3)), t)
    return found


def inputs(kinds, rng):
    ints = [-300, -129, -128, -101, -100, -99, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 31, 32, 33, 63, 64,
            65, 99, 100, 101, 127, 128, 129, 255, 256, 257, 359, 360, 361, 999, 1000, 1001, 4095, 4096, 65535, 65536, 0x7FFFFFFF,
            -0x80000000]
    floats = [-1000.0, -10.0, -1.5, -1.0, -0.5, -0.0001, 0.0, 0.0001, 0.25, 0.5, 1.0, 1.5, 2.0, 3.14159, 10.0, 99.9, 100.0, 1000.0, 1e6]
    n_i, n_f = kinds.count("i") + kinds.count("b"), kinds.count("f") + kinds.count("d")
    if n_i == 1 and n_f == 0:
        for v in range(-5, 420):
            yield [v], []
        for v in ints:
            yield [v], []
        return
    if n_i == 0 and n_f == 1:
        for v in floats:
            yield [], [v]
        return
    for _ in range(400):
        yield [rng.choice(ints) for _ in range(n_i)], [rng.choice(floats) for _ in range(n_f)]


def same(a, b, ret, tolerance=2e-6):
    """Do the two results agree, in the register that the return type uses."""
    if ret in ("float", "double"):
        fmt, width = ("<f", 4) if ret == "float" else ("<d", 8)
        x, = struct.unpack(fmt, a[1][:width])
        y, = struct.unpack(fmt, b[1][:width])
        return x == y or (x != x and y != y) or (x == x and y == y and abs(x - y) <= tolerance * max(1.0, abs(x), abs(y)))
    mask = {"bool": 0xFF, "char": 0xFF, "unsigned char": 0xFF, "short": 0xFFFF, "unsigned short": 0xFFFF, "long": 0xFFFFFFFFFFFFFFFF,
            "unsigned long": 0xFFFFFFFFFFFFFFFF}.get(ret, 0xFFFFFFFF)
    return (a[0] & mask) == (b[0] & mask)


def kind_of_return(t):
    t = re.sub(r"(const|static|inline)", "", t).strip()
    if t in ("float", "double"):
        return t
    if t in INT_TYPES or ENUM.match(t):
        return t
    return None


def main():
    args = sys.argv[1:]
    headers = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src", "deponia4")
    flt = ""
    only_list = "--list" in args
    methods = "--methods" in args
    args = [a for a in args if a not in ("--list", "--methods")]
    for opt in ("--headers", "--filter"):
        if opt in args:
            i = args.index(opt)
            if opt == "--headers":
                headers = args[i + 1]
            else:
                flt = args[i + 1]
            del args[i:i + 2]
    if len(args) != 2:
        print(__doc__)
        return 1
    s1, s2 = symbols(args[0]), symbols(args[1])
    common = sorted(set(s1) & set(s2))
    dem = demangle([n for n in common if n.startswith("_Z")])
    static_names = statics(headers)
    returns = return_types(headers)
    todo = []
    skipped_unknown = []
    for n in common:
        d = dem.get(n)
        if not d or d.startswith(("std::", "__gnu", "operator ", "wx", "_", "hb_", "FT_", "SDL_", "lua", "tolua")) or "[clone" in d:
            continue
        kinds = classify(d)
        if kinds is None or len(kinds) > 5 or (not kinds and not methods):
            continue
        head = d.split("(")[0]
        is_method = False
        if "::" in head:
            if head not in static_names:
                if not methods or not re.match(r"^T\w*::\w+$", head) or d.endswith(" const") and False:
                    continue
                is_method = True
        elif methods:
            continue
        if flt and flt not in d:
            continue
        cls, _, name = head.rpartition("::")
        ret = kind_of_return(returns.get((cls.split("::")[-1], name), returns.get(("", name), "")))
        if ret is None:
            if not is_method:
                skipped_unknown.append(d)
                continue
            ret = "memory"  # (a method of a type not known: only what it writes into the object is compared)
        if methods and not is_method:
            continue
        todo.append((n, d, kinds, ret, is_method))
    print("candidates: %d" % len(todo))
    if only_list:
        for n, d, k, r, m in todo:
            print(" ", d, "".join(k), "->", r)
        print("return type not known from the sources: %d" % len(skipped_unknown))
        return 0
    im1, im2 = Image(args[0]), Image(args[1])
    rng = random.Random(1)
    ran = same_all = differ = skipped = 0
    for n, d, kinds, ret, is_method in todo:
        diffs = []
        count = 0
        failed = False
        arg_sets = list(inputs(kinds, rng)) if kinds else [([], [])]
        if is_method:
            rng.shuffle(arg_sets)
            arg_sets = arg_sets[:60]
        for iv, fv in arg_sets:
            iv = list(iv)
            for pos, k in enumerate([k for k in kinds if k in "ib"]):
                if k == "b":
                    iv[pos] &= 1  # (a bool is 0 or 1)
            obj = None
            if is_method:
                obj = bytearray()
                for _ in range(OBJECT_BYTES // 4):
                    roll = rng.random()
                    if roll < 0.15:
                        obj += struct.pack("<I", 0)
                    elif roll < 0.8:
                        obj += struct.pack("<I", rng.randint(0, 8))
                    elif roll < 0.9:
                        obj += struct.pack("<f", rng.choice([0.5, 1.0, 2.5, 10.0, 100.0, -3.0]))
                    else:
                        obj += struct.pack("<I", rng.choice([0xFFFFFFFF, 100, 255, 359, 1000]))
                obj = bytes(obj)
                iv = [HEAP] + list(iv)
            # the floats in the order of the arguments
            fl = []
            fi = iter(fv)
            for k in kinds:
                if k in "fd":
                    fl.append((next(fi), k == "d"))
            r1 = im1.call(s1[n], iv, fl, obj=obj)
            r2 = im2.call(s2[n], iv, fl, obj=obj)
            if r1 is None or r2 is None:
                failed = True
                break
            count += 1
            memory_differs = r1[2][8:] != r2[2][8:]  # (the first 8 bytes are the pointer to the table of virtual functions, each build has its own)
            if memory_differs or (ret != "memory" and not same(r1, r2, ret)):
                at = next((i for i in range(8, len(r1[2])) if r1[2][i] != r2[2][i]), -1) if memory_differs else -1
                diffs.append((iv[1:] if is_method else iv, [f[0] for f in fl], r1[0] & 0xFFFFFFFF, r2[0] & 0xFFFFFFFF,
                              "object differs at +0x%x" % at if memory_differs else ""))
        if failed or count == 0:
            skipped += 1
            continue
        ran += 1
        if diffs:
            differ += 1
            print("DIFFERENT %s: %d of %d inputs differ, e.g. %s" % (d, len(diffs), count, diffs[:3]))
        else:
            same_all += 1
            print("same      %s (%d inputs)" % (d, count))
    print("ran %d, the same on every input %d, different %d, skipped %d" % (ran, same_all, differ, skipped))
    return 0


sys.exit(main())
