#!/usr/bin/env python3
"""For the Do() of a script command that turns a name into a code (a chain of wxString::Cmp against fixed strings,
each followed by a store of a code in the command): prints `"name" -> code`.
Usage: evmap.py <file.asm> <first-line> <last-line> [<member-offset-hex, default 64>]"""
import re
import subprocess
import sys

sys.stdout.reconfigure(encoding="utf-8", errors="replace")
path, first, last = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
member = (sys.argv[4] if len(sys.argv) > 4 else "64").upper()

strings = {}
out = subprocess.run([sys.executable, __file__.replace("evmap.py", "wstr.py"), path] + [], capture_output=True,
                     text=True).stdout
labels = []
with open(path, encoding="utf-8", errors="replace") as f:
    for n, line in enumerate(f, 1):
        if n < first:
            continue
        if n > last:
            break
        for m in re.finditer(r"offset (\w+)", line):
            if m.group(1) not in labels and not m.group(1).startswith(("_Z", "sub_", "loc_")):
                labels.append(m.group(1))

# the wide strings of the labels
res = subprocess.run([sys.executable, __file__.replace("evmap.py", "strs.py"), path, str(first), str(last)],
                     capture_output=True, text=True, encoding="utf-8", errors="replace").stdout
for line in res.splitlines():
    m = re.match(r"(\w+): L?\"(.*)\"$", line)
    if m:
        strings[m.group(1)] = m.group(2)

last_label = None
with open(path, encoding="utf-8", errors="replace") as f:
    for n, line in enumerate(f, 1):
        if n < first:
            continue
        if n > last:
            break
        code = line.split(";")[0].strip()
        m = re.match(r"mov\s+esi, offset (\w+)", code)
        if m and m.group(1) in strings:
            last_label = m.group(1)
        m = re.match(r"mov\s+dword ptr \[r\w+\+%sh\], ([0-9A-F]+)h?$" % member, code)
        if m and last_label:
            print('"%s" -> %d' % (strings[last_label], int(m.group(1), 16)))
            last_label = None
