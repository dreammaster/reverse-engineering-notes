"""
IDA Pro script: comments canMoveToTile's 4 vehicle-tile dispatch
comparisons against _playerTileId with their TileId enum equivalents.

Found while cross-checking the TileId enum against raw tile-ID
comparisons in canMoveToTile (docs/roadmap.md's "apply the enum where
unambiguous" item). The comparisons use decimal literals (34/36/38/40)
that are TileId x2 (matching the already-confirmed x2 encoding used
for _playerTileId elsewhere, e.g. board's vehicle-boarding checks) --
but the TileId enum itself stores the x1 base values (0x11-0x14), so
IDA's op_enum operand typing won't render these cleanly on its own
(no exact literal match). Comments instead of enum typing.

Scope: comment-only, not a rename or struct change, so it doesn't fit
apply_renames.py's or apply_structs.py's stated scope -- small enough
(4 sites) to not need its own accumulating master file. DRY_RUN=False
per this project's convention for direct, low-risk annotations (same
risk profile as a rename, easily reversible).

Addresses were hand-derived (not read live from the IDB) from label
positions and instruction byte-lengths, so each one is verified against
the actual decoded instruction (must be `cmp al, <expected>`) before
commenting -- a wrong guess fails loudly instead of landing silently on
the wrong instruction.
"""

import ida_allins
import ida_ua
import idc

DRY_RUN = False

# (ea, expected_cmp_immediate, comment)
COMMENTS = [
    (0x1253E, 0x28, "_playerTileId == TILE_ROCKET x2 (0x28)"),
    (0x12545, 0x26, "_playerTileId == TILE_AIRPLANE x2 (0x26)"),
    (0x1254C, 0x24, "_playerTileId == TILE_SHIP x2 (0x24)"),
    (0x12553, 0x22, "_playerTileId == TILE_HORSE x2 (0x22)"),
]


def verify_cmp_al_imm(ea, expected):
    insn = ida_ua.insn_t()
    if ida_ua.decode_insn(insn, ea) == 0:
        return False, "couldn't decode instruction"
    if insn.itype != ida_allins.NN_cmp:
        return False, f"not a cmp instruction (itype={insn.itype})"
    op1_text = idc.print_operand(ea, 0).lower()
    if insn.Op1.type != ida_ua.o_reg or op1_text != "al":
        return False, f"first operand isn't AL (got {op1_text!r})"
    if insn.Op2.type != ida_ua.o_imm or insn.Op2.value != expected:
        return False, f"second operand isn't imm {expected:#x} (got {insn.Op2.value:#x})"
    return True, None


def apply_comment(ea, expected, comment):
    ok, err = verify_cmp_al_imm(ea, expected)
    if not ok:
        print(f"{ea:X}: [!] address verification FAILED -- {err}, skipping")
        return

    cur = idc.get_cmt(ea, 0)
    if cur == comment:
        print(f"{ea:X}: already commented -- skipping")
        return
    print(f"{ea:X}: verified cmp al,{expected:#x} -- {cur!r} -> {comment!r}")
    if DRY_RUN:
        return
    if not idc.set_cmt(ea, comment, 0):
        print("    [!] set_cmt FAILED")


def main():
    for ea, expected, comment in COMMENTS:
        apply_comment(ea, expected, comment)
    if DRY_RUN:
        print("\n[dry] nothing changed. Set DRY_RUN = False to apply.")
    else:
        print("\nDone. Re-export the .asm/.idc and check the comments took.")


if __name__ == "__main__":
    main()
