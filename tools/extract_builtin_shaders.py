#!/usr/bin/env python3
"""Extracts the shaders that are built into the Linux executable of a Visionaire game (the table `builtins` of
`BuiltinShader(std::string &, const char *)`: name, zlib data, size of the data, size of the text) into a directory.
Usage: tools/extract_builtin_shaders.py <executable> <outdir>"""
import os
import struct
import sys
import zlib


def main():
    exe, out = sys.argv[1], sys.argv[2]
    data = open(exe, "rb").read()
    assert data[:4] == b"\x7fELF" and data[4] == 2
    shoff, = struct.unpack_from("<Q", data, 0x28)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x3A)
    phoff, = struct.unpack_from("<Q", data, 0x20)
    phentsize, phnum = struct.unpack_from("<HH", data, 0x36)
    sections = []
    for i in range(shnum):
        sections.append(struct.unpack_from("<IIQQQQIIQQ", data, shoff + i * shentsize))
    loads = []
    for i in range(phnum):
        ptype, flags, offset, vaddr, paddr, filesz, memsz, align = struct.unpack_from("<IIQQQQQQ", data, phoff + i * phentsize)
        if ptype == 1:
            loads.append((vaddr, offset, filesz))

    def fileoff(addr):
        for vaddr, offset, filesz in loads:
            if vaddr <= addr < vaddr + filesz:
                return offset + addr - vaddr
        raise KeyError(hex(addr))

    symtab = next(s for s in sections if s[1] == 2)
    strtab = sections[symtab[6]]
    symbols = {}
    for i in range(symtab[5] // 24):
        name, info, other, shndx, value, size = struct.unpack_from("<IBBHQQ", data, symtab[4] + i * 24)
        end = data.index(b"\0", strtab[4] + name)
        symbols[data[strtab[4] + name:end].decode("latin1")] = (value, size)

    builtins = symbols["builtins"]
    os.makedirs(out, exist_ok=True)
    count = 0
    off = fileoff(builtins[0])
    while True:
        namep, datap, ulenp, lenp = struct.unpack_from("<QQQQ", data, off)
        if not namep:
            break
        try:
            nameoff = fileoff(namep)
            blobcheck = fileoff(datap)
        except KeyError:
            break
        name = data[nameoff:data.index(b"\0", nameoff)].decode("latin1")
        ulen, = struct.unpack_from("<i", data, fileoff(ulenp))
        clen, = struct.unpack_from("<i", data, fileoff(lenp))
        blob = data[fileoff(datap):fileoff(datap) + clen]
        try:
            text = zlib.decompress(blob)
        except zlib.error:
            text = b""
        assert len(text) == ulen, (name, len(text), ulen)
        open(os.path.join(out, name), "wb").write(text)
        print(name, clen, ulen)
        count += 1
        off += 32
    print(count, "shaders")


main()
