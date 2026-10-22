"""
Name the SW.EXE functions whose role has been established (docs1/program-graph.md).
Addresses are linear (segment base + offset) as shown in the listing.

    .\run_ida_script.ps1 sw name_sw.py
"""
import idc
import ida_name

NAMES = {
    0x10000: ("start", "Shell main: open data files, run PROG0/1/2/3/4/10/16/17/18 per exit codes (see docs1/program-graph.md)"),
    0x103AB: ("showEndScreen", "shareware thank-you / register screen built from tiles and text"),
    0x10571: ("runPrologue", "EXEC PROG2 (prologue), reset prologue flags, save party state"),
    0x1059A: ("drawTileRun", "draw CX consecutive tiles left to right (tile index word_12878 increments)"),
    0x105AF: ("openDataFiles", "open the 15 data files, load palettes/headers; exits with the failing file index in byte_12877"),
    0x1082E: ("flushPartyAndRefresh", "write party records back and show page 0"),
    0x10873: ("startDriversEga", "EXEC VID16.DRV, INT 62h fn00, EXEC VIDP16.DRV"),
    0x108AC: ("startDriversSvga", "EXEC VID256.DRV, INT 62h fn00, EXEC VIDP256.DRV (not present in this install)"),
    0x10984: ("showClosingMessageA", "closing text screen after PROG17"),
    0x10A46: ("showClosingMessageB", "closing text screen after PROG16"),
    0x10B08: ("calibrateDelays", "CPU class -> ctx+8/0Ah/0Ch delay counts"),
    0x10B64: ("drawPartyStrip", "draw the party/character portrait strip"),
    0x111A4: ("clearTextScreen", "INT 10h scroll-clear the text screen"),
    0x111BC: ("detectCpuClass", "returns AX=88h (8088), 186h, 286h, 386h"),
    0x111F8: ("delayTicks", "wait AX BIOS clock ticks (INT 1Ah)"),
    0x1120E: ("fileSeekRecord", "lseek to record number [bx+8] * record size [bx+6]"),
    0x1122C: ("fileOpen", "open file descriptor BX (name at +0Eh) with mode AL; byte_1287F = error count"),
    0x1124F: ("fileReadRecord", "read record into [bx+2]"),
    0x11276: ("fileReadToSegment", "read record into segment DX:0"),
    0x112A0: ("fileWriteRecord", "write record from [bx+2]"),
    0x112C8: ("waitInput", "poll until pollInput reports an event"),
    0x112D6: ("drawTileKeyed", "colour-keyed copy of a 32x32 tile via vidCopyRectKeyed"),
    0x1134E: ("pollInput", "keyboard (INT 21h/06h) or mouse button -> ctx+2Ch key, byte ds:0EDFh = source"),
    0x11432: ("fileReadRecordTo", "read record AX into the descriptor buffer and copy it to ES:DI"),
    0x1144C: ("execProgram", "DOS EXEC of the program named at DS:DX; AX = parameter block offset in the command tail"),
    0x114A4: ("drawStringLines", "draw CX consecutive text lines from BX"),
    0x114B2: ("paletteFade", "AL=0 fade up from black, 1 fade to black, 2 fade to white (VGA DAC)"),
    0x11658: ("fileWriteRecordFrom", "copy [si] into the descriptor buffer as record AX and write it"),
    0x11678: ("drawString", "draw the ASCIZ string at BX with the mouse hidden"),
    0x116F2: ("drawBackdropTile", "draw tile 0D5h of file 0 at (0,0)"),
    0x11716: ("sendPaletteCommand", "wait for ctx+676h to clear, then store AL there (command mailbox read by VIDP16)"),
    0x11724: ("speakerOn", "program PIT channel 2 for frequency 1193180/CX, enable speaker"),
    0x11741: ("speakerOff", "silence the PC speaker"),
    0x1174C: ("tileOp", "tile cache dispatcher: AL = 0..5 operation, tile = (byte_1287A file, word_12878 index)"),
    0x118BD: ("tileLookup", "find tile (type,index) in the ctx+2 directory; on miss pick a free slot (CF-style code in byte_1287F)"),
    0x11957: ("tileLoad", "load the missing tile from FILEnn.16 into its slot"),
}
n = 0
for ea, (name, cmt) in NAMES.items():
    if idc.get_name(ea) and not idc.get_name(ea).startswith(("sub_", "start", "loc_", "execProgram")):
        pass
    ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    idc.set_func_cmt(ea, cmt, 1)
    n += 1
print("named", n, "SW functions")
