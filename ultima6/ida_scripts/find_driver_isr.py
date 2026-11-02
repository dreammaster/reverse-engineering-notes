"""
The sound drivers install their player as the timer interrupt: snd_play does
`mov dx, <offset>; mov ax, 2508h; int 21h`.  Nothing calls that offset, so analysis never
reaches it.  Find the offset, make it a function and name it snd_timer_isr.

    .\run_ida_script.ps1 -Idb u6_snd_adlib -ScriptName find_driver_isr.py
"""

import idautils
import idc
import ida_auto
import ida_funcs
import ida_name

found = None
for ea in idautils.Heads(0, idc.get_segm_end(0)):
    if idc.print_insn_mnem(ea) == "mov" and idc.print_operand(ea, 0) == "ax" and (idc.get_operand_value(ea, 1) & 0xFFFF) == 0x2508:
        prev = idc.prev_head(ea)
        if idc.print_insn_mnem(prev) == "mov" and idc.print_operand(prev, 0) == "dx":
            found = idc.get_operand_value(prev, 1) & 0xFFFF
if found is not None:
    idc.del_items(found, 0, 4)
    ida_funcs.add_func(found)
    ida_name.set_name(found, "snd_timer_isr", ida_name.SN_NOCHECK | ida_name.SN_NOWARN)
    ida_auto.auto_wait()
    print("timer interrupt handler at %04X" % found)
else:
    print("no INT 08h hook found")
