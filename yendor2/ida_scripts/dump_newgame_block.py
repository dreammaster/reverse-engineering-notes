"""
InitializeNewGameWorldState (yendor2.asm:49634) reads a 5000-byte template (the size of CURGAME section 0:
the game-state block plus the nine 500-byte party records) from WORLD.DAT. PrepareNewGameResetBlockRead
(:42631) takes its 32-bit file offset from the dword at DS:0xCE6F. Prints it.

    .\run_ida_script.ps1 dump_newgame_block.py -NoExport
"""
import ida_bytes

DS_BASE = 0x2D860
lo = ida_bytes.get_word(DS_BASE + 0xCE6F)
hi = ida_bytes.get_word(DS_BASE + 0xCE6F + 2)
line = f"newgame template offset = {(hi << 16) | lo} (0x{(hi << 16) | lo:X}), size 5000"
out_path = r"C:\dev\yendor\yendor2\ida_scripts\newgame_block.txt"
with open(out_path, "w", encoding="utf-8") as f:
    f.write(line)
print(line)
