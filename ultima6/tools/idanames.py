"""Reads the IDA listings (md/*.asm, se/*.asm) with the names the apply_renames_*.py scripts have given put back to IDA's own sub_XXXXX / word_XXXXX form.

The tools that parse a listing (emulate_intro.py, gen_md_create.py, trace_calls.py, md_pretty.py, md_findstr.py) were written against the automatic names; reading the file through
`read_asm` keeps them working whatever has been renamed since.  The other direction is for people: `idanames.py sub_2C86D` or `idanames.py worktype_berserk` prints the name, the address and
the note of the entry in the master lists (the game is chosen by --game md|se|md_martian|se_savage, default: all).

    from idanames import read_asm
    lines = read_asm('md/md_martian.asm').split('\\n')
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPTS = os.path.join(os.path.dirname(HERE), 'ida_scripts')
ENTRY = re.compile(r'^\s*\(0x([0-9A-Fa-f]+),\s*"([A-Za-z_][A-Za-z_0-9]*)",\s*(.*)\),?\s*$')


def master_lists():
    """{script stem: [(address, name, note, is_global)]} from every apply_renames_*.py."""
    out = {}
    for fn in sorted(os.listdir(SCRIPTS)):
        if not (fn.startswith('apply_renames_') and fn.endswith('.py')):
            continue
        entries, in_globals = [], False
        for line in open(os.path.join(SCRIPTS, fn), encoding='utf-8'):
            if line.startswith('GLOBALS'):
                in_globals = True
            elif line.startswith('FUNCTIONS'):
                in_globals = False
            m = ENTRY.match(line)
            if m:
                entries.append((int(m.group(1), 16), m.group(2), m.group(3).strip('\'" '), in_globals))
        out[fn[len('apply_renames_'):-3]] = entries
    return out


def read_asm(path):
    """The text of a listing with the renamed identifiers put back (functions to sub_XXXXX, globals to byte_/word_/dword_XXXXX)."""
    text = open(path, encoding='utf-8', errors='replace').read()
    stem = os.path.splitext(os.path.basename(path))[0]
    entries = master_lists().get(stem, [])
    if not entries:
        return text
    kinds = {'db': 'byte_', 'dw': 'word_', 'dd': 'dword_', 'dq': 'qword_'}
    for ea, name, _, is_global in entries:
        if is_global:
            m = re.search(r'^%s\s+(db|dw|dd|dq)\b' % re.escape(name), text, re.M)
            if not m:
                continue
            old = '%s%05X' % (kinds[m.group(1)], ea)
        else:
            old = 'sub_%05X' % ea
            if re.search(r'^%s\s+proc' % old, text, re.M):  # not renamed in this listing
                continue
        text = re.sub(r'\b%s\b' % re.escape(name), old, text)
    return text


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    game = None
    if '--game' in sys.argv:
        game = sys.argv[sys.argv.index('--game') + 1]
        args = [a for a in args if a != game]
    if not args:
        print(__doc__)
        return
    key = args[0]
    for stem, entries in master_lists().items():
        if game and not stem.endswith(game):
            continue
        for ea, name, note, is_global in entries:
            if key == name or key.lower() in ('sub_%05x' % ea, '%05x' % ea, '%x' % ea):
                print('%s: %s  %s%05X  %s' % (stem, name, 'data ' if is_global else 'sub_', ea, note))


if __name__ == '__main__':
    main()
