"""
Read-only check: for each of the 32 spell-effect addresses gathered by
dump_spell_tables.py, report whether IDA already recognizes it as a
function start, whether it already has a non-default name, and flag
any address collisions between the wizard/cleric tables before any
renames are attempted.
"""

import idc
import ida_bytes
import ida_funcs
import ida_name

WIZARD_SPELL_TABLE = "WIZARD_SPELL_TABLE"
CLERIC_SPELL_TABLE = "CLERIC_SPELL_TABLE"
COUNT = 16


def entries(table_name):
    start = idc.get_name_ea_simple(table_name)
    out = []
    for i in range(COUNT):
        ptr = ida_bytes.get_word(start + i * 2)
        out.append(0x10000 + ptr)
    return out


def main():
    wiz = entries(WIZARD_SPELL_TABLE)
    clr = entries(CLERIC_SPELL_TABLE)

    seen = {}
    for label, lst in (("W", wiz), ("C", clr)):
        for i, ea in enumerate(lst):
            seen.setdefault(ea, []).append(f"{label}{i}")

    print("Address collisions (same effect addr used by >1 slot):")
    for ea, tags in seen.items():
        if len(tags) > 1:
            print(f"  {ea:#x}: {tags}")

    print("\nPer-address function/name status:")
    for label, lst in (("WIZARD", wiz), ("CLERIC", clr)):
        for i, ea in enumerate(lst):
            f = ida_funcs.get_func(ea)
            is_func_start = f is not None and f.start_ea == ea
            nm = ida_name.get_name(ea)
            print(f"  {label}[{i:2}] {ea:#x}: func_start={is_func_start} name={nm!r}")


if __name__ == "__main__":
    main()
