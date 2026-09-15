import ida_bytes
DS_BASE = 0x2D860
for label, off, cx in [("0x8E3D item247", 0x8E3D, 2), ("0x8E4E item248", 0x8E4E, 2)]:
    ea = DS_BASE + off
    print(f"--- {label} linear={ea:#x} ---")
    a = ea
    for i in range(cx):
        b = bytearray()
        p = a
        while True:
            c = ida_bytes.get_byte(p)
            if c == 0:
                break
            b.append(c)
            p += 1
        a = p + 1
        print(f"  [{i}] {bytes(b)!r}")
