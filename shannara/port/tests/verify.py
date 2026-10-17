"""
Differential test: the C++ readers (port/build/shan_tool) must give exactly the same results as the verified Python
references in reversing/scripts on the real retail/demo files.

    python port/tests/verify.py [path to shan_tool]

Needs the git-ignored data directories game/, game_out/pics (extracted PICS.UHA) and demo/ next to port/.
"""
import glob
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, os.path.join(ROOT, "reversing", "scripts"))
TOOL = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "port", "build", "shan_tool.exe")

import fnt_read, pic_scan, rgn_read, shanstr           # noqa: E402
import q_decode_ref, q_play                            # noqa: E402

M = (1 << 64) - 1


def fnv(data, h=1469598103934665603):
    for b in data:
        h ^= b
        h = (h * 1099511628211) & M
    return h


def run(*args):
    r = subprocess.run([TOOL] + list(args), capture_output=True, text=True, check=True)
    return r.stdout.splitlines()


fails = 0
checks = 0


def check(name, ok, detail=""):
    global fails, checks
    checks += 1
    if not ok:
        fails += 1
        print("FAIL", name, detail)


# ---- SHANSTR -------------------------------------------------------------------------------------------------------------
path = os.path.join(ROOT, "game", "SHANSTR.DAT")
if os.path.exists(path):
    out_cpp = os.path.join(ROOT, "port", "build", "shanstr_cpp.txt")
    out_py = os.path.join(ROOT, "port", "build", "shanstr_py.txt")
    head = run("shanstr", path, "dump", out_cpp)[0]
    s = shanstr.ShanStr(open(path, "rb").read())
    with open(out_py, "w", encoding="latin-1", newline="\n") as f:
        for b, (cnt, _) in enumerate(s.blocks):
            for i in range(cnt):
                f.write("%d:%d\t%s\n" % (b, i, s.string(b, i).decode("latin-1").replace("\n", "\\n").replace("\r", "\\r").replace("\x0b", "\\v")))
    a, b = open(out_cpp, "rb").read(), open(out_py, "rb").read()
    check("SHANSTR dump identical", a == b, "(%d vs %d bytes) %s" % (len(a), len(b), head))
    print("SHANSTR: %d strings, %s" % (sum(c for c, _ in s.blocks), "identical" if a == b else "DIFFERENT"))

# ---- PIC -----------------------------------------------------------------------------------------------------------------
for path in sorted(glob.glob(os.path.join(ROOT, "game_out", "pics", "*.PIC"))):
    d = open(path, "rb").read()
    lines = run("pic", path)[1:]
    es = pic_scan.entries(d)
    py = []
    for e in es:
        pal, xy, px = pic_scan.load_entry(d, e)
        # python's load_entry reads the frame table as one (x, y) pair at the start; the C++ reader keeps every frame's pair
        n = e["w"] * e["h"] * max(1, e["count"])
        base = e["off"]
        offs = b""
        if e["count"]:
            offs = d[base:base + 4 * e["count"]]
            base += 4 * e["count"]
        if e["flags"] & 0x10:
            pal = d[base:base + 768]
            base += 768
        px = d[base:base + n]
        h = fnv(px)
        if e["flags"] & 0x10:
            h = fnv(pal, h)
        h = fnv(offs, h)
        py.append("%d off=%d frames=%d flags=%02x w=%d h=%d pal=%d px=%d hash=%016x" % (e["i"], e["off"], e["count"], e["flags"], e["w"], e["h"], 1 if e["flags"] & 0x10 else 0, len(px), h))
    check(os.path.basename(path), lines == py, "first diff: %s" % (next(((a, b) for a, b in zip(lines, py) if a != b), (len(lines), len(py))),))
print("PIC files checked:", len(glob.glob(os.path.join(ROOT, "game_out", "pics", "*.PIC"))))

