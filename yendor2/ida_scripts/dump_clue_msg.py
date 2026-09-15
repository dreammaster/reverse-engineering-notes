import ida_bytes
ea = 0x2D860 + 0x8E63
b = bytearray()
p = ea
while True:
    c = ida_bytes.get_byte(p)
    if c == 0:
        break
    b.append(c)
    p += 1
print(f"0x8E63 linear={ea:#x}: {bytes(b)!r}")
