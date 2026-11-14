"""Check that the engine's save loader and writer are exact inverses on every fixture save.

For each tests/fixtures/*/SAVED.GAM: copy it to a scratch SAVEGAME/, run build/savetool.exe (FILE_ReadSavegameFile then
FILE_WriteSavegameFile) and compare the result byte for byte with the original.
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "build", "savetool.exe")
FIX = os.path.join(ROOT, "tests", "fixtures")


def main():
    bad = 0
    for name in sorted(os.listdir(FIX)):
        src = os.path.join(FIX, name, "SAVED.GAM")
        if not os.path.exists(src):
            continue
        tmp = tempfile.mkdtemp(prefix="u5save_")
        os.makedirs(os.path.join(tmp, "SAVEGAME"))
        dst = os.path.join(tmp, "SAVEGAME", "SAVED.GAM")
        shutil.copy2(src, dst)
        r = subprocess.run([EXE], cwd=tmp, capture_output=True, text=True)
        a, b = open(src, "rb").read(), open(dst, "rb").read()
        if r.returncode != 0 or a != b:
            bad += 1
            diffs = [i for i in range(min(len(a), len(b))) if a[i] != b[i]]
            print("%-24s MISMATCH: sizes %d/%d, %d differing bytes, first at 0x%x" % (name, len(a), len(b), len(diffs), diffs[0] if diffs else -1))
        else:
            print("%-24s ok (%d bytes)" % (name, len(a)))
        shutil.rmtree(tmp, ignore_errors=True)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
