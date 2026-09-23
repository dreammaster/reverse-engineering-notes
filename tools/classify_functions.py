#!/usr/bin/env python3
"""Survey tool: classify every function in an IDA .asm export as either part
of a known third-party/vendored library, or Visionnaire's own proprietary
code, and group the proprietary ones by owning class/namespace.

Why this exists: the game binaries are enormous (Deponia_Linux.asm alone is
~20k functions). Most of that is statically-linked open-source middleware
(SDL2, wxWidgets, LuaJIT+tolua++, Box2D, Spine, FreeType, HarfBuzz, Assimp,
libpng/libjpeg/libwebp, libvorbis+ogg, OpenAL, zlib, rapidxml, rapidjson,
PCRE, GLEW, ...) that we should link as real upstream libraries rather than
reimplement. This script separates that from the actual reverse-engineering
workload (Visionnaire's own classes) and groups the latter by owner class in
file order, which - per manual verification - tracks address order closely
enough that a class's methods land together, making "work one class/cluster
at a time" a viable strategy.

Usage:
    python tools/classify_functions.py <path-to-.asm> [--out-dir manifest]

Outputs (TSV, tab-separated so they diff/grep cleanly):
    manifest/vendor_libraries.tsv       one row per detected vendor library
    manifest/proprietary_classes.tsv    one row per proprietary class/namespace,
                                         in first-appearance (address) order
    manifest/proprietary_functions.tsv  one row per proprietary function,
                                         for fine-grained progress tracking
    manifest/unclassified_samples.tsv   sample of names that matched no vendor
                                         pattern and had no "::" owner, for
                                         manually spotting missed libraries

Re-run this whenever the vendor pattern table below grows; it's cheap
(a few seconds) and fully regenerates the manifest from the .asm, so the
manifest itself is safe to hand-edit for the `status`/`notes` columns - only
re-running with `--merge` (default) preserves those columns for rows that
still exist.
"""
import argparse
import re
import subprocess
import sys
from pathlib import Path

PROC_RE = re.compile(r"^(\S+)\s+proc (?:near|far)\b")

# (library name, regex matched against the RAW mangled/C symbol name)
VENDOR_PATTERNS = [
    ("SDL2", r"^SDL_"),
    ("SDL2 (X11 backend)", r"^X11_"),
    ("SDL2 (other backend)", r"^(Wayland_|EVDEV|ALSA_|PULSEAUDIO|DBus_|UDEV_|LINUX_|Xinput)"),
    ("wxWidgets", r"^_ZN\d+wx|^_ZNK\d+wx|^_ZTI\d+wx|^wxGetUserId"),
    ("Assimp", r"^_ZN6Assimp|^_ZNK6Assimp"),
    ("libstdc++/STL", r"^_ZNSt|^_ZSt|^_ZNKSt|^_ZTISt|^_ZN9__gnu_cxx|^_ZNK9__gnu_cxx"),
    ("zlib", r"^(z_|compress|uncompress|inflate|deflate|zError|adler32|crc32)"),
    ("libpng", r"^png_"),
    ("FreeType (public API)", r"^FT_"),
    ("FreeType (internal: autofit/cff/truetype/etc)",
     r"^(af_|tt_|cff_|sfnt|psaux|t1_|cid_|bdf_|pcf_|pfr_|ps_hints|cf2_|ft_)"),
    ("libvorbis/ogg", r"^(ov_|vorbis|op_|res0_|floor|mapping0_|codebook)"),
    ("OpenAL", r"^(al[A-Z]|alc[A-Z])"),
    ("LuaJIT core", r"^lj_"),
    ("Lua/LuaJIT public API", r"^lua[_A-Z]"),
    ("tolua++ bindings", r"tolua_"),
    ("Box2D", r"^_ZN2b2|^b2[A-Z]"),
    ("Spine runtime", r"^_?sp[A-Z]|^_spine"),
    ("HarfBuzz", r"^hb_|^_ZN2OT|^_ZNK2OT|^_ZN11hb_"),
    ("jpgd (public-domain JPEG codec)", r"jpeg"),
    # Emit[A-Z] (not bare "Emit") to avoid matching the proprietary
    # particle-system `Emitter` class, which also lives in this address
    # range and starts with the same four letters.
    ("libwebp", r"^(VP8|vp8|WebP|Upsample|Emit[A-Z]|YUV)"),
    ("GLEW / OpenGL loader", r"^(gl[A-Z]|_glewInit|GLEW)"),
    ("libcurl", r"^curl_"),
    ("boost", r"boost"),
    ("rapidjson", r"rapidjson"),
    ("rapidxml", r"rapidxml"),
    ("PCRE", r"^pcre"),
    ("Scintilla (script editor widget)", r"^Scintilla|^SC[A-Z]|_CallTipShow"),
]
VENDOR_COMPILED = [(name, re.compile(pat)) for name, pat in VENDOR_PATTERNS]

