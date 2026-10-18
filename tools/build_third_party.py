#!/usr/bin/env python3
"""Builds the third-party libraries of third_party/ (the versions of the original, see THIRD_PARTY.md) as static libraries:
build/third_party/<bits>/lib<name>.a with the headers they need staying where they are in third_party/.
Usage: tools/build_third_party.py [-32] [name ...]     (names: ogg opus opusfile webp freetype harfbuzz; default all)"""
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TP = os.path.join(ROOT, "third_party")
bits = 32 if "-32" in sys.argv else 64
BIN = r"C:\mingw64\bin" if bits == 64 else r"C:\mingw32\bin"
CC = os.path.join(BIN, "gcc.exe")
CXX = os.path.join(BIN, "g++.exe")
AR = os.path.join(BIN, "ar.exe")
OUT = os.path.join(ROOT, "build", "third_party", str(bits))


def read_mk(path):
    """The variables of an automake source list: NAME = a b \\ c ..."""
    text = open(path).read().replace("\\\n", " ")
    result = {}
    for line in text.splitlines():
        m = re.match(r"^(\w+)\s*=\s*(.*)$", line)
        if m:
            result[m.group(1)] = m.group(2).split()
    return result


def ogg_config_types():
    """include/ogg/config_types.h from the .in file (the sizes of the ints of the platform)."""
    src = os.path.join(TP, "ogg", "include", "ogg", "config_types.h.in")
    dst = os.path.join(OUT, "gen", "ogg", "config_types.h")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    text = open(src).read()
    for name, value in (("INCLUDE_INTTYPES_H", "1"), ("INCLUDE_STDINT_H", "1"), ("INCLUDE_SYS_TYPES_H", "0"),
                        ("SIZE16", "short"), ("USIZE16", "unsigned short"), ("SIZE32", "int"), ("USIZE32", "unsigned int"),
                        ("SIZE64", "long long")):
        text = text.replace("@" + name + "@", value)
    open(dst, "w").write(text)
    return os.path.join(OUT, "gen")


