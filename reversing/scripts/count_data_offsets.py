"""
Mechanically computes byte offsets for every labeled global in a slice of
rob_blanc_1.asm's .data section, by parsing db/dw/dd declarations (dup()
counts, align directives) and summing their sizes -- rather than trusting
a loop bound or a pre-existing label's declared extent alone (both have
been shown, in this project, to be necessary-but-not-sufficient evidence
for a field's true size/position; see reversing/notes/struct-layout-
drift.md's `defpal` retraction).

Usage: extract the relevant line range from rob_blanc_1.asm first (it's
too big to load whole), e.g.:

    sed -n '639954,700000p' rob_blanc_1.asm > /tmp/region.txt

then run:

    python count_data_offsets.py /tmp/region.txt

Prints the running byte offset for every labeled declaration in the
region (relative to the FIRST label in the file), and reports any line
it couldn't parse (should be zero for a clean .data region -- an
unhandled line means an unrecognized directive, and any offsets after
it should not be trusted until it's handled).

This was used to confirm rob_blanc_1's `game_gamename` global exactly
matches `OriGameSetupStruct` (Common/acroom.h:2769) -- see the "MAJOR
FINDING" section of struct-layout-drift.md for the full writeup, and
the offsets it found landed exactly on every previously-confirmed
struct-field anchor as a validation check before trusting it for new
fields.
"""
import re
import sys


def parse_dup_count(s):
    m = re.match(r'\s*([0-9A-Fa-f]+)h\s+dup\(', s)
    if m:
        return int(m.group(1), 16)
    return None


LABEL_RE = re.compile(r'^(\S+)\s+(db|dw|dd|dq)\s+(.*)$')
CONT_RE = re.compile(r'^\s+(db|dw|dd|dq)\s+(.*)$')
ALIGN_RE = re.compile(r'^\s*align\s+([0-9A-Fa-f]+)h?\s*$', re.IGNORECASE)
UNIT = {'db': 1, 'dw': 2, 'dd': 4, 'dq': 8}


def count_offsets(lines, base_addr=None):
    """
    base_addr: the TRUE absolute address of the first label in `lines`, if
    known. `align N` directives align the *absolute* address, not the
    running relative offset -- if base_addr is not a multiple of N, aligning
    the relative offset directly gives the WRONG answer. Omitting base_addr
    silently falls back to relative-offset alignment, which is only correct
    if the true base happens to be a multiple of every alignment used (do
    not rely on this -- always pass the real base_addr when one is known).
    """
    offset = 0
    unhandled = []
    anchors = []

    for i, raw in enumerate(lines):
        line = raw.rstrip('\n')
        stripped = line.strip()
        if not stripped:
            continue

        m_align = ALIGN_RE.match(line)
        if m_align:
            n = int(m_align.group(1), 16)
            if base_addr is not None:
                abs_addr = base_addr + offset
                aligned_abs = ((abs_addr + n - 1) // n) * n
                offset = aligned_abs - base_addr
            else:
                offset = ((offset + n - 1) // n) * n
            continue

        m = LABEL_RE.match(line)
        label = None
        if m:
            label, directive, rest = m.group(1), m.group(2), m.group(3)
        else:
            m2 = CONT_RE.match(line)
            if not m2:
                # not a data declaration -- a bare comment, a DATA XREF
                # continuation line, etc. Only truly safe to skip if it
                # doesn't itself start a new declaration.
                if stripped.startswith(';') or 'DATA XREF' in line or '↑' in line:
                    continue
                unhandled.append((i, line))
                continue
            directive, rest = m2.group(1), m2.group(2)

        unit = UNIT[directive]
        rest_nc = rest.split(';')[0].strip()
        dupcount = parse_dup_count(rest_nc)
        if dupcount is not None:
            size = dupcount * unit
        elif rest_nc == '?':
            size = unit
        elif rest_nc.startswith("'"):
            parts = re.findall(r"'[^']*'|[0-9A-Fa-fh]+", rest_nc)
            total = 0
            for p in parts:
                total += (len(p) - 2) if p.startswith("'") else 1
            size = total * unit
        else:
            items = [x for x in rest_nc.split(',') if x.strip()]
            if not items:
                unhandled.append((i, line))
                continue
            size = len(items) * unit

        start = offset
        offset += size
        if label:
            anchors.append((label, start, offset))

    return offset, anchors, unhandled


if __name__ == '__main__':
    path = sys.argv[1]
    base_addr = int(sys.argv[2], 16) if len(sys.argv) > 2 else None
    lines = open(path, encoding='utf-8', errors='replace').read().splitlines()
    final_offset, anchors, unhandled = count_offsets(lines, base_addr=base_addr)

    print('FINAL OFFSET:', hex(final_offset), final_offset)
    print()
    print('Unhandled lines:', len(unhandled))
    for i, l in unhandled[:40]:
        print(i, repr(l))
    print()
    for label, start, end in anchors:
        print(f'{label:24s} +0x{start:05X}..+0x{end:05X}  size {end - start}')
