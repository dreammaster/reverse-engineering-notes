"""Rename Disk1 (save file) fields in whichever IDB is open (FOD.EXE and KEH.EXE share the layout)."""
import ida_struct
NAMES = {0x00: "saveCounterLo", 0x02: "saveCounterHi", 0x04: "regionAndSpeed", 0x07: "uniqueItems",
         0x38: "facing", 0x0E: "cash"}
sid = ida_struct.get_struc_id("Disk1")
sptr = ida_struct.get_struc(sid)
for off, nm in NAMES.items():
    m = ida_struct.get_member(sptr, off)
    if m and m.soff == off and ida_struct.get_member_name(m.id) != nm and not ida_struct.get_member_name(m.id).startswith("_"):
        print("rename +%X %s -> %s" % (off, ida_struct.get_member_name(m.id), nm))
        ida_struct.set_member_name(sptr, off, nm)
