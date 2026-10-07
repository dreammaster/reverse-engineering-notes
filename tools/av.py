#!/usr/bin/env python3
"""A compact reading view of a range of the IDA .asm export, for long functions.
Keeps every instruction that moves data into an argument register or compares, but drops the
unwind/cleanup noise: destructor calls of temporaries (and the `lea rdi, [..] ; void *` before
them), try/} markers, alignment, and comments except the demangled callee. The 3-byte id packing
after TVisObjRef::GetId() becomes one `;PACKID` marker (its result is the last register added).
Usage: av.py <file.asm> <first-line> <last-line>
Local variables are written as `v398` (var_398) / `a8` (arg_8); calls as `CALL Class::method(sig)`."""
import re
import sys

sys.stdout.reconfigure(encoding="utf-8", errors="replace")

path, first, last = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])

DTOR = re.compile(r"_ZN10TVisObjRefD[12]Ev|_ZN6TVListD[12]Ev|~basic_string|_ZNSbIwSt11char_traitsIwESaIwEED|"
                  r"_ZNSsD1Ev|_ZN6TTimerD2Ev|_ZN8wxStringD|_ZN10wxFileNameD|_ZN11TCharHolderD|"
                  r"_ZNSt6vectorI.*ED[12]Ev|Unwind_Resume|__cxa_end_catch|__cxa_begin_catch|__cxa_rethrow|"
                  r"__cxa_guard")
NOISE = re.compile(r"exchange_and_add|_M_destroy|S_empty_rep|^\s*nop|^\s*(push|pop)\s|\bxchg\s+ax, ax|"
                   r"_ZdlPv|~wxString|~wxFileName|~TCharHolder|~TVisObjRef|~TVList|~basic_string|~vector|~list")
out = []

# field ids of the game-data schema, to name the immediates that are one
FIELDS = {}
try:
    import os
    here = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(here, "..", "src", "deponia1", "vstables", "fieldIds.h"), encoding="utf-8") as ff:
        for ln in ff:
            m = re.match(r"\s*(k\w+) = 0x([0-9A-F]+),", ln)
            if m:
                FIELDS[int(m.group(2), 16)] = m.group(1)
except OSError:
    pass


def short(s):
    s = re.sub(r"\[rsp\+[0-9A-F]+h\+(var|arg)_([0-9A-F]+)\]",
               lambda m: ("v" if m.group(1) == "var" else "a") + m.group(2), s)
    return s


with open(path, encoding="utf-8", errors="replace") as f:
    for n, line in enumerate(f, 1):
        if n < first:
            continue
        if n > last:
            break
        s = line.rstrip("\r\n")
        raw = s.strip()
        if not raw or raw.startswith(";") or raw.startswith(("align", "var_", "arg_", "public ")):
            continue
        code, _, comment = raw.partition(";")
        code = code.strip()
        comment = comment.strip()
        if re.match(r"^[A-Za-z_][\w@$.?]*:", s):
            out.append("%d %s" % (n, s.split(";")[0].rstrip()))
            continue
        if not code:
            continue
        if code.startswith("call"):
            if DTOR.search(code):
                # the destructor's argument load and the call are noise
                while out and "void*" in out[-1]:
                    out.pop()
                continue
            callee = comment if comment and "::" in comment else code.split()[1]
            out.append("%d  CALL %s" % (n, short(callee)))
            continue
        if NOISE.search(code):
            continue
        if "void *" in comment and code.startswith(("lea     rdi", "mov     rdi")):
            out.append("   %s  ;void*" % short(code))
            continue
        extra = ""
        mi = re.match(r"mov\s+(esi|edx|ecx|r8d|r9d),\s+([0-9A-F]+)h$", code)
        if mi and int(mi.group(2), 16) in FIELDS and int(mi.group(2), 16) >= 0x20:
            extra = "   ;" + FIELDS[int(mi.group(2), 16)]
        if comment and ("offset" in code or "TVisObjRef *" in comment or "this" in comment):
            extra = "   ; " + comment[:50]
        if code.startswith("j"):
            out.append("%d  %s" % (n, short(code)))
        else:
            out.append("   %s%s" % (short(code), extra))

PACK = re.compile(r"^\s*(movsx|movzx)\s+\w+, (byte ptr \[rax(\+[12])?\]|\w+)$|^\s*shl\s+\w+, (8|10h)$|"
                  r"^\s*sar\s+\w+, 1Fh$|^\s*and\s+\w+, 0FF000000h$|^\s*add\s+\w+, \w+$|^\s*mov\s+e\w+, e\w+$|"
                  r"^\s*lea\s+\w+, \[r\w+\+r\w+\]$|^\s*mov\s+rdi, r\w+(\s+; this)?$|^\s*xor\s+edx, edx$|"
                  r"^\s*lea\s+\w+, \[rax\+rdx\]$")
res = []
i = 0
while i < len(out):
    res.append(out[i])
    if out[i].endswith("GetId(void)"):
        j = i + 1
        dropped = []
        while j < len(out) and PACK.match(out[j]):
            dropped.append(out[j])
            j += 1
        if len(dropped) >= 6:
            keep = [d for d in dropped if re.match(r"^\s*mov\s+rdi, r", d)]
            last_add = [d for d in dropped if re.match(r"^\s*(add|lea)\s", d)]
            res.append("   ;PACKID " + (last_add[-1].strip() if last_add else ""))
            res.extend(keep[-1:])
            i = j
            continue
    i += 1
print("\n".join(res))