def libs():
    ogg = os.path.join(TP, "ogg")
    opus = os.path.join(TP, "opus")
    webp = os.path.join(TP, "libwebp")
    gen = ogg_config_types()

    o = read_mk(os.path.join(opus, "opus_sources.mk"))
    c = read_mk(os.path.join(opus, "celt_sources.mk"))
    s = read_mk(os.path.join(opus, "silk_sources.mk"))
    opus_files = o["OPUS_SOURCES"] + o["OPUS_SOURCES_FLOAT"] + c["CELT_SOURCES"] + s["SILK_SOURCES"] + s["SILK_SOURCES_FLOAT"]
    opus_files = [os.path.join(opus, f) for f in opus_files]

    webp_files = []
    for d in ("dec", "dsp", "enc", "utils"):
        for f in sorted(os.listdir(os.path.join(webp, "src", d))):
            if not f.endswith(".c"):
                continue
            if any(k in f for k in ("neon", "mips", "avx2")):
                continue
            webp_files.append(os.path.join(webp, "src", d, f))

    opusfile_files = [os.path.join(TP, "opusfile", "src", f) for f in ("info.c", "internal.c", "opusfile.c", "stream.c")]
    ogg_files = [os.path.join(ogg, "src", f) for f in ("bitwise.c", "framing.c")]

    ft = os.path.join(TP, "freetype", "src")
    # the modules of the default ftmodule.h (what builds/unix compiles), each as its one source file
    freetype_files = [os.path.join(ft, f) for f in (
        "base/ftsystem.c", "base/ftdebug.c", "base/ftinit.c", "base/ftbase.c", "base/ftbbox.c", "base/ftbitmap.c",
        "base/ftfntfmt.c", "base/ftfstype.c", "base/ftgasp.c", "base/ftglyph.c", "base/ftgxval.c", "base/ftlcdfil.c",
        "base/ftmm.c", "base/ftotval.c", "base/ftpatent.c", "base/ftpfr.c", "base/ftstroke.c", "base/ftsynth.c",
        "base/fttype1.c", "base/ftwinfnt.c", "autofit/autofit.c", "bdf/bdf.c", "cff/cff.c", "cid/type1cid.c",
        "gzip/ftgzip.c", "lzw/ftlzw.c", "pcf/pcf.c", "pfr/pfr.c", "psaux/psaux.c", "pshinter/pshinter.c",
        "psnames/psmodule.c", "raster/raster.c", "sfnt/sfnt.c", "smooth/smooth.c", "truetype/truetype.c", "type1/type1.c",
        "type42/type42.c", "winfonts/winfnt.c")]

    hb = os.path.join(TP, "harfbuzz", "src")
    h = read_mk(os.path.join(hb, "Makefile.sources"))
    # the library as the original has it: the base and the OpenType shapers, the UCDN Unicode functions and the FreeType glue
    harfbuzz_files = [os.path.join(hb, f) for f in h["HB_BASE_sources"] + h["HB_FALLBACK_sources"] + h["HB_OT_sources"] +
                      h["HB_FT_sources"] + h["HB_UCDN_sources"] if f.endswith(".cc")]
    harfbuzz_files.append(os.path.join(hb, "hb-ucdn", "ucdn.c"))

    opus_inc = [os.path.join(opus, d) for d in ("include", "celt", "silk", "silk/float", "src")]
    return {
        "ogg": (ogg_files, [os.path.join(ogg, "include"), gen], []),
        "opus": (opus_files, opus_inc, ["-DOPUS_BUILD", "-DUSE_ALLOCA", "-DHAVE_LRINT", "-DHAVE_LRINTF", "-DFLOAT_APPROX",
                                        '-DPACKAGE_VERSION="1.1"', "-Wno-unused-variable"]),
        "opusfile": (opusfile_files, [os.path.join(TP, "opusfile", "include"), os.path.join(opus, "include"),
                                      os.path.join(ogg, "include"), gen], ["-DOP_HAVE_LRINTF", "-DOP_ENABLE_ASSERTIONS=0"]),
        "freetype": (freetype_files, [os.path.join(TP, "freetype", "include")], ["-DFT2_BUILD_LIBRARY"]),
        "harfbuzz": (harfbuzz_files, [hb, os.path.join(hb, "hb-ucdn"), os.path.join(TP, "freetype", "include")],
                     ["-DHAVE_OT", "-DHAVE_FALLBACK", "-DHAVE_UCDN", "-DHAVE_FREETYPE", "-DHB_NO_MT"]),
        "webp": (webp_files, [os.path.join(webp, "src"), webp], []),
    }


def flags_for(src):
    flags = []
    if src.endswith("_sse2.c"):
        flags.append("-msse2")
    if src.endswith("_sse41.c"):
        flags.append("-msse4.1")
    return flags


def compile_one(item):
    src, obj, incs, defs = item
    os.makedirs(os.path.dirname(obj), exist_ok=True)
    if os.path.exists(obj) and os.path.getmtime(obj) >= os.path.getmtime(src):
        return None
    cmd = [CXX if src.endswith(".cc") else CC, "-O2", "-w", "-c", src, "-o", obj] + defs + flags_for(src)
    if src.endswith(".cc"):
        cmd += ["-std=c++11", "-fno-exceptions", "-fno-rtti", "-fno-threadsafe-statics"]
    for i in incs:
        cmd += ["-I", i]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        return src + "\n" + (r.stderr or r.stdout)[:2000]
    return None


def main():
    names = [a for a in sys.argv[1:] if not a.startswith("-")]
    all_libs = libs()
    names = names or list(all_libs)
    failed = False
    for name in names:
        files, incs, defs = all_libs[name]
        jobs = [(f, os.path.join(OUT, name, os.path.basename(os.path.dirname(f)) + "_" + os.path.basename(f)[:-2] + ".o"), incs, defs) for f in files]
        with ThreadPoolExecutor(12) as ex:
            for err in ex.map(compile_one, jobs):
                if err:
                    failed = True
                    print("FAILED", err)
        if failed:
            sys.exit(1)
        lib = os.path.join(OUT, "lib" + name + ".a")
        if os.path.exists(lib):
            os.remove(lib)
        subprocess.run([AR, "rcs", lib] + [j[1] for j in jobs], check=True)
        print("built", lib)


main()
