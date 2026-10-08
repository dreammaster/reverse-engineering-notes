#!/usr/bin/env python3
"""The syntax of a script command from its `Cmd<Name>_GetSyntax` function: the arguments and flags (type, mandatory),
the minimum version, and the description texts, in the order the function adds them.
Usage: cmdsyn.py <file.asm> <first-line> <last-line> [--docs]
Without --docs only AddDoc (the description of the command) and the argument names (AddArgDoc) are shown."""
import re
import subprocess
import sys

sys.stdout.reconfigure(encoding="utf-8", errors="replace")

path, first, last = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
docs = "--docs" in sys.argv

strings = {}
out = subprocess.run([sys.executable, __file__.replace("cmdsyn.py", "strs.py"), path, str(first), str(last)],
                     capture_output=True, text=True, encoding="utf-8", errors="replace").stdout
for line in out.splitlines():
    m = re.match(r"(\w+): [L]?\"(.*)\"$", line)
    if m:
        strings[m.group(1)] = m.group(2)

TYPES = ["none", "bool", "int", "float", "point", "rect", "string", "path", "sprite", "object", "text", "intlist",
         "floatlist", "pointlist", "rectlist", "stringlist", "pathlist", "spritelist", "objectlist", "textlist",
         "any", "flags"]

regs = {}
vars_ = {}      # stack variable -> label of the wide string built into it
lastlea = {}    # register -> stack variable
with open(path, encoding="utf-8", errors="replace") as f:
    for n, line in enumerate(f, 1):
        if n < first:
            continue
        if n > last:
            break
        code = line.split(";")[0].strip()
        ml = re.match(r"lea\s+(\w+), \[rsp\+\w+\+(var_\w+)\]", code)
        if ml:
            lastlea[ml.group(1)] = ml.group(2)
            continue
        if "call" in code and "basic_string(wchar_t const*" in line.replace("_ZNSbIwSt11char_traitsIwESaIwEEC2EPKwRKS1_", "basic_string(wchar_t const*") or "EC2EPKwRKS1_" in code:
            if "rdi" in lastlea and regs.get("esi") and regs["esi"][0] == "s":
                vars_[lastlea["rdi"]] = regs["esi"][1]
            continue
        m = re.match(r"mov\s+(\w+), (?:\(?offset (\w+)(?:\+(\w+))?\)?|([0-9A-F]+)h?|(\w+))$", code)
        if m and m.group(1) in ("esi", "edx", "ecx", "r8d", "rsi", "rdx", "rcx", "r8"):
            reg = m.group(1)
            reg = {"rsi": "esi", "rdx": "edx", "rcx": "ecx", "r8": "r8d"}.get(reg, reg)
            if m.group(2):
                regs[reg] = ("s", m.group(2))
            elif m.group(4) is not None:
                regs[reg] = ("i", int(m.group(4), 16) if re.fullmatch(r"[0-9A-F]+", m.group(4)) else 0)
            continue
        m = re.match(r"xor\s+(\w+), (\w+)$", code)
        if m and m.group(1) == m.group(2):
            regs[{"rsi": "esi", "rdx": "edx", "rcx": "ecx", "r8": "r8d"}.get(m.group(1), m.group(1))] = ("i", 0)
            continue
        if "call" not in code:
            continue

        def val(r):
            v = regs.get(r)
            if not v:
                return "?"
            if v[0] == "s":
                return '"%s"' % strings.get(v[1], v[1])
            return v[1]

        if "TArgSyntax::AddArg(" in line or "AddArgE8TArgTypeb" in line:
            t = val("esi")
            print("AddArg(%s, %s)" % (TYPES[t] if isinstance(t, int) and t < len(TYPES) else t,
                                      "mandatory" if val("edx") == 1 else "optional"))
        elif "AddFlagERK8wxString" in line or "TArgSyntax::AddFlag(" in line:
            t = val("ecx")

            def name(reg):
                label = vars_.get(lastlea.get(reg))
                return '"%s"' % strings.get(label, label) if label else val({"rsi": "esi", "rdx": "edx"}[reg])

            print("AddFlag(short=%s, long=%s, %s, %s)" % (name("rsi"), name("rdx"),
                                                         TYPES[t] if isinstance(t, int) and t < len(TYPES) else t,
                                                         "mandatory" if val("r8d") == 1 else "optional"))
        elif "SetMinVersion" in line:
            print("SetMinVersion(%s)" % val("esi"))
        elif "AddDoc" in line and "ArgDoc" not in line and "FlagDoc" not in line and "ReturnDoc" not in line and "ExampleDoc" not in line:
            print("DOC: %s" % val("esi"))
        elif "AddArgDoc" in line:
            print("  argname=%s%s" % (val("esi"), ("  - " + val("edx")) if docs else ""))
        elif "AddFlagDoc" in line and docs:
            print("  flagdoc(%s): %s" % (val("esi"), val("edx")))
        elif "AddReturnDoc" in line:
            print("  returns %s%s" % (val("esi"), ("  - " + val("edx")) if docs else ""))
        elif "AddBoolRetDoc" in line:
            print("  returns bool %s" % val("esi"))
        elif "AddExampleDoc" in line and docs:
            print("  example: %s" % val("edx"))
