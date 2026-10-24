"""
Regenerate the complete audit of src/ against the original game binaries.

    python audit/run_all.py [--game-dir C:/games/ultima5] [--idat "C:/Program Files/IDA Pro 8.3/idat.exe"] [--skip-ida]

Pipeline (see docs/audit.md for what each stage means):
  1. copy the original ULTIMA.EXE / *.OVL / *.DRV into ida/in/ (git-ignored scratch; never committed)
  2. make_seeds             C-defined function offsets -> seeds for IDA (frameless leaf functions have no push bp)
  3. IDA (headless)         ida_scripts/export_functions.py -> ida/out/<MODULE>.json for the EXE and every overlay
  4. derive_overlay_bases   per-overlay address shift (overlays are near-called into the EXE)
  5. parse_stubs            the EXE's overlay call-stub table (overlay entry points)
  6. audit_functions        function coverage       -> audit/function_audit.md/json
  7. fingerprint            callees + data refs     -> audit/fingerprint.md/json
  8. constants              integer immediates      -> audit/constants.md/json (json only)
  9. data_audit + pointer_tables   initialised data vs DATA.OVL -> audit/data_audit.md
 10. deviations             where the modern build differs from the original -> audit/deviations.md
"""
import argparse
import importlib
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

OVERLAYS = ["BLCKTHRN", "CAST", "CAST2", "CMDS", "COMBAT", "COMSUBS", "DNGLOOK", "DUNGEON", "ENDGAME", "FLAMES", "FONT",
            "INTRO", "LOOKOBJ", "MAINOUT", "NPC", "OUTSUBS", "SHOPPES", "SHOPPES2", "SHOPPES3", "SJOG", "TALK", "TOWN",
            "ZSTATS"]


def run_ida(idat, script, out_json, seeds, src, raw):
    arg = "%s %s %s %s" % (script, out_json, "raw" if raw else "noraw", seeds)
    cmd = [idat, "-A", "-c"]
    if raw:
        cmd += ["-TBinary file", "-pmetapc", "-b0"]
    cmd += ["-S" + arg, src]
    # IDA relaunches itself; capturing its output makes subprocess wait until it has really finished
    subprocess.run(cmd, cwd=os.path.dirname(src), capture_output=True)
    log = out_json + ".log"
    done = os.path.exists(log) and "done:" in open(log).read()
    return done


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game-dir", default=os.environ.get("U5_GAME_DIR", "C:/games/ultima5"))
    ap.add_argument("--idat", default="C:/Program Files/IDA Pro 8.3/idat.exe")
    ap.add_argument("--skip-ida", action="store_true", help="reuse existing ida/out/*.json")
    args = ap.parse_args()
    os.environ["U5_GAME_DIR"] = args.game_dir

    indir = os.path.join(ROOT, "ida", "in")
    outdir = os.path.join(ROOT, "ida", "out")
    os.makedirs(indir, exist_ok=True)
    os.makedirs(outdir, exist_ok=True)

    import audit_functions as a
    importlib.import_module("make_seeds")

    if not args.skip_ida:
        script = os.path.join(ROOT, "ida_scripts", "export_functions.py").replace("\\", "/")
        modules = [(m, m + ".OVL", True) for m in OVERLAYS] + [("ULTIMA", "ULTIMA.EXE", False)]
        for mod, fn, raw in modules:
            src = os.path.join(indir, fn)
            shutil.copy2(os.path.join(args.game_dir, fn), src)
            for stale in (src + ".idb", src + ".i64"):
                if os.path.exists(stale):
                    os.remove(stale)
            out_json = os.path.join(outdir, mod + ".json").replace("\\", "/")
            seeds = os.path.join(outdir, mod + ".seeds.json").replace("\\", "/")
            ok = run_ida(args.idat, script, out_json, seeds, src, raw)
            print("%-9s %s" % (mod, "ok" if ok else "FAILED (see %s.log)" % out_json))
            # the scratch copy of the original binary and its database are not kept
            for stale in (src, src + ".idb", src + ".i64"):
                if os.path.exists(stale):
                    os.remove(stale)

    for step in ("derive_overlay_bases", "parse_stubs"):
        importlib.import_module(step)
    a.main()
    for step in ("fingerprint", "constants", "data_audit", "pointer_tables", "deviations"):
        importlib.import_module(step).main()


if __name__ == "__main__":
    main()
