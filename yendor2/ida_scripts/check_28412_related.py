import idc
for ea in (0x2849C, 0x28412):
    print(hex(ea), idc.get_func_name(ea), idc.get_name(ea))
