import ida_bytes
base = 0x7B31 + 0x2D860
ea = base
for i in range(9):
    chars = []
    while True:
        b = ida_bytes.get_byte(ea)
        ea += 1
        if b == 0:
            break
        chars.append(chr(b) if 32 <= b < 127 else f"\\x{b:02x}")
    print(f"[{i}] {''.join(chars)!r}")

hdr = 0x7B24 + 0x2D860
chars = []
for j in range(20):
    b = ida_bytes.get_byte(hdr + j)
    if b == 0:
        break
    chars.append(chr(b) if 32 <= b < 127 else f"\\x{b:02x}")
print("header 0x7B24:", repr("".join(chars)))
