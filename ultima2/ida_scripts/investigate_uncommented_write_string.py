"""Diagnostic: dump raw bytes for a sample of the 36 empty-looking
write_string strings to understand what they actually contain."""

import ida_bytes
import ida_ua
import idc

ADDRS = [0x1382A, 0x138EB, 0x13923, 0x13A12, 0x13C10]

for call_ea in ADDRS:
    insn = ida_ua.insn_t()
    ida_ua.decode_insn(insn, call_ea)
    str_start = call_ea + insn.size
    item_len = ida_bytes.get_item_size(str_start)
    raw = ida_bytes.get_bytes(str_start, item_len)
    print(f"{call_ea:X}: str_start={str_start:X} item_len={item_len} raw={raw!r}")
