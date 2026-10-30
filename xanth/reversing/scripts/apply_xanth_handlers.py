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
idc.save_database(idc.get_idb_path(), 0)
print("renamed handlers", done)
if not ida_kernwin.is_idaq():
    idc.qexit(0)
