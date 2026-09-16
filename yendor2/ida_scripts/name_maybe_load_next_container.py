"""
Names sub_18FC5, called 9 times from sub_18C79: a small guard --
if the item's catalog record has the confirmed "is container" flag
([+0xC] bit 0x2000) set, and dx (an input flag) is 0, calls
LoadNextContainerInChain and stores its result back into dx. A no-op
otherwise. Reused repeatedly within sub_18C79, which processes
multiple container slots. -> TryLoadNextContainerLink

Run via:
    .\run_ida_script.ps1 name_maybe_load_next_container.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x18FC5
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryLoadNextContainerLink", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryLoadNextContainerLink': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "If the item is a container ([+0xC] bit 0x2000) and dx==0, calls "
    "LoadNextContainerInChain and stores the result in dx; no-op "
    "otherwise. Called 9 times from sub_18C79.",
    False,
)
