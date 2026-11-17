"""Lists the handler scenarios where the standalone engine differs from the oracle: scan_diff.py mine.log oracle.log [filter]

mine.log is written by test_scan with XANTH_SCANOUT=<file>, oracle.log by `host_test ... scanlog <file>` (both hold game text and stay local).  Each scenario is
"obj phase verb<TAB>ret<TAB>state<TAB>events<TAB>eventCount<TAB>text"; the original's word wrap writes the bytes F0 FF into the strings it prints, they are ignored.
"""
import re
import sys


def load(path):
    d = {}
    cur = None
    for line in open(path, "rb").read().decode("latin1").split(chr(10)):
        m = re.match(r"^(\d+ \d+ \d+)" + chr(9), line)
        if m:
            cur = m.group(1)
            d[cur] = line
        elif cur:
            d[cur] += chr(10) + line
    return d


def main():
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    mine, oracle = load(sys.argv[1]), load(sys.argv[2])
    want = sys.argv[3] if len(sys.argv) > 3 else None
    n = 0
    for k, v in sorted(mine.items(), key=lambda kv: tuple(int(x) for x in kv[0].split())):
        w = oracle.get(k)
        if w is None:
            continue
        w = w.replace(chr(0xF0) + chr(0xFF), "")
        if v == w:
            continue
        a, b = v.split(chr(9)), w.split(chr(9))
        what = "".join(c for c, i in (("ret ", 1), ("state ", 2), ("events ", 3), ("count ", 4), ("text ", 5)) if a[i] != b[i])
        if want and want not in what:
            continue
        n += 1
        print(k, "|", what, "| ret", a[1], "/", b[1])
        if "text" in what:
            print("   mine:  ", a[5][:160].replace(chr(31), "~"))
            print("   oracle:", b[5][:160].replace(chr(31), "~"))
    print(n, "differences")


main()