# GCC/IDA-added disambiguation suffixes that break demangling; stripped only
# as a fallback when demangling the raw name fails outright.
SUFFIX_RE = re.compile(
    r"(\.\d+)?(_constprop_\d+|_isra_\d+|_part_\d+|_cold_\d+|_cold|_\d+)+$"
)


def demangle_all(names, cppfilt):
    proc = subprocess.run(
        [cppfilt], input="\n".join(names), capture_output=True, text=True, check=True
    )
    return proc.stdout.splitlines()


def find_owner(demangled: str):
    """Best-effort split of a demangled signature into (owner, member).
    owner is None for free functions / already-classified C symbols."""
    s = demangled
    m = re.match(r"^(?:virtual |non-virtual )?thunk(?: to)? ", s)
    if m:
        s = s[m.end():]

    # Find the first top-level '(' (bracket-depth-aware) - that's where the
    # parameter list starts, so everything before it is "[return type ]name".
    depth = 0
    paren_idx = None
    for i, ch in enumerate(s):
        if ch in "(<[":
            if ch == "(" and depth == 0:
                paren_idx = i
                break
            depth += 1
        elif ch in ")>]":
            depth = max(0, depth - 1)
    head = s[:paren_idx] if paren_idx is not None else s

    # Strip a top-level leading return type: split at the LAST top-level
    # space (depth-aware, so template commas/spaces don't confuse it).
    depth = 0
    last_space = -1
    for i, ch in enumerate(head):
        if ch in "(<[":
            depth += 1
        elif ch in ")>]":
            depth = max(0, depth - 1)
        elif ch == " " and depth == 0:
            last_space = i
    qualified = head[last_space + 1:] if last_space >= 0 else head

    # Split off the LAST top-level "::".
    depth = 0
    last_scope = -1
    i = 0
    while i < len(qualified) - 1:
        ch = qualified[i]
        if ch in "(<[":
            depth += 1
        elif ch in ")>]":
            depth = max(0, depth - 1)
        elif ch == ":" and qualified[i + 1] == ":" and depth == 0:
            last_scope = i
            i += 1
        i += 1
    if last_scope < 0:
        return None, qualified
    return qualified[:last_scope], qualified[last_scope + 2:]


