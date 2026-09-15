import ida_bytes
DS_BASE = 0x2D860
for label, off in [("0x7916", 0x7916), ("0x791F", 0x791F)]:
    ea = DS_BASE + off
    b = bytearray()
    p = ea
    while True:
        c = ida_bytes.get_byte(p)
        if c == 0:
            break
        b.append(c)
        p += 1
    print(f"{label} linear={ea:#x}: {bytes(b)!r}")
