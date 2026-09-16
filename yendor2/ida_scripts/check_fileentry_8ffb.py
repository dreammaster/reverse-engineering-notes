import idc

DS_BASE = 0x2D860
ea = DS_BASE + 0x8FFB + 0xE
print("filename ea:", hex(ea))
print("contents:", idc.get_strlit_contents(ea))
