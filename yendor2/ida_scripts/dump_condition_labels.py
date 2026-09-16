import ida_bytes
addrs = {
    "default(0x7AC0)": 0x7AC0,
    "bit0x2000(0x7ACA)": 0x7ACA,
    "bit0x4000(0x7AD3)": 0x7AD3,
    "bit0x8000(0x7ADC)": 0x7ADC,
    "bit0x400(0x7AE1)": 0x7AE1,
    "bit0x800(0x7AE8)": 0x7AE8,
    "bit0x1000(0x7AEF)": 0x7AEF,
    "bit0x80(0x7AF9)": 0x7AF9,
    "bit0x100(0x7B00)": 0x7B00,
    "bit0x200(0x7B06)": 0x7B06,
    "header(0x7AB3)": 0x7AB3,
}
for label, base in addrs.items():
    ea = base + 0x2D860
    chars = []
    for j in range(30):
        b = ida_bytes.get_byte(ea + j)
        if b == 0:
            break
        chars.append(chr(b) if 32 <= b < 127 else f"\\x{b:02x}")
    print(f"{label}: {''.join(chars)!r}")
