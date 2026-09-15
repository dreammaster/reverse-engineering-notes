import ida_bytes
for base in (0x7960, 0xAFE7, 0xAFDA):
    ea = base + 0x2D860
    chars = []
    for j in range(60):
        b = ida_bytes.get_byte(ea + j)
        if b == 0:
            break
        chars.append(chr(b) if 32 <= b < 127 else f"\\x{b:02x}")
    print(f"{base:#x} ({ea:#x}): {''.join(chars)!r}")
