#!/usr/bin/env python3
"""av.py with the noise of C++ cleanup dropped even more: string refcount releases, spills of registers,
exception paths after the function's last `retn`. For reading the Do/Redo of the script commands.
Usage: avc.py <file.asm> <first-line> <last-line>"""
import re
import subprocess
import sys

sys.stdout.reconfigure(encoding="utf-8", errors="replace")

out = subprocess.run([sys.executable, __file__.replace("avc.py", "av.py")] + sys.argv[1:], capture_output=True,
                     text=True, encoding="utf-8", errors="replace").stdout.splitlines()

drop = re.compile(r"exchange_and_add|_M_destroy|~pair|rsp\+arg|^\s*retn|add\s+rsp|sub\s+rsp|\s+(push|pop)\s|"
                  r"mov\s+(rbx|rbp|r1[2-5]), v[0-9A-F]+$|mov\s+v[0-9A-F]+, (rbx|rbp|r1[2-5])$|"
                  r"^\s+sub\s+(rbx|rbp|r1[2-5]|r[0-9]+), 18h|^\s+lea\s+rdi, \[(rbx|rbp|r1[2-5])\+10h\]$|"
                  r"^\s+lea\s+rsi, v[0-9A-F]+$|^\s+mov\s+rdi, (rbx|rbp|r1[2-5])$|std::wstring::basic_string|"
                  r"lea\s+rdx, v[0-9A-F]+$|^\s*$")
lines = []
for l in out:
    if drop.search(l):
        continue
    lines.append(l)

# cut everything after the last `endp` of the first function unless several are listed (cleanup tails come before endp)
print("\n".join(lines))
