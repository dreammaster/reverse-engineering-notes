"""Name the two whirlpool position-index tables in ultima_exodus.idb
that teleportPartyWithFanfare and updateWhirlpoolPosition both read
via raw hex offsets ([bx+194Dh]/[bx+1955h]) -- confirmed this session
by cross-referencing the external doc's guessed "moongate coordinate"
file offsets (0x184D/0x1855) against the disassembly. Both tables are
8 bytes (indices 0-7, per the confirmed 'and al,7' masking in
updateWhirlpoolPosition), holding the whirlpool's possible on-map X/Y
positions respectively."""
import idc
import ida_bytes

TABLES = [
    (0x1194D, "WHIRLPOOL_X_TABLE"),
    (0x11955, "WHIRLPOOL_Y_TABLE"),
]

for ea, name in TABLES:
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, 8)
    ok = ida_bytes.create_data(ea, ida_bytes.byte_flag(), 8, idc.BADADDR)
    print(f"create_data({ea:#x}, 8 bytes) -> {ok}")
    renamed = idc.set_name(ea, name, idc.SN_NOWARN)
    print(f"set_name({ea:#x}, {name!r}) -> {renamed}")
    data = ida_bytes.get_bytes(ea, 8)
    print(f"  bytes: {' '.join(f'{b:02x}' for b in data)}")
