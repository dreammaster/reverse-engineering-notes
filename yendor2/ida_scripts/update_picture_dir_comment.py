"""
Updates g_pictureDir's comment with the full catalog (all 10 entries --
confirmed this is the table's complete extent by scanning for plausible
width/height/offset values until 20 implausible entries in a row,
enumerate_pictures.py) now that all of them have been extracted and
visually identified.

Run via:
    .\run_ida_script.ps1 update_picture_dir_comment.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x3508E,
    "Picture directory for PICTURES.VGA -- exactly 10 entries (confirmed "
    "by scanning for plausible width/height/offset until the pattern "
    "breaks down). 16-byte entries: word @+8 = width, word @+0xA = "
    "height, dword (low @+0xC, high @+0xE) = byte offset into "
    "PICTURES.VGA. Raw 8bpp indexed pixels, no per-image header. Full "
    "catalog, extracted and visually identified via extract_pic.py:\n"
    "  0: 318x198 @ 0x0        -- \"SmithWare\" splash-screen logo\n"
    "  1: 210x105 @ 0xE694C    -- GameDialog button panel background\n"
    "  2: 140x155 @ 0x3064B6   -- two-figure combat/fighting scene\n"
    "  3: 190x110 @ 0x779552   -- wolf/monster silhouette\n"
    "  4: 224x74  @ 0xAB3F1A   -- light gradient panel (sky/background?)\n"
    "  5: 224x62  @ 0xAFCC9A   -- sky/cloud gradient\n"
    "  6: 56x136  @ 0xB2579A   -- male character silhouette (char. creation?)\n"
    "  7: 32x32   @ 0xB8BBDA   -- icon (indistinct in grayscale, real "
    "palette not recovered)\n"
    "  8: 16x16   @ 0xBCF3DA   -- mouse cursor\n"
    "  9: 8x8     @ 0xBEF1DA   -- scroll-arrow icon\n"
    "Indexed as g_pictureDir + word_2E532 (picture id * 0x10) elsewhere.",
    False,
)
print("done")
