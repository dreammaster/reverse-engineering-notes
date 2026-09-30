"""Define NUL-terminated ASCII strings (>= 4 chars) in the initialised part of DGROUP
(0..546Ch; the rest is BSS, zeroed by the C startup).  Only touches undefined bytes."""
import os, sys
import ida_bytes, idc
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mm2_ida_common import *

INIT_END = DGROUP[0] + 0x546C
ea, made = DGROUP[0], 0
while ea < INIT_END:
    n = 0
    while ea + n < INIT_END and (0x20 <= ida_bytes.get_byte(ea + n) < 0x7F or ida_bytes.get_byte(ea + n) in (9, 10, 13)):
        n += 1
    if n >= 4 and ida_bytes.get_byte(ea + n) == 0 and all(ida_bytes.is_unknown(ida_bytes.get_flags(ea + i)) for i in range(n + 1)):
        if ida_bytes.create_strlit(ea, n + 1, idc.STRTYPE_C):
            made += 1
        ea += n + 1
    else:
        ea += max(n, 1)
print("strings defined:", made)
