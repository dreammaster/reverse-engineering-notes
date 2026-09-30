"""Convert immediate operands that are string offsets into offset references (whole database)."""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_ida_common import *
print("string offsets resolved:", resolve_string_offsets(0x10000, OVL_B[1]))
