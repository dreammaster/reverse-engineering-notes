#!/usr/bin/env python3
"""insertsec.py <target.cpp> <section-file> [--reg <registration-file>] [--before <marker>]
Inserts the text of a file before a marker line of a source file (default: the Registration banner), and appends
another file at the end; keeps the line ending style of the target."""
import sys
args = sys.argv[1:]
target, section = args[0], args[1]
reg = None
marker = "// ---------------------------------------------------------------------------------------------------------\n// Registration"
i = 2
while i < len(args):
    if args[i] == "--reg":
        reg = args[i + 1]; i += 2
    elif args[i] == "--before":
        marker = args[i + 1]; i += 2
    else:
        i += 1
s = open(target, newline="").read()
nl = "\r\n" if "\r\n" in s else "\n"
s = s.replace("\r\n", "\n")
new = open(section, newline="").read().replace("\r\n", "\n")
if marker not in s:
    sys.exit("marker not found")
s = s.replace(marker, new + marker, 1)
if reg:
    s += open(reg, newline="").read().replace("\r\n", "\n")
open(target, "w", newline="").write(s.replace("\n", nl))
