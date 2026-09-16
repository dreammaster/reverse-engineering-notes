"""
Names sub_26C0E, called from LoadNextContainerInChain and sub_2621C
(the container-interaction input handler): a minimal "commit" step --
just FileEntry_Write(errorCode=0xB, bx=0x8FFB) + ErrorCheck, with no
descriptor setup of its own (unlike the similarly-shaped
SyncContainerContents, which first configures the write descriptor via
sub_27E3A). Assumes the caller already left the container-write
descriptor configured. -> CommitContainerWrite

Run via:
    .\run_ida_script.ps1 name_commit_container_write.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x26C0E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CommitContainerWrite", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CommitContainerWrite': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Minimal write-commit: FileEntry_Write(errorCode=0xB) + ErrorCheck, "
    "assuming the caller already configured the container-write "
    "descriptor (unlike SyncContainerContents, which configures it "
    "itself via sub_27E3A). Called from LoadNextContainerInChain and "
    "sub_2621C.",
    False,
)
