"""
Names sub_1E4AA and sub_1E4D6, called from RunAlchemyScreen at
different points: draw one of two alchemy-screen icon states (picture
6 vs 5) at the same fixed position (0x102, 0x43).

sub_1E4AA (-> ShowAlchemyIconActive): also shows message 1 (via
sub_28412) before drawing picture 6.

sub_1E4D6 (-> ShowAlchemyIconIdle): draws picture 5 only, called from
2 sites.

Exact narrative (what these icon states represent -- e.g. brewing vs.
idle) not confirmed; named by their confirmed mechanism.

Run via:
    .\run_ida_script.ps1 name_alchemy_icon_states.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1E4AA: "ShowAlchemyIconActive",
    0x1E4D6: "ShowAlchemyIconIdle",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1E4AA,
    "Draws alchemy-screen picture 6 at (0x102,0x43), plus message 1. "
    "Called from RunAlchemyScreen. Exact narrative (vs. "
    "ShowAlchemyIconIdle) not confirmed.",
    False,
)
ida_bytes.set_cmt(
    0x1E4D6,
    "Draws alchemy-screen picture 5 at (0x102,0x43) -- sibling of "
    "ShowAlchemyIconActive. Called from RunAlchemyScreen (2 sites).",
    False,
)
