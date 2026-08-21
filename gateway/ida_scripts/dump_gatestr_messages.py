"""
Standalone Python helper (NOT an IDA script -- no idat.exe, no .idb
needed): decodes real message text directly out of a GATESTR.DAT file,
using the format traced in docs/file-formats.md (sections, per-string
Huffman compression against a shared global tree, a "common strings"
symbol extension for whole words/phrases).

Confirmed correct against the real file at c:\\games\\gw\\GATESTR.DAT --
used to settle the exact meaning of several ambiguous globals/functions
by reading their actual printed text rather than guessing from code
shape (e.g. this is how Score_add and the _score/_turnCount/_gameTicks
globals got their real names -- see docs/overview.md's "scoring
subsystem" section). Same technique as the sibling ultima1 project's
ida_scripts/dump_msg_strings.py, generalized into a real decoder here
since GATESTR.DAT needed actual decompression, not just a dseg-relative
string read.

Usage (plain Python, no IDA):
    python dump_gatestr_messages.py <path-to-GATESTR.DAT> <msgId> [<msgId> ...]

msgId may be decimal or 0x-prefixed hex, matching the packed
(sectionId << 10) | indexWithinSection scheme documented in
file-formats.md.
"""

import struct
import sys


def load(path):
    with open(path, "rb") as f:
        data = f.read()

    pos = 0

    def u16():
        nonlocal pos
        v = struct.unpack_from("<H", data, pos)[0]
        pos += 2
        return v

    sectionsCount = u16()
    sections = [(u16(), u16()) for _ in range(sectionsCount)]
    sectionsOffset = pos

    total_strings = sum(s[0] for s in sections)
    pos = sectionsOffset + total_strings * 2

    huffmanTableSize = u16()
    huffmanTable = list(struct.unpack_from(f"<{huffmanTableSize}h", data, pos))
    pos += huffmanTableSize * 2

    commonStringsCount = u16()
    commonStrings = []
    if commonStringsCount != 0:
        offsets = list(struct.unpack_from(f"<{commonStringsCount}H", data, pos))
        pos += commonStringsCount * 2
        tableSize = u16()
        commonData = data[pos:pos + tableSize]
        pos += tableSize
        for off in offsets:
            end = commonData.index(b"\0", off)
            commonStrings.append(commonData[off:end])

    strOffset2 = pos  # start of per-string compressed bitstreams

    return {
        "data": data,
        "sections": sections,
        "sectionsOffset": sectionsOffset,
        "huffmanTable": huffmanTable,
        "commonStrings": commonStrings,
        "strOffset2": strOffset2,
    }


def huffman_decompress(stream, table, symbols):
    out = bytearray()
    bitpos = 0
    total_bits = len(stream) * 8

    def get_bit():
        nonlocal bitpos
        byte = stream[bitpos // 8]
        bit = (byte >> (bitpos % 8)) & 1
        bitpos += 1
        return bit

    huff_base = len(table) - 2
    while bitpos < total_bits:
        bit = get_bit()
        val = table[huff_base | bit]
        if val >= 0:
            huff_base = val
            continue
        sym = -val - 1
        if sym < 0x80:
            out.append(sym & 0xFF)
        else:
            k = sym - 0x80
            out.extend(symbols[k] if k < len(symbols) else bytes([sym & 0xFF]))
        huff_base = len(table) - 2
    return bytes(out)


def get_message_bytes(gs, msg_id):
    section_id = msg_id >> 10
    subnum = msg_id & 0x3FF
    sections = gs["sections"]
    if section_id >= len(sections):
        return None
    strings_count, _ = sections[section_id]
    if subnum >= strings_count:
        return None
    data = gs["data"]
    lentab_off = gs["sectionsOffset"] + 2 * sum(s[0] for s in sections[:section_id])
    lengths = list(struct.unpack_from(f"<{strings_count}H", data, lentab_off))
    data_off = gs["strOffset2"] + sum(s[1] for s in sections[:section_id])
    data_off += sum(lengths[:subnum])
    this_len = lengths[subnum]
    stream = data[data_off:data_off + this_len]
    return huffman_decompress(stream, gs["huffmanTable"], gs["commonStrings"])


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"usage: {sys.argv[0]} <GATESTR.DAT path> <msgId> [<msgId> ...]")
        sys.exit(1)
    gs = load(sys.argv[1])
    print(f"{len(gs['sections'])} sections, "
          f"{sum(s[0] for s in gs['sections'])} total strings, "
          f"{len(gs['commonStrings'])} common-string dictionary entries")
    for arg in sys.argv[2:]:
        msg_id = int(arg, 0)
        text = get_message_bytes(gs, msg_id)
        print(f"msgId={msg_id:#x} (section={msg_id >> 10}, idx={msg_id & 0x3FF}) -> {text!r}")
