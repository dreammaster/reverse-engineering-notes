#!/usr/bin/env python3
"""
Build reversing/analysis/matches.json — the canonical, IDA-agnostic record of
"we know what this disassembly entity corresponds to in the reference source".

This is the file reversing/scripts/apply_matches.py (run inside IDA) consumes
to (re-)annotate the live IDB. Re-run this generator any time
refmap_symbols.json / string_matches.json / functions.json are refreshed, or
append hand-verified entries directly (kind: "manual") for matches found by
inspection that the automated passes can't derive (e.g. sub_XXXXXXXX ->
a specific source function, found by reading the disassembly).

Automated sources currently merged, in confidence order:
  1. refmap exact-name match (high) -- the asm function's current name is an
     exact linker-symbol match against the reference build's map file, so we
     know precisely which reference .obj/source file it was compiled from.
     NOTE: this reference map is from a build of the *3.2.1.1115* engine, ~9
     years newer than Rob Blanc 1 (2002). A name match confirms lineage/
     naming, not that the implementation is identical -- always diff by hand
     before assuming behavior is unchanged.
  2. Everything else (string matches, manual notes) is left in its own file
     for human review rather than auto-merged, since string co-occurrence is
     suggestive, not proof.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
ANALYSIS = Path(__file__).resolve().parent.parent / "analysis"

# obj file stem (as it appears in the .map, without extension) -> actual source path
# Built by matching stems against files under Common/ and Engine/ (case-insensitive).
def build_obj_index():
    index = {}
    for d in (ROOT / "Common", ROOT / "Engine"):
        if not d.exists():
            continue
        for p in d.rglob("*"):
            if p.is_file() and p.suffix.lower() in (".cpp", ".c"):
                index.setdefault(p.stem.lower(), []).append(str(p.relative_to(ROOT)).replace("\\", "/"))
    return index


def main():
    functions = json.loads((ANALYSIS / "functions.json").read_text(encoding="utf-8"))
    refmap = json.loads((ANALYSIS / "refmap_symbols.json").read_text(encoding="utf-8"))
    obj_index = build_obj_index()

    matches = []
    unresolved_obj = {}
    for f in functions:
        if not f["is_named"]:
            continue
        name = f["name"]
        sym = refmap.get(name)
        if not sym:
            continue
        obj = sym["objfile"]
        stem = obj.rsplit(".", 1)[0].lower()
        candidates = obj_index.get(stem, [])
        if len(candidates) == 1:
            source_file = candidates[0]
        elif len(candidates) > 1:
            source_file = candidates  # ambiguous, list all
        else:
            source_file = None
            unresolved_obj.setdefault(obj, 0)
            unresolved_obj[obj] += 1

        is_library = source_file is None and (":" in obj)  # e.g. "libucrtd:strcmp.obj", "alleg_s_crt:sound.obj"

        matches.append({
            "kind": "function",
            "asm_name": name,
            "new_name": None,
            "source_file": source_file,
            "source_obj": obj,
            "is_library": is_library,
            "confidence": "high",
            "evidence": f"exact linker-symbol match vs reference build map (acwin.map), obj={obj}",
        })

    ANALYSIS.mkdir(parents=True, exist_ok=True)
    (ANALYSIS / "matches.json").write_text(json.dumps(matches, indent=1), encoding="utf-8")
    print(f"Wrote {len(matches)} matches")
    if unresolved_obj:
        print("Object files referenced by matches but not found under Common/Engine (need manual mapping):")
        for obj, count in sorted(unresolved_obj.items(), key=lambda x: -x[1]):
            print(f"  {obj}: {count} symbol(s)")


if __name__ == "__main__":
    main()
