"""
IDAPython script -- run inside IDA (File > Script file..., or Alt-F7) with
the Rob Blanc 1 IDB open.

Reads reversing/analysis/matches.json and applies it to the live database:
  - renames functions/globals when "new_name" is set
  - attaches a repeatable function comment recording the confirmed source
    file / evidence, so the correlation survives independent of this repo
    and is visible directly in the disassembly while doing later C
    reconstruction work.

Safe to re-run any time matches.json grows (idempotent: comments are
replaced wholesale, not appended to, so re-running never duplicates text).

EDIT ASM_MATCHES_JSON below if your checkout lives somewhere other than
C:\\dev\\ags.
"""
import json

ASM_MATCHES_JSON = r"C:\dev\ags\reversing\analysis\matches.json"

try:
    import idc
    import idaapi
    import ida_funcs
    import ida_name
    IN_IDA = True
except ImportError:
    IN_IDA = False


TAG = "[reversing] "  # marks the start of our managed comment block, so re-runs can find/replace it


def make_comment(entry):
    lines = [TAG + "confirmed match"]
    if entry.get("source_file"):
        sf = entry["source_file"]
        if isinstance(sf, list):
            lines.append("source (ambiguous, pick one): " + " OR ".join(sf))
        else:
            lines.append(f"source: {sf}")
    elif entry.get("source_obj"):
        kind = "library" if entry.get("is_library") else "obj"
        lines.append(f"source obj ({kind}): {entry['source_obj']}")
    lines.append(f"confidence: {entry.get('confidence', 'unknown')}")
    if entry.get("evidence"):
        lines.append(f"evidence: {entry['evidence']}")
    if entry.get("notes"):
        lines.append(f"notes: {entry['notes']}")
    return "\n".join(lines)


def strip_old_managed_comment(existing):
    if not existing:
        return ""
    if TAG not in existing:
        return existing
    # keep any human-authored text that precedes our managed block, drop the block itself
    idx = existing.find(TAG)
    prefix = existing[:idx].rstrip("\n")
    return prefix


def apply_function_match(entry):
    name = entry["asm_name"]
    new_name = entry.get("new_name")
    ea = idc.get_name_ea_simple(name)
    if ea == idc.BADADDR and new_name:
        # already renamed by a previous run of this script -- look up by new_name instead
        ea = idc.get_name_ea_simple(new_name)
    if ea == idc.BADADDR:
        return f"SKIP (name not found in IDB): {name}"

    actions = []

    if entry.get("revert_name"):
        # A prior name (typically an uncertain FLIRT guess, tagged with "?"
        # by IDA) has been independently checked against the reference
        # source and found wrong -- clear it back to the auto-generated
        # sub_XXXXXXXX name rather than leave a misleading label in place.
        if idc.set_name(ea, "", idc.SN_AUTO | idc.SN_NOWARN):
            actions.append(f"reverted incorrect name '{name}' -> auto (sub_{ea:X})")
        else:
            actions.append(f"REVERT FAILED for '{name}'")

    if new_name and new_name != name:
        ok = idc.set_name(ea, new_name, idc.SN_CHECK | idc.SN_NOWARN)
        if not ok:
            # Common case: a data string literal (e.g. inside a script-export
            # name table) was auto-named identically to the function we want
            # to rename -- AGS registers script API functions by storing
            # their name as a string constant right next to the function
            # pointer, so this collision recurs constantly. If the colliding
            # address is data (not another function), bump it out of the way
            # with an s_ prefix and retry once.
            colliding_ea = idc.get_name_ea_simple(new_name)
            if colliding_ea != idc.BADADDR and ida_funcs.get_func(colliding_ea) is None:
                bumped = "s_" + new_name
                if idc.set_name(colliding_ea, bumped, idc.SN_CHECK | idc.SN_NOWARN):
                    ok = idc.set_name(ea, new_name, idc.SN_CHECK | idc.SN_NOWARN)
                    if ok:
                        actions.append(f"resolved collision: renamed conflicting data label -> {bumped}")
            if ok:
                actions.append(f"renamed -> {new_name}")
            else:
                actions.append(f"RENAME FAILED -> {new_name} (name collision, could not auto-resolve)")
        else:
            actions.append(f"renamed -> {new_name}")

    func = ida_funcs.get_func(ea)
    if func:
        existing = ida_funcs.get_func_cmt(func, True) or ""
        kept = strip_old_managed_comment(existing)
        managed = make_comment(entry)
        new_comment = (kept + "\n\n" + managed).strip() if kept else managed
        ida_funcs.set_func_cmt(func, new_comment, True)
        actions.append("comment updated")
    else:
        actions.append("WARNING: no function at address, comment not set (is it actually code?)")

    return f"OK {name} (0x{ea:X}): " + "; ".join(actions)


def main():
    if not IN_IDA:
        print("This script must be run inside IDA (idc/idaapi not importable).")
        return

    with open(ASM_MATCHES_JSON, "r", encoding="utf-8") as f:
        matches = json.load(f)

    print(f"Loaded {len(matches)} match entries from {ASM_MATCHES_JSON}")

    counts = {"ok": 0, "skip": 0, "other": 0}
    for entry in matches:
        kind = entry.get("kind")
        if kind in ("function", "manual"):
            result = apply_function_match(entry)
        else:
            result = f"SKIP (kind '{kind}' not yet handled by this script): {entry.get('asm_name')}"

        if result.startswith("OK"):
            counts["ok"] += 1
        elif result.startswith("SKIP"):
            counts["skip"] += 1
        else:
            counts["other"] += 1
        print(result)

    print(f"\nDone. ok={counts['ok']} skip={counts['skip']} other={counts['other']}")


if __name__ == "__main__":
    main()
