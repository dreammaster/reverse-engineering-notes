#!/usr/bin/env python3
"""Makes graphicslib/builtinShaders.cpp (the GLSL of the shaders that are built into the Linux executable of a Visionaire game) from the
directory that tools/extract_builtin_shaders.py wrote.
Usage: tools/gen_builtin_shaders.py <dir with the .bin files> <output .cpp>

A .bin file is "VSBIN\\n" and sections `api;stage;length of the attributes;attributes;length of the code;code` (the apis are glsl,
hlsl3, hlsl4 ...; the stages vs and ps). Only the glsl sections are used."""
import os
import sys


def parse(b):
    assert b.startswith(b"VSBIN\n")
    i = 6
    out = []
    while i < len(b):
        def field():
            nonlocal i
            j = b.index(b";", i)
            v = b[i:j]
            i = j + 1
            return v
        api = field().decode()
        stage = field().decode()
        alen = int(field())
        attrs = b[i:i + alen]
        i += alen
        dlen = int(field())
        data = b[i:i + dlen]
        i += dlen
        out.append((api, stage, attrs, data))
        if i < len(b) and b[i:i + 1] == b"\n":
            i += 1
    return out


def main():
    src, dst = sys.argv[1], sys.argv[2]
    lines = ["// Made by tools/gen_builtin_shaders.py from the shaders built into the Linux executable (`builtins` of `BuiltinShader()`): the GLSL of",
             "// each, as the original's `GLShader::CompileFromMemory()` takes it (the sections \"glsl\", \"vs\" and \"ps\").",
             '#include "graphicslib/builtinShaders.h"', "", "const TBuiltinShader g_builtinShaders[] = {"]
    count = 0
    for name in sorted(os.listdir(src)):
        parts = {}
        for api, stage, attrs, data in parse(open(os.path.join(src, name), "rb").read()):
            if api == "glsl":
                parts[stage] = data.decode("latin1")
        if "vs" not in parts or "ps" not in parts:
            continue
        for stage in ("vs", "ps"):
            assert ')GLSL"' not in parts[stage]
        lines.append('\t{"%s",' % name)
        lines.append('\t R"GLSL(%s)GLSL",' % parts["vs"])
        lines.append('\t R"GLSL(%s)GLSL"},' % parts["ps"])
        count += 1
    lines.append("\t{nullptr, nullptr, nullptr}")
    lines.append("};")
    lines.append("")
    lines.append("const int g_builtinShaderCount = %d;" % count)
    open(dst, "w", encoding="latin1", newline="\n").write("\n".join(lines) + "\n")
    print(count, "shaders")


main()
