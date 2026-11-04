r"""
Apply the rename table for the open database (IDA 8.3, headless or Alt+F7).

    .\run_ida_script.ps1 -Idb cadaver1\cad -ScriptName apply_renames.py

The table is renames/<game>/<stem>.tsv, derived from the IDB path
(disasm/<game>/<stem>.idb).  See renames/README.md for the column meanings.

Safety rules, so re-running is harmless and nothing of yours is overwritten:
  * a row is skipped if its `sig` bytes do not match what is at `ea`
    (wrong executable build / wrong address);
  * an address that already has a name that is neither IDA's auto name, the
    row's `old` name nor the `new` name is a CONFLICT and is skipped (you
    renamed it yourself);
  * comments are only written when the location has no comment yet;
  * rows below the minimum confidence are skipped;
  * the whole table is refused when fewer than 70% of its informative
    signatures (>= 8 bytes, not all the same byte) match the database -
    i.e. this is the wrong executable build.

Environment variables (set before launching idat/IDA):
    CADAVER_RENAMES_DRY=1       report only, change nothing (also pass -NoExport)
    CADAVER_RENAMES_MIN=guess   lowest confidence to apply: verified | likely (default) | guess
    CADAVER_RENAMES_FORCE=1     ignore sig mismatches and name conflicts
"""
import os
import sys

import idc
import ida_bytes
import ida_funcs
import ida_name

_here = os.path.dirname(os.path.abspath(__file__))
_root = os.path.dirname(_here)
sys.path.insert(0, os.path.join(_root, "tools"))
import renames_lib as R  # noqa: E402

DRY = os.environ.get("CADAVER_RENAMES_DRY") == "1"
FORCE = os.environ.get("CADAVER_RENAMES_FORCE") == "1"
MIN_CONF = os.environ.get("CADAVER_RENAMES_MIN", "likely")


def table_path():
    idb = idc.get_idb_path()
    game = os.path.basename(os.path.dirname(idb))
    stem = os.path.splitext(os.path.basename(idb))[0]
    return os.path.join(_root, "renames", game, stem + ".tsv"), "%s/%s" % (game, stem)


def apply_row(r, stats):
    ea = r.ea
    tag = "%#x %s" % (ea, r.new)
    if not ida_bytes.is_loaded(ea):
        stats["not_loaded"] += 1
        print("SKIP  %s: address not in this database" % tag)
        return
    if r.sig and not FORCE:
        want = R.sig_bytes(r.sig)
        have = ida_bytes.get_bytes(ea, len(want)) or b""
        if not R.sig_matches(have, r.sig):
            stats["sig"] += 1
            print("SKIP  %s: signature mismatch (database has %s)" % (tag, have.hex(" ")))
            return

    cur = ida_name.get_ea_name(ea)
    if cur == r.new:
        stats["same"] += 1
    else:
        usable = (not cur) or R.AUTO_NAME_RE.match(cur) or cur == r.old
        if not usable and not FORCE:
            stats["conflict"] += 1
            print("CONFLICT %s: already named %r in the database, left alone" % (tag, cur))
            return
        if DRY:
            print("WOULD rename %#x %s -> %s" % (ea, cur or "(none)", r.new))
        else:
            if r.kind == "func" and ida_funcs.get_func(ea) is None:
                if not idc.add_func(ea):
                    print("WARN  %s: could not create a function here, naming the address only" % tag)
            ok = idc.set_name(ea, r.new, idc.SN_NOWARN | idc.SN_CHECK)
            if not ok:
                stats["failed"] += 1
                print("FAIL  %s: set_name refused (name in use elsewhere?)" % tag)
                return
            print("OK    %#x %s -> %s" % (ea, cur or "(none)", r.new))
        stats["renamed"] += 1

    if r.comment and not DRY:
        if r.kind == "func" and ida_funcs.get_func(ea) is not None and ida_funcs.get_func(ea).start_ea == ea:
            if not idc.get_func_cmt(ea, 1):
                idc.set_func_cmt(ea, r.comment, 1)
                stats["comments"] += 1
        elif not idc.get_cmt(ea, 1):
            idc.set_cmt(ea, r.comment, 1)
            stats["comments"] += 1


def main():
    path, key = table_path()
    print("database %s, table %s" % (key, path))
    if not os.path.exists(path):
        print("no rename table for this database")
        return
    rows, errors = R.parse_tsv(path)
    for e in errors:
        print("TABLE ERROR:", e)
    if errors:
        print("table has errors, nothing applied")
        return
    if not FORCE:
        informative = [r for r in rows if r.sig and '??' not in r.sig and len(set(R.sig_bytes(r.sig))) > 2
                       and len(R.sig_bytes(r.sig)) >= 8]
        good = sum(1 for r in informative
                   if R.sig_matches(ida_bytes.get_bytes(r.ea, len(R.sig_bytes(r.sig))) or b"", r.sig))
        if informative and good * 10 < len(informative) * 7:
            print("WRONG BUILD? only %d of %d informative signatures match this database; table refused "
                  "(CADAVER_RENAMES_FORCE=1 overrides)" % (good, len(informative)))
            return
    min_rank = R.CONF_ORDER[MIN_CONF]
    stats = dict(renamed=0, same=0, conflict=0, sig=0, failed=0, not_loaded=0, comments=0, below_conf=0)
    for r in sorted(rows, key=lambda r: r.ea):
        if R.CONF_ORDER[r.conf] < min_rank:
            stats["below_conf"] += 1
            continue
        apply_row(r, stats)
    print("%s: %d rows; %s%s" % (key, len(rows), ", ".join("%s=%d" % kv for kv in stats.items()),
                                 "  (DRY RUN)" if DRY else ""))


main()
