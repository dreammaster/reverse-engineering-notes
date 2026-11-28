#!/usr/bin/env python3
"""The source trees under src/ are named after the engine they reconstruct, not after a game that happens to use it:

    src/vis-<release of the engine>-<year of the build>/        (see the README.md in each)

The release is the one of the string "Engine Version: %s (Build %d from Build date: %s)", the year the one of the build date: two
builds of one release can differ (Deponia 1 and Deponia 4 are both 5.0.1 / build 1189, built in 2019 and in 2017, and their code is
not the same). `resolve(name)` gives the name of a tree from what a tool is given: the name itself, or a part of it that makes one
tree ('5.0.3', '5.0.1-2019', 'vis-4.2.5').

    python tools/vis_engines.py             lists the trees
    python tools/vis_engines.py <name>      prints the name of the tree (what tools/build.sh does for its argument)
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# The tree the tools use when they are given none: the reference of the reconstruction (Deponia 1, Linux, 5.0.1 built in 2019).
DEFAULT = "vis-5.0.1-2019"

# The trees whose engine has Lua 5.4 linked in (third_party/lua54), the others use the Lua 5.1 of ScummVM.
LUA54 = ("vis-5.0.3-2025",)


def trees():
    src = os.path.join(ROOT, "src")
    return sorted(d for d in os.listdir(src) if os.path.isdir(os.path.join(src, d)))


def resolve(name, exact=False):
    """The name of the tree for `name` (None if there is none and `exact` is set; else the program stops with a message)."""
    if not name:
        return DEFAULT
    found = trees()
    if name in found:
        return name
    for prefix in ("vis-", ""):
        matches = [t for t in found if t.startswith(prefix + name) or t.startswith("vis-" + name)]
        matches = sorted(set(matches))
        if len(matches) == 1:
            return matches[0]
        if len(matches) > 1:
            sys.exit("'%s' is not one source tree: %s" % (name, ", ".join(matches)))
    if exact:
        return None
    sys.exit("no source tree '%s' under src/ (have: %s)" % (name, ", ".join(found)))


if __name__ == "__main__":
    if len(sys.argv) > 1:
        print(resolve(sys.argv[1]))
    else:
        for t in trees():
            print(t + ("   (default)" if t == DEFAULT else ""))