# ---- RGN -----------------------------------------------------------------------------------------------------------------
n = 0
for path in sorted(glob.glob(os.path.join(ROOT, "game", "*.RGN"))):
    n += 1
    lines = run("rgn", path)[1:]
    py = []
    for i, e in enumerate(rgn_read.read(path)):
        if not e[0]:
            continue
        h = 1469598103934665603
        for rid, kind, pts in e[3]:
            h = fnv(struct.pack("<H", rid & 0xFFFF) + bytes([kind == "rect"]), h)
            for v in pts:
                h = fnv(struct.pack("<h", v), h)
        py.append("%d base=%d count=%d hash=%016x" % (i, e[1], e[2], h))
    check(os.path.basename(path), lines == py, "")
print("RGN files checked:", n)

# ---- FNT -----------------------------------------------------------------------------------------------------------------
n = 0
for path in sorted(glob.glob(os.path.join(ROOT, "game", "*.FNT"))):
    n += 1
    d = open(path, "rb").read()
    f = fnt_read.Font(d)
    h = 1469598103934665603
    for c in range(f.first, f.last + 1):
        h = fnv(f.glyph(c), h)
    exp = "first %d last %d bpr %d height %d bpp %d size %d expected %d glyphhash=%016x" % (f.first, f.last, f.bpr, f.height, f.b1, f.size, f.expected, h)
    check(os.path.basename(path), run("fnt", path)[0] == exp, "")
print("FNT files checked:", n)

# ---- Q video -------------------------------------------------------------------------------------------------------------
n = 0
for path in sorted(glob.glob(os.path.join(ROOT, "demo", "*.q"))):
    n += 1
    lines = run("q", path)
    q = q_play.QFile(open(path, "rb").read())
    tables = q_decode_ref.load_tables()
    py = []
    for f, video, pal, used, size, t in q.frames(tables):
        h = 1469598103934665603
        for y in range(q.H * 4):
            h = fnv(video[y * 640: y * 640 + q.W * 4], h)
        h = fnv(pal, h)
        py.append("frame %d type %d stream %d consumed %d hash=%016x" % (f, t, size, used, h))
    got = [l for l in lines if l.startswith("frame ")]
    check(os.path.basename(path), got == py, "first diff: %s" % (next(((a, b) for a, b in zip(got, py) if a != b), (len(got), len(py))),))
    print("Q %s: %d frames %s" % (os.path.basename(path), len(py), "identical" if got == py else "DIFFERENT"))
print("Q files checked:", n)

# ---- MUS (XMIDI archives) -------------------------------------------------------------------------------------------------
for path in (os.path.join(ROOT, "game", "SHAN001.MUS"), os.path.join(ROOT, "demo", "Demo001.mus")):
    if not os.path.exists(path):
        continue
    d = open(path, "rb").read()
    count, = struct.unpack_from("<H", d, 0)
    py = []
    for i in range(count):
        off, size = struct.unpack_from("<IH", d, 2 + 6 * i)
        if size:
            py.append("%d off=%d size=%d hash=%016x" % (len(py), off, size, fnv(d[off:off + size])))
            assert d[off:off + 4] == b"FORM" and d[off + 8:off + 12] == b"XDIR" and size == 30 + struct.unpack_from(">I", d, off + 26)[0]
    lines = run("mus", path)
    check(os.path.basename(path), lines[0] == "tracks %d valid xmidi %d" % (len(py), len(py)) and lines[1:] == py, lines[0])
    print("MUS %s: %d tracks" % (os.path.basename(path), len(py)))

# ---- OBJECT.DAT ----------------------------------------------------------------------------------------------------------
path = os.path.join(ROOT, "game", "OBJECT.DAT")
if os.path.exists(path):
    d = open(path, "rb").read()
    n, = struct.unpack_from("<H", d, 0)
    pool = d[2:2 + n]
    names = pool.split(b"\0")[:5]
    exp = ["pool %d strings %d" % (n, pool.count(b"\0"))]
    off = 1
    for nm in names:
        exp.append("%d %s" % (off, nm.decode("latin-1")))
        off += len(nm) + 1
    check("OBJECT.DAT", run("objdat", path) == exp, str(exp[:2]))
    print("OBJECT.DAT: pool", n, "bytes,", pool.count(b"\0"), "strings")

print("\n%d checks, %d failed" % (checks, fails))
sys.exit(1 if fails else 0)
