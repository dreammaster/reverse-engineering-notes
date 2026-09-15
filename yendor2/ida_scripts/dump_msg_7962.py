import ida_bytes
ea = 0x7962 + 0x2D860
chars = []
for j in range(60):
    b = ida_bytes.get_byte(ea + j)
    if b == 0:
        break
    chars.append(chr(b) if 32 <= b < 127 else f"\\x{b:02x}")
print(f"{ea:#x}: {''.join(chars)!r}")
