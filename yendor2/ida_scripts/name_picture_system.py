"""
Names the PICTURES.VGA loading/blitting pipeline, fully traced and
verified this session by extracting actual pixel data:

- The picture directory table at DS:0x782E (linear 0x3508E, seg129):
  16-byte entries -- word width @+8, word height @+0xA, 32-bit file
  offset (low word @+0xC, high word @+0xE). Verified by extracting and
  rendering three entries as raw 8bpp images straight from
  game/PICTURES.VGA: entry 0 (318x198 @ offset 0) is the "SmithWare"
  splash-screen logo; entry 8 (16x16 @ 0xBCF3DA) is a mouse-cursor arrow;
  entry 9 (8x8 @ 0xBEF1DA) is a small scroll-arrow icon. -> g_pictureDir
- sub_2A68D: LRU cache of picture IDs mapped into a small pool of LIM
  EMS 4.0 pages (INT 67h/AX=0x5000, "map/unmap multiple handle pages").
  On a cache hit, just re-maps the already-loaded pages; on a miss,
  evicts the oldest slot and reads the picture's bytes from
  PICTURES.VGA (the fixed FileEntry at bx=0x9011) into the newly-mapped
  EMS pages. -> LoadPictureIntoEms
- sub_29878: looks up g_pictureDir[word_2E532], calls LoadPictureIntoEms
  to ensure the picture's data is EMS-mapped, then blits width x height
  pixels from the EMS page frame to the video buffer at (x, y), with the
  blit mode selected by _font_bgTransparent (0-5: different
  transparency/color-key handling per branch). -> DrawPicture

Called from `start` directly and from at least 8 other functions
(sub_178A6, sub_2A788, sub_2D60A, sub_2AA58, sub_2C010, sub_29738,
sub_2C0FE, sub_294A3) -- the core picture-drawing primitive used
throughout the game.

Run via:
    .\run_ida_script.ps1 name_picture_system.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x3508E: "g_pictureDir",
    0x2A68D: "LoadPictureIntoEms",
    0x29878: "DrawPicture",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x3508E,
    "Picture directory for PICTURES.VGA. 16-byte entries: word @+8 = "
    "width, word @+0xA = height, dword (low @+0xC, high @+0xE) = byte "
    "offset into PICTURES.VGA. Raw 8bpp indexed pixels, no per-image "
    "header. Verified by extraction: entry 0 (318x198 @ 0) is the "
    "SmithWare splash screen; entry 8 (16x16 @ 0xBCF3DA) is a mouse "
    "cursor; entry 9 (8x8 @ 0xBEF1DA) is a scroll-arrow icon. Indexed "
    "as g_pictureDir + word_2E532 (picture id * 0x10) elsewhere.",
    False,
)
ida_bytes.set_cmt(
    0x2A68D,
    "LRU cache: maps picture ids into a small pool of LIM EMS 4.0 pages "
    "(INT 67h/AX=0x5000). Cache hit: just re-maps the already-loaded "
    "pages. Cache miss: evicts the oldest slot and reads the picture's "
    "bytes from PICTURES.VGA (FileEntry at bx=0x9011) into the newly-"
    "mapped pages.",
    False,
)
ida_bytes.set_cmt(
    0x29878,
    "Core picture-drawing primitive: looks up g_pictureDir[word_2E532], "
    "calls LoadPictureIntoEms to ensure it's EMS-resident, then blits "
    "width x height pixels from the EMS page frame to the video buffer "
    "at (x, y). Blit mode selected by _font_bgTransparent (0-5 -- "
    "different transparency/color-key branches).",
    False,
)
