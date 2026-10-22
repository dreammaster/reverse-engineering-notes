"""
Name and comment VID16.DRV: the INT 62h dispatcher, the 36 service routines
(names/summaries from vid_api.py), the internal helpers, and the mouse event
handler. Idempotent. Run with an export:

    .\run_ida_script.ps1 vid16 name_vid16.py
"""
import idc, ida_segment, ida_funcs, ida_auto, ida_name

from vid_api import VID_FUNCS

seg0 = ida_segment.get_segm_by_name("seg000")
seg1 = ida_segment.get_segm_by_name("seg001")
org = seg0.start_ea
ds_base = ((seg1.start_ea >> 4) << 4)
table = ds_base + 0x1B


def nm(ea, name, cmt=None, func=False):
    if func:
        idc.create_insn(ea)
        ida_funcs.add_func(ea)
    ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    if cmt:
        if func and ida_funcs.get_func(ea):
            idc.set_func_cmt(ea, cmt, 1)
        else:
            idc.set_cmt(ea, cmt, 1)


# service routines
for ah, (name, summary) in VID_FUNCS.items():
    w = idc.get_wide_word(table + ah * 2)
    nm(org + w, name, f"INT 62h AH={ah:02X}: {summary}", func=(ah != 0))
idc.set_cmt(org + idc.get_wide_word(table), "INT 62h AH=00 vidInit -- reached by 'jmp ax' from int62_isr", 1)

nm(org, "int62_isr", "INT 62h service routine. AH<=2Dh indexes the near-offset table at DS:001Bh "
   "(vid_dispatchTable); DS=driver data seg, ES=context block (word_117B9). Preserves all regs but AX.",
   func=True)
nm(table, "vid_dispatchTable", "46 near offsets indexed by AH (IDA seg000 offsets; +0100h at run time behind the PSP)")
nm(ds_base + 9, "vid_ctxSeg", "segment of the context block allocated by vidInit (returned in AX)")

# internal helpers (addresses are IDA seg000 offsets)
HELPERS = {
    0x0C3D: ("vid_drawCharCell", "text cell draw: char ctx+2Ch at col ctx+2Ah / row ctx+2Bh, font ctx+35h, masks ctx+2Eh/2Fh"),
    0x0D23: ("vid_getImageRaw", "screen rect -> memory (ctx+10h rows, ctx+12h bytes wide, mask ctx+14h) at ctx+16h:18h, no header"),
    0x0DA2: ("vid_getPixelRaw", "read pixel via the 4 read-map planes; AL=colour"),
    0x0DC1: ("vid_getImage", "screen rect (x1,y1,x2,y2) -> memory ctx+16h:18h with 5-byte header (rows,w,mask)"),
    0x0E5D: ("vid_lineImpl", "Bresenham line (x1,y1)-(x2,y2) colour ctx+30h; vertical/horizontal fast paths"),
    0x0FEF: ("vid_cursorBlitKeyed", "draw the staged 16x16 cursor at the drawn position, skipping colour 0"),
    0x10A8: ("vid_cursorSaveBackground", "save the 16x16 under (ctx+966h,968h) into cursor store slot 0"),
    0x1145: ("vid_cursorStageShape", "load cursor shape ctx+956h into the off-screen staging area (0,360)"),
    0x1208: ("vid_cursorRestoreBackground", "put the saved background back at (ctx+962h,964h)"),
    0x12C0: ("vid_calcVramAddr", "AX=y BX=x -> BX=A000h offset (+8000h on page 1), CL=bit shift, AH=1, ES=A000h"),
    0x12EF: ("vid_putImageImpl", "draw planar image: rows ctx+10h, w ctx+12h, mask ctx+14h, data ctx+16h:18h at (x1,y1)"),
    0x141F: ("vid_setPageAndDelay", "INT 10h AH=5 (AL=CL page), then busy-wait ctx+8"),
    0x142D: ("vid_putPixelRaw", "write one pixel (GC set/reset) colour ds:6"),
    0x144D: ("vid_mouseEventHandler", "INT 33h user event handler (mask 0Bh: move, left press, right press)"),
    0x14FD: ("vid_mouseUpdateCursorShape", "choose cursor shape ctx+956h from mode ctx+958h and position"),
}
for off, (name, cmt) in HELPERS.items():
    ea = org + off
    nm(ea, name, cmt, func=True)

ida_auto.auto_wait()
print("named", len(VID_FUNCS), "services and", len(HELPERS), "helpers")
