"""Write ida/out/<MOD>.seeds.json (offsets of every C-defined function) for export_functions.py."""
import json
import os
import audit_functions as a

funcs = a.load_c_functions()
by_mod = {}
for (mod, off) in funcs:
    by_mod.setdefault(mod, []).append(off - (0 if mod != "ULTIMA" else 0))
for mod, offs in by_mod.items():
    # EXE offsets are relative to 0x10000 in IDA's segment base; pass them as segment-relative offsets
    json.dump(sorted(set(offs)), open(os.path.join(a.OUT, mod + ".seeds.json"), "w"))
print({m: len(v) for m, v in by_mod.items()})
