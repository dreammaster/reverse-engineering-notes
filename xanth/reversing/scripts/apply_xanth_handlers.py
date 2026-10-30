"""
IDAPython: names the Companions of Xanth object handlers.

call_object_handler (Xanth 0x1FB8A) calls the far pointer at table[handler_index - 1]; the table holds 448 entries (Shannara retail: 640 in two
512-entry halves) and starts at linear 0x34F80 (`off_34F84` is its second entry).  Each entry points at a 10-byte RTLink stub
(`call rtlink_thunk / jmp target`).  The stub is named objh_NNN_stub and the real handler objh_NNN (NNN = 1-based handler index, as in Shannara).

Idempotent.  Run with apply_to_idb.sh from the xanth folder:  bash reversing/scripts/apply_to_idb.sh xanth.idb reversing/scripts/apply_xanth_handlers.py
"""
import idc
import idautils
import ida_funcs
import ida_kernwin

TABLE = idc.get_name_ea_simple("off_34F84") - 4
COUNT = 448
done = 0
for i in range(COUNT):
    slot = TABLE + 4 * i
    refs = [r for r in idautils.DataRefsFrom(slot) if ida_funcs.get_func(r)]
    if not refs:
        print("WARN slot", i + 1, hex(slot), "has no code reference")
        continue
    stub = refs[0]
    target = stub
    # follow `call rtlink_thunk; jmp target`
    ea = stub
    for _ in range(4):
        mn = idc.print_insn_mnem(ea)
        if mn == "jmp":
            for t in idautils.CodeRefsFrom(ea, 0):
                tf = ida_funcs.get_func(t)
                if tf and tf.start_ea == t and t != stub:
                    target = t
            break
        ea = idc.next_head(ea)
        if ea == idc.BADADDR:
            break
    nm = "objh_%03d" % (i + 1)
    if target != stub:
        idc.set_name(stub, nm + "_stub", idc.SN_NOWARN)
    idc.set_name(target, nm, idc.SN_NOWARN)
    idc.set_func_cmt(target, "object handler %d (table entry at %x)" % (i + 1, slot), 0)
    done += 1
idc.set_name(TABLE, "object_handler_table", idc.SN_NOWARN)

# computed room exits: sub_1FB15(index, 0xF) calls off_34D6A[index] (56 far pointers to RTLink stubs); see dump_exits.py
EXITS = idc.get_name_ea_simple("off_34D6A")
if EXITS == idc.BADADDR:
    EXITS = idc.get_name_ea_simple("exit_function_table")
exits = 0
for k in range(0x38):
    refs = [r for r in idautils.DataRefsFrom(EXITS + 4 * k) if ida_funcs.get_func(r)]
    if not refs:
        continue
    stub = refs[0]
    target = stub
    for r in idautils.CodeRefsFrom(idc.next_head(stub), 0):
        tf = ida_funcs.get_func(r)
        if tf and tf.start_ea == r and r != stub:
            target = r
    if target != stub:
        idc.set_name(stub, "exit_fn_%02d_stub" % k, idc.SN_NOWARN)
    idc.set_name(target, "exit_fn_%02d" % k, idc.SN_NOWARN)
    idc.set_func_cmt(target, "computed room exit %d: returns the target room for exits of kind 3/4 whose arg is %d (called with 0xF); <= 0 = blocked" % (k, k), 0)
    exits += 1
idc.set_name(EXITS, "exit_function_table", idc.SN_NOWARN)
print("renamed exit functions", exits)

# conversation callbacks (dword_3DCA4[1..82]) and dynamic line texts (seg069[(strIdx-0x8000)*4], 90 entries); see dump_conversations.py
import ida_segment


def name_table(base, count, first, fmt, comment):
    n = 0
    for k in range(first, first + count):
        refs = [r for r in idautils.DataRefsFrom(base + 4 * k) if ida_funcs.get_func(r)]
        if not refs:
            continue
        stub = refs[0]
        target = stub
        for r in idautils.CodeRefsFrom(idc.next_head(stub), 0):
            tf = ida_funcs.get_func(r)
            if tf and tf.start_ea == r and r != stub:
                target = r
        if target != stub:
            idc.set_name(stub, fmt % k + "_stub", idc.SN_NOWARN)
        idc.set_name(target, fmt % k, idc.SN_NOWARN)
        idc.set_func_cmt(target, comment.replace("%d", str(k)), 0)
        n += 1
    return n


CB = idc.get_name_ea_simple("dword_3DCA4")
if CB == idc.BADADDR:
    CB = idc.get_name_ea_simple("conversation_callbacks")
c1 = name_table(CB, 83, 0, "conv_cb_%02d", "conversation menu callback %d: called as (convId, option index) after the player picks an option")
idc.set_name(CB, "conversation_callbacks", idc.SN_NOWARN)
S69 = ida_segment.get_segm_by_name("seg069").start_ea
c2 = name_table(S69, 90, 0, "conv_text_%02d", "dynamic conversation text %d (strIdx 0x8000+%d): returns the line's text pointer")
print("renamed conversation callbacks", c1, "text functions", c2)

# timed events: run_event (sub_1F070) calls the far pointer at seg_8AA18:[(id-1)*4] for id 1..0x2D; advance_time? runs it when an event's countdown expires
EV = idc.get_wide_word(idc.get_name_ea_simple("seg_8AA18")) * 16
c3 = name_table(EV, 45, 0, "event_fn_%02d", "timed event table entry %d (event id %d + 1 as used by schedule_event?/cancel_event; run_event calls it when the countdown expires)")
idc.set_name(EV, "event_function_table", idc.SN_NOWARN)
print("renamed event functions", c3)
idc.save_database(idc.get_idb_path(), 0)
print("renamed handlers", done)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