def load_manifest_column(path: Path, key_col: int, value_cols):
    """Read an existing TSV manifest and return {key: {col_name: value}} for
    the given value column indices, so a re-run can preserve hand-edited
    status/notes columns."""
    if not path.exists():
        return {}
    out = {}
    with path.open(encoding="utf-8") as f:
        header = f.readline().rstrip("\n").split("\t")
        for line in f:
            parts = line.rstrip("\n").split("\t")
            if len(parts) <= key_col:
                continue
            key = parts[key_col]
            out[key] = {header[c]: parts[c] if c < len(parts) else "" for c in value_cols}
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("asm_path", type=Path)
    ap.add_argument("--out-dir", type=Path, default=Path("manifest"))
    ap.add_argument("--cppfilt", default="c++filt")
    args = ap.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)

    print(f"Scanning {args.asm_path} ...", file=sys.stderr)
    entries = []  # (line_no, raw_symbol)
    with args.asm_path.open(encoding="utf-8", errors="replace") as f:
        for line_no, line in enumerate(f, start=1):
            m = PROC_RE.match(line)
            if m:
                entries.append((line_no, m.group(1)))
    print(f"Found {len(entries)} functions.", file=sys.stderr)

    raw_names = [name for _, name in entries]
    demangled = demangle_all(raw_names, args.cppfilt)

    # Retry demangling for names c++filt left untouched, after stripping
    # known GCC/IDA suffixes.
    retry_idx = [i for i, (raw, dem) in enumerate(zip(raw_names, demangled)) if raw == dem]
    if retry_idx:
        stripped = [SUFFIX_RE.sub("", raw_names[i]) for i in retry_idx]
        redone = demangle_all(stripped, args.cppfilt)
        for j, i in enumerate(retry_idx):
            if redone[j] != stripped[j]:
                demangled[i] = redone[j]

    vendor_counts = {}
    vendor_samples = {}
    vendor_lines = {}
    proprietary_rows = []  # (line_no, raw, demangled, owner, member)
    unclassified_samples = []

    for (line_no, raw), dem in zip(entries, demangled):
        owner, member = find_owner(dem)
        # Match vendor patterns only against the raw mangled name and the
        # function's own qualified name (owner::member, no parameter/return
        # types) - matching the full demangled signature caused false
        # positives like a proprietary `soundengine::SyncBus::Serialize`
        # being classified as "rapidjson" merely because it takes a
        # `rapidjson::Value&` parameter.
        scope_text = f"{owner}::{member}" if owner else member
        vendor_hit = None
        for name, pat in VENDOR_COMPILED:
            # An Itanium-mangled `raw` name embeds its parameter types too,
            # so an *unanchored* pattern (e.g. "rapidjson") can match deep
            # inside an encoded parameter type even when the function itself
            # belongs to Visionnaire's own code (e.g. a proprietary
            # `soundengine::SyncBus::Serialize(rapidjson::Value&)`).
            # Anchored patterns (`^...`) only ever look at the start of the
            # string, which for a mangled name is always the function's own
            # namespace/class, so they're safe to test against `raw`
            # regardless. Unanchored patterns are only tested against
            # `scope_text`, which already excludes parameter/return types.
            raw_ok = pat.search(raw) if pat.pattern.startswith("^") else False
            if raw_ok or pat.search(scope_text):
                vendor_hit = name
                break
        if vendor_hit:
            vendor_counts[vendor_hit] = vendor_counts.get(vendor_hit, 0) + 1
            vendor_lines.setdefault(vendor_hit, [line_no, line_no])
            vendor_lines[vendor_hit][1] = line_no
            samples = vendor_samples.setdefault(vendor_hit, [])
            if len(samples) < 5:
                samples.append(dem)
            continue

        if owner is None:
            unclassified_samples.append((line_no, raw, dem))
            owner = "(free function)"
        proprietary_rows.append((line_no, raw, dem, owner, member))

    # --- vendor_libraries.tsv ---
    vpath = args.out_dir / "vendor_libraries.tsv"
    with vpath.open("w", encoding="utf-8") as f:
        f.write("library\tfunction_count\tfirst_line\tlast_line\tsample_names\n")
        for name, count in sorted(vendor_counts.items(), key=lambda kv: -kv[1]):
            lo, hi = vendor_lines[name]
            samples = " | ".join(vendor_samples[name])
            f.write(f"{name}\t{count}\t{lo}\t{hi}\t{samples}\n")
    print(f"Wrote {vpath}", file=sys.stderr)

    # --- proprietary_classes.tsv (grouped, first-appearance order) ---
    class_order = []
    class_info = {}
    for line_no, raw, dem, owner, member in proprietary_rows:
        if owner not in class_info:
            class_info[owner] = {"count": 0, "first": line_no, "last": line_no}
            class_order.append(owner)
        info = class_info[owner]
        info["count"] += 1
        info["first"] = min(info["first"], line_no)
        info["last"] = max(info["last"], line_no)

    cpath = args.out_dir / "proprietary_classes.tsv"
    existing = load_manifest_column(cpath, key_col=0, value_cols=[1])  # preserve "status"
    header_cols = ["owner", "status", "method_count", "first_line", "last_line"]
    with cpath.open("w", encoding="utf-8") as f:
        f.write("\t".join(header_cols) + "\n")
        for owner in sorted(class_order, key=lambda o: class_info[o]["first"]):
            info = class_info[owner]
            status = existing.get(owner, {}).get("status", "todo")
            f.write(f"{owner}\t{status}\t{info['count']}\t{info['first']}\t{info['last']}\n")
    print(f"Wrote {cpath} ({len(class_order)} owners)", file=sys.stderr)

    # --- proprietary_functions.tsv (fine-grained, for per-function tracking) ---
    fpath = args.out_dir / "proprietary_functions.tsv"
    existing_f = load_manifest_column(fpath, key_col=1, value_cols=[2])  # keyed by raw symbol, preserve "status"
    with fpath.open("w", encoding="utf-8") as f:
        f.write("line\traw_symbol\tstatus\towner\tmember\tdemangled\n")
        for line_no, raw, dem, owner, member in sorted(proprietary_rows, key=lambda r: r[0]):
            status = existing_f.get(raw, {}).get("status", "todo")
            f.write(f"{line_no}\t{raw}\t{status}\t{owner}\t{member}\t{dem}\n")
    print(f"Wrote {fpath} ({len(proprietary_rows)} functions)", file=sys.stderr)

    # --- unclassified_samples.tsv ---
    upath = args.out_dir / "unclassified_samples.tsv"
    with upath.open("w", encoding="utf-8") as f:
        f.write("line\traw_symbol\tdemangled\n")
        for line_no, raw, dem in unclassified_samples[:2000]:
            f.write(f"{line_no}\t{raw}\t{dem}\n")
    print(f"Wrote {upath} ({len(unclassified_samples)} free functions, first 2000 shown)", file=sys.stderr)

    print(
        f"\nTotals: {len(entries)} functions total, "
        f"{sum(vendor_counts.values())} vendor, "
        f"{len(proprietary_rows)} proprietary "
        f"({len(class_order)} distinct owners).",
        file=sys.stderr,
    )


if __name__ == "__main__":
    main()
