"""
Names sub_2D65A with moderate confidence: one of HandleGameCommand's
fallback interaction handlers (triggered when the targeted object's
flags match a "container-like" bit pattern per HandleGameCommand's own
check). Checks a "needs confirmation" flag on the target (word_2E548's
+2 bit 0x200) and if set, shows ShowConfirmPrompt(msg=0x20) before
proceeding -- consistent with e.g. a locked/trapped container prompting
before opening. Not fully traced past that point (several more
sub-calls -- sub_2D809, sub_2C0FE -- not read this round).

-> InteractWithContainer (moderate confidence)

Also looked at sub_2D60A (HandleGameCommand's other fallback): searches
the SAME 0xDFBB table sub_294A3 (RunGameDialog's unresolved 0x242-0x245
handler) scans, matching on the target's type field -- ties these two
areas together but isn't independently resolved enough to name yet.

Run via:
    .\run_ida_script.ps1 name_container.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D65A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "InteractWithContainer", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'InteractWithContainer': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Moderate confidence: one of HandleGameCommand's fallback handlers "
    "for 'container-like' target flags. Checks a needs-confirmation bit "
    "on the target and prompts (ShowConfirmPrompt msg=0x20) before "
    "proceeding if set -- consistent with a locked/trapped container. "
    "Not fully traced past that point.",
    False,
)
