#!/usr/bin/env python3
"""Makes the cross toolchain that tools/crosscompile.py uses, outside the repository (default C:\\dev\\crosstools, override with
VIS_CROSSTOOLS): clang of the ziglang wheel (it writes Linux ELF objects) and the headers of Debian 7 (wheezy), the system the
original GCC 4.7.2 came from (libstdc++6-4.7-dev, libc6-dev, linux-libc-dev; also the GCC 4.7.2 packages themselves, whose
cc1plus could make the very same code as the original if it could be run: it is a Linux program).
Needs `curl` (python's own download fails on some networks for the certificates). Usage: tools/crosssetup.py"""
import io
import os
import subprocess
import sys
import tarfile
import zipfile

TOOLS = os.environ.get("VIS_CROSSTOOLS", r"C:\dev\crosstools")
ZIG_VERSION = "0.14.1"
DEBIAN = "http://archive.debian.org/debian/pool/main/"
DEBS = [
    "g/gcc-4.7/libstdc++6-4.7-dev_4.7.2-5_amd64.deb",
    "e/eglibc/libc6-dev_2.13-38+deb7u10_amd64.deb",
    "e/eglibc/libc6_2.13-38+deb7u10_amd64.deb",
    "l/linux/linux-libc-dev_3.2.78-1_amd64.deb",
    "g/gcc-4.7/gcc-4.7_4.7.2-5_amd64.deb",
    "g/gcc-4.7/g++-4.7_4.7.2-5_amd64.deb",
    "g/gcc-4.7/cpp-4.7_4.7.2-5_amd64.deb",
]


def download(url, path):
    if os.path.exists(path) and os.path.getsize(path) > 0:
        return
    print("downloading", url)
    subprocess.run(["curl", "-s", "-L", "-m", "900", "-o", path, url], check=True)


def zig_wheel_url():
    """The address of the Windows wheel, from the index of PyPI."""
    import json
    text = subprocess.run(["curl", "-s", "-m", "60", "https://pypi.org/pypi/ziglang/%s/json" % ZIG_VERSION], capture_output=True, text=True, check=True).stdout
    for f in json.loads(text)["urls"]:
        if f["filename"].endswith("win_amd64.whl"):
            return f["url"]
    sys.exit("no Windows wheel of ziglang " + ZIG_VERSION)


def unpack_deb(deb, out):
    data = open(deb, "rb").read()
    assert data[:8] == b"!<arch>\n", deb
    pos = 8
    while pos < len(data):
        name = data[pos:pos + 16].decode().strip()
        size = int(data[pos + 48:pos + 58].decode())
        body = data[pos + 60:pos + 60 + size]
        pos += 60 + size + (size & 1)
        if name.startswith("data.tar"):
            with tarfile.open(fileobj=io.BytesIO(body), mode="r:*") as t:
                for m in t:
                    if m.issym() or m.islnk():
                        continue   # (links need rights on Windows and are not needed)
                    try:
                        t.extract(m, out)
                    except Exception as e:
                        print("skipped", m.name, e)
            return


def main():
    os.makedirs(os.path.join(TOOLS, "debs"), exist_ok=True)
    wheel = os.path.join(TOOLS, "ziglang.whl")
    download(zig_wheel_url(), wheel)
    if not os.path.exists(os.path.join(TOOLS, "zigpkg", "ziglang", "zig.exe")):
        zipfile.ZipFile(wheel).extractall(os.path.join(TOOLS, "zigpkg"))
    for deb in DEBS:
        path = os.path.join(TOOLS, "debs", os.path.basename(deb))
        download(DEBIAN + deb, path)
        unpack_deb(path, os.path.join(TOOLS, "sysroot"))
    print("done:", TOOLS)


main()
