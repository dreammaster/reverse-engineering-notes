#!/usr/bin/env python3
"""Lists the lines of a source file of src/deponia1 that no test ran (the tests built and run with -c, see coverage.py):
python tools/uncovered.py <path-filter> [first-line last-line]"""
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJ = os.path.join(ROOT, "build", "tests-64c")
GCOV = r"C:\mingw64\bin\gcov.exe"
SRC = os.path.join(ROOT, "src", "deponia1").replace("\\", "/").lower()
flt = sys.argv[1].lower()
first = int(sys.argv[2]) if len(sys.argv) > 2 else 0
last = int(sys.argv[3]) if len(sys.argv) > 3 else 10 ** 9

counts = {}
for dirpath, _, files in os.walk(OBJ):
    for f in files:
        if not f.endswith(".gcda"):
            continue
        r = subprocess.run([GCOV, "--json-format", "--stdout", os.path.join(dirpath, f)], capture_output=True, text=True, cwd=dirpath)
        for line in r.stdout.splitlines():
            if not line.startswith("{"):
                continue
            for entry in json.loads(line).get("files", []):
                path = entry["file"].replace("\\", "/")
                if not path.lower().startswith(SRC) or flt not in path.lower():
                    continue
                for l in entry["lines"]:
                    key = (path, l["line_number"])
                    counts[key] = counts.get(key, 0) + l["count"]

lines_by_file = {}
for (path, number), count in counts.items():
    lines_by_file.setdefault(path, {})[number] = count
for path, lines in sorted(lines_by_file.items()):
    source = open(path, encoding="utf-8", errors="replace").read().split("\n")
    print("==", path[len(SRC) + 1:])
    for number in sorted(lines):
        if lines[number] == 0 and first <= number <= last:
            print("%5d: %s" % (number, source[number - 1].rstrip()[:150]))
