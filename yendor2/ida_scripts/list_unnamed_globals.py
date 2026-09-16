"""
Read-only: lists unnamed data globals (word_XXXXX/byte_XXXXX/
dword_XXXXX default names) sorted by cross-reference count
descending, to prioritize renaming candidates -- the data-global
analog of list_remaining_unnamed.py for functions.

    .\run_ida_script.ps1 list_unnamed_globals.py -NoExport
"""
import idc
import idautils
import ida_bytes
import re

DEFAULT_PAT = re.compile(r'^(word|byte|dword|unk|qword)_[0-9A-Fa-f]+$')

total_names = 0
matched = 0
rows = []
for ea, name in idautils.Names():
    total_names += 1
    if not DEFAULT_PAT.match(name):
        continue
    matched += 1
    nrefs = len(list(idautils.DataRefsTo(ea))) + len(list(idautils.CodeRefsTo(ea, 0))) + len(list(idautils.CodeRefsTo(ea, 1)))
    size = ida_bytes.get_item_size(ea)
    rows.append((nrefs, ea, name, size))

rows.sort(key=lambda r: -r[0])
for nrefs, ea, name, size in rows[:150]:
    print(f"{ea:#x}  refs={nrefs:4d}  size={size:2d}  {name}")
print(f"total names seen: {total_names}, matched default pattern: {matched}")
