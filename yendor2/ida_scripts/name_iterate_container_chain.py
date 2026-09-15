"""
Names sub_26022, called from sub_18FC5 and sub_2621C (both not fully
traced): iterates a chain of container/world-object records.

If word_36E0F (a "next id in chain" pointer) is nonzero, reads a
CURGAME record for it (FileEntry bx=0x8FFB, errorCode=0xB) and
advances word_36E0F to the next id from that record's own [+8] field
-- a linked-list walk through CURGAME. Otherwise falls back to a
simple incrementing counter (word_36E0D). Either way, calls the
already-named LoadContainerContents for the resulting id, clears a
17-word buffer, and calls sub_26C0E (not traced).

Reads as "load the next container in a chain of linked container
records" -- e.g. multiple containers/chests found together.

-> LoadNextContainerInChain

Run via:
    .\run_ida_script.ps1 name_iterate_container_chain.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x26022
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "LoadNextContainerInChain", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'LoadNextContainerInChain': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Iterates a chain of container/world-object records: if "
    "word_36E0F (next-id pointer) is set, reads its CURGAME record and "
    "advances to the next id from that record's own [+8] field "
    "(linked-list walk); else uses a simple counter (word_36E0D). "
    "Loads the result via LoadContainerContents. Called from "
    "sub_18FC5 and sub_2621C.",
    False,
)
