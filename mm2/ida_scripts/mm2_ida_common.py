"""Helpers shared by the MM2 IDA scripts (import via exec/sys.path from __file__)."""
import os
import sys

import ida_bytes
import ida_segment
import idc

_HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(os.path.dirname(_HERE), "tools"))
import mm2_layout  # noqa: E402

BASE = mm2_layout.IDA_BASE_PARA * 16          # linear address of load-base:0
CODE_SEL = 0x1000                             # IDA selector of the resident code segment
DGROUP_SEL = 0x1D85                           # relative 0D85h + IDA base
OVL_A = (0x17E10, 0x1C130)                    # overlay window shared by 1MENU2/2COMBAT/2PLAY
OVL_B = (0x1C130, 0x1D850)                    # overlay window shared by the 11 others
DGROUP = (0x1D850, 0x28220)                   # initialised data + BSS (0A9D0h bytes)
STACK = (0x28220, 0x28A20)


def layout():
    return mm2_layout.Layout(exe_path=idc.get_input_file_path())


def load_bytes(ea, blob):
    ida_bytes.put_bytes(ea, blob)


def apply_relocs(rec_relocs, body_ea, load_para_base):
    """Apply Plink86 fixups: each (offset, seg) adds the load base to the word at seg:offset."""
    n = 0
    for off, seg in rec_relocs:
        ea = (seg + mm2_layout.IDA_BASE_PARA) * 16 + off
        idc.patch_word(ea, (idc.get_wide_word(ea) + mm2_layout.IDA_BASE_PARA) & 0xFFFF)
        n += 1
    return n
