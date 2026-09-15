"""
Names the F5 item-subtype-7 clue-book cluster: "TRANSPORTATIONS"
(dumped title msg 0x8A6A), listing PEGASUS / GIANT EAGLE / MAGIC
DRAGON (msgs 0x77C6/0x77E0/0x7814) -- ties back to
IsItemRangeAvailable's documented "boat/horse-style transport gate"
use case.

sub_1334E (-> RunClueBookTransportCategory): called from ShowClueBook
(item subtype 7). Draws via sub_13E98, polls input until ESC -- the
same simple shape as other single-screen categories.

sub_13E98 (-> ShowClueBookTransportDetail): message box + nav bar,
then draws the 3 named mounts via sub_13EDF (not traced -- likely a
per-mount stat-line drawer, called once per mount).

Run via:
    .\run_ida_script.ps1 name_cluebook_transport_category.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1334E: "RunClueBookTransportCategory",
    0x13E98: "ShowClueBookTransportDetail",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1334E,
    "F5 item-subtype-7 'TRANSPORTATIONS' clue-book category loop "
    "(called from ShowClueBook). Draws via "
    "ShowClueBookTransportDetail, polls input until ESC.",
    False,
)
ida_bytes.set_cmt(
    0x13E98,
    "'TRANSPORTATIONS' detail screen (msg 0x8A6A): message box + nav "
    "bar, then 3 named mounts -- PEGASUS, GIANT EAGLE, MAGIC DRAGON -- "
    "drawn via sub_13EDF (not traced, likely a per-mount stat-line "
    "drawer).",
    False,
)
