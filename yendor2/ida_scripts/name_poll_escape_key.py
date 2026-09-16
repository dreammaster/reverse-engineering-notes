"""
Names the byte-for-byte-identical pair sub_11900/sub_15249 -- another
instance of this session's overlay-segment duplicate-function pattern
(cf. DrawShadowedText/DrawShadowedTextAlt,
ConfirmContainerInteraction/ConfirmAlchemyInteraction).

Both: call PollKeyboardInput; if a key came in (errorCode!=0) and it
isn't ESC (byte_2E400 != 0x1B), discard it by clearing byte_2E400 to
0. Either way, return with ZF set iff byte_2E400==0x1B. In short: "poll
for a keypress, but only ESC matters -- everything else is silently
swallowed." sub_11900 is called 3x from the already-named
ShowIntroPicture; sub_15249 is the duplicate, called 3x from unnamed
sub_15429.

-> PollForEscapeKeyOnly / PollForEscapeKeyOnlyAlt

Run via:
    .\run_ida_script.ps1 name_poll_escape_key.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x11900: "PollForEscapeKeyOnly",
    0x15249: "PollForEscapeKeyOnlyAlt",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x11900,
    "Polls for a keypress but only cares about ESC: any other key is "
    "silently discarded (byte_2E400 cleared). Returns ZF set iff "
    "byte_2E400==0x1B. Called from ShowIntroPicture. Byte-for-byte "
    "identical to PollForEscapeKeyOnlyAlt (sub_15249), another "
    "overlay-segment duplicate.",
    False,
)
ida_bytes.set_cmt(
    0x15249,
    "Byte-for-byte duplicate of PollForEscapeKeyOnly (sub_11900), in "
    "a different overlay segment. Called from unnamed sub_15429.",
    False,
)
