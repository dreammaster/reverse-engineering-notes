"""
Documents (comment only, no fabricated names) the ~27-function cluster at
0x27B42-0x2801A in seg096: each is a tiny far proc taking a FileEntry*
in bx (per the old analysis's SetType info on several of them, e.g.
sub_27CB0) that hardcodes one resource's location -- copies a fixed
dword (read from a small pointer table via `mov si, imm; mov ax,[si]`)
into FileEntry._blockOffset/_blockOffsetHi (struct offsets 0xA/0xC), and
sets FileEntry._blockSize (offset 6) either to a hardcoded constant or
computed from another global. Confirmed NOT a jump/dispatch table --
each one is `call`ed directly and individually from many different, and
different, subroutines scattered across the binary (see git blame on
sub_27CB0/sub_27D8A/sub_27FE0 callers), so this is "one function per
game resource/asset", not an indexed table.

Deliberately not renaming these individually: nothing in static analysis
distinguishes *which* resource each one represents (the pointer-table
constants are opaque without correlating against the actual WORLD.DAT/
PICTURES.VGA file layout or observing the game at runtime) -- the prior
IDA-8.3-era session's own analyst reached the same conclusion (all of
these were left as sub_XXXXX there too, despite having the FileEntry*
parameter type worked out). Naming them ShowErr_-style from guesswork
would be actively misleading given how explicitly the project notes
existing names shouldn't be trusted uncritically.

Run via:
    .\run_ida_script.ps1 document_resource_stubs.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x27CB0,
    "First of a ~27-function cluster (0x27B42-0x2801A, seg096) of tiny "
    "'resource block setup' stubs: each hardcodes one FileEntry's "
    "_blockOffset/_blockOffsetHi (from a small pointer table) and "
    "_blockSize for one specific game resource, then returns -- the "
    "caller does the actual FileEntry_Read. Each is called directly "
    "from many different, scattered call sites (not through a dispatch "
    "table), so which resource each one represents isn't recoverable "
    "from static analysis alone; left unnamed deliberately rather than "
    "guessed -- see ida_scripts/document_resource_stubs.py.",
    False,
)
print("done")
