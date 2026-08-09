"""
IDAPython script -- run inside IDA (Alt-F7) with the Rob Blanc 1 IDB open,
AFTER apply_matches.py (needs the renamed functions to already exist).

Reads reversing/analysis/prototypes.json and applies each prototype via
idc.SetType(). Many of these reference AGS-specific struct/typedef names
(CharacterInfo*, ccInstance*, block, GUIMain*, ...) that likely don't exist
yet in this IDB's local type library -- SetType will fail for those, which
is expected, not a bug. Failures are reported and, where possible, recorded
as a plain-text comment instead so the intended signature isn't lost; once
the corresponding structs are defined (a separate, not-yet-built phase of
this project), re-running this script will pick them up automatically.

Safe to re-run: SetType is idempotent, and the fallback comment uses the
same [reversing-proto] tag convention as apply_matches.py's [reversing] tag
so re-runs replace rather than duplicate it.
"""
import json

PROTOTYPES_JSON = r"C:\dev\ags\reversing\analysis\prototypes.json"

try:
    import idc
    import ida_funcs
    IN_IDA = True
except ImportError:
    IN_IDA = False


TAG = "[reversing-proto] "


def strip_old_managed_comment(existing):
    if not existing or TAG not in existing:
        return existing or ""
    idx = existing.find(TAG)
    return existing[:idx].rstrip("\n")


def apply_one(name, info):
    if info.get("status") != "ok":
        return f"SKIP {name}: extraction failed ({info.get('reason')})"

    ea = idc.get_name_ea_simple(name)
    if ea == idc.BADADDR:
        return f"SKIP {name}: not found in IDB (run apply_matches.py first?)"

    prototype = info["prototype"]
    ok = idc.SetType(ea, prototype + ";")
    if ok:
        return f"OK {name}: type applied -- {prototype}"

    # SetType failed -- almost always an unresolved struct/typedef name.
    # Don't lose the intended signature: leave it as a comment.
    func = ida_funcs.get_func(ea)
    if func:
        existing = ida_funcs.get_func_cmt(func, True) or ""
        kept = strip_old_managed_comment(existing)
        note = f"{TAG}intended prototype (not yet applied -- unresolved type, needs struct defs):\n{prototype}"
        new_comment = (kept + "\n\n" + note).strip() if kept else note
        ida_funcs.set_func_cmt(func, new_comment, True)
    return f"DEFERRED {name}: SetType failed (likely unresolved struct/typedef), noted in comment -- {prototype}"


def main():
    if not IN_IDA:
        print("This script must be run inside IDA (idc/ida_funcs not importable).")
        return

    with open(PROTOTYPES_JSON, "r", encoding="utf-8") as f:
        prototypes = json.load(f)

    print(f"Loaded {len(prototypes)} prototype entries")

    counts = {"ok": 0, "deferred": 0, "skip": 0}
    for name, info in prototypes.items():
        result = apply_one(name, info)
        if result.startswith("OK"):
            counts["ok"] += 1
        elif result.startswith("DEFERRED"):
            counts["deferred"] += 1
        else:
            counts["skip"] += 1
        print(result)

    print(f"\nDone. applied={counts['ok']} deferred(needs structs)={counts['deferred']} skipped={counts['skip']}")


if __name__ == "__main__":
    main()
