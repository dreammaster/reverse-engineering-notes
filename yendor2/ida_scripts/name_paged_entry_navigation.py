"""
Names sub_133EB, called from ShowPagedEntryScreen: the page-navigation
input handler. 'I' key (or mouse region-table 0x6960 hit 1) goes to
the previous page (word_3293A--, gated on word_328CC bit 0x100,
errorCode=1). 'Q' key (or hit 2) goes to the next page (word_3293A++,
errorCode=2), gated on word_328CC bit 0x80 and a registration check
(word_328CA bit 1 or word_3293A<=5) -- showing
ShowClueBookRegistrationNag instead when unregistered and past page 5.
-> HandlePagedEntryNavigation

Run via:
    .\run_ida_script.ps1 name_paged_entry_navigation.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x133EB
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandlePagedEntryNavigation", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandlePagedEntryNavigation': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Page-navigation input for ShowPagedEntryScreen: 'I'/hit-1 -> "
    "previous page (word_3293A--, errorCode=1); 'Q'/hit-2 -> next page "
    "(word_3293A++, errorCode=2), gated on a registration check "
    "(shows ShowClueBookRegistrationNag past page 5 when unregistered).",
    False,
)
