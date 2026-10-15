#!/usr/bin/env python3
"""Lists the functions of src/deponia1 that no test ever called.
Build and run the tests with coverage first:  python tools/tbuild.py <test>.cpp -c -r   (for every test)
then:  python tools/coverage.py [path-filter]     (-> the functions that were never called, by file, longest first)"""
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJ = os.path.join(ROOT, "build", "tests-64c")
GCOV = r"C:\mingw64\bin\gcov.exe"
SRC = os.path.join(ROOT, "src", "deponia1").replace("\\", "/").lower()
flt = sys.argv[1].lower() if len(sys.argv) > 1 else ""

functions = {}
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
                if not path.lower().startswith(SRC):
                    continue
                key_file = path[len(SRC) + 1:]
                for fn in entry["functions"]:
                    key = (key_file, fn["demangled_name"], fn["start_line"])
                    old = functions.get(key, (0, 0))
                    functions[key] = (old[0] + fn["execution_count"], max(old[1], fn["end_line"] - fn["start_line"] + 1))

by_file = {}
total = {}
for (path, name, start), (count, size) in functions.items():
    total[path] = total.get(path, 0) + 1
    if count == 0:
        by_file.setdefault(path, []).append((size, start, name))
n = 0
for path in sorted(by_file):
    if flt and flt not in path.lower():
        continue
    print("%s: %d of %d functions never called" % (path, len(by_file[path]), total[path]))
    for size, start, name in sorted(by_file[path], reverse=True):
        print("   line %5d  %4d lines  %s" % (start, size, name[:140]))
        n += 1
print("%d functions never called, of %d" % (n, len(functions)))
