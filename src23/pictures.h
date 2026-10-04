#ifndef YENDOR23_PICTURES_H
#define YENDOR23_PICTURES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * PICTURES.VGA: the picture directory g_pictureDir (yendor2.asm:84214, DS:0x782E; Chapter 3 DS:0x7B5C) is not a list of
 * ten pictures but of ten picture CATEGORIES (g_pictureCategory = category * 0x10). Every picture in a category has the
 * same size, the pictures lie back to back as raw 8 bpp pixels (colour 0xFF = transparent), and
 *
 *     file offset of picture `id` = category base + id * width * height          (g_pictureId = id)
 *
 * (LoadPictureIntoEms, :48116; the per-category EMS cache slots and LRU cursor InitGraphics/Struc1_Allocate build in the
 * first 8 bytes of each entry are an implementation detail and not represented here.) The counts below are what fills the
 * file exactly; every category divides evenly in both games, which is how the layout was confirmed
 * (ida_scripts/dump_picture_dir.py, tools/pic_sheet.py renders any run of pictures).
 *
 *   0  318 x 198  full-screen scenes (splash, title, ...)
 *   1  210 x 105  panels and first-person-view props: dialog/stat/inventory backgrounds, book, scroll, doors, barrels, beds,
 *                 fireplaces, mountains
 *   2  140 x 155  monster sprites (10 frames each: base..base+5 idle, +6..+8 attack, +9 hit flash) and, in Chapter 3, object sprites
 *   3  190 x 110  the large monsters (MonsterFlagAltSprite), 10 frames each
 *   4  224 x  74  the lower half of the first-person view: sky, floor textures
 *   5  224 x  62  the upper half: sky with horizon, ceilings
 *   6   56 x 136  paper-doll bodies of the inventory screen (and the chapter title card, dried-blood stains, slot grid)
 *   7   32 x  32  effect/shop icons, character portraits, clothing layers
 *   8   16 x  16  mouse cursor and UI buttons, equipment and item icons
 *   9    8 x   8  minimap tile glyphs (the legends' minimap offset words select among them: worldmap.h)
 * Chapter 3's pictures are the same sizes with more of each (23/156/270/238/28/14/70/180/340/576).
 *
 * The 256-colour palette is 768 bytes of 6-bit VGA DAC values in WORLD.DAT (LoadMasterPalette, :42668).
 *
 * DrawPicture's blit modes (g_fontBgTransparent, 0-5): 0 plain copy, 1 skip colour 0xFF (optionally remapping hues),
 * 2 skip 0xFF and shift the palette index by the shade delta (clamped), 3 the same without a transparent colour, 4 / 5 copy
 * a sub-rectangle (x offset, y offset, width, height from the blit globals) skipping 0xFF / opaque.
 */
enum { PictureCategoryCount = 10, PicturePaletteSize = 768 };

/*
 * Category 0 screens drawn at (1, 1). The same in both games: 1 the main-screen frame, 2 the title menu, 3 the character sheet,
 * 4 the party roster. Chapter 2 (15 pictures): 0 SmithWare logo, 5 chapter title card, 6 castle gate, 7 automap backdrop, 8 scroll in
 * hand, 9 castle, 10 throne room, 11 "Tyrants of Thaine", 12 Dark Union / clue book, 13-14 stone panels (clue book / dialogs).
 * Chapter 3 (23): 0 SW Games logo, 5 "Restoration" title card, 6-7 stone panels, 8 "Yendorian Tales" banner, 9-10 the story so far (Book I,
 * Chapter 2), 11 the villain, 12 "Tyrants of Thaine" art, 13-14 the Chapter 3 story pages, 15 order form, 16 publisher logo, 17-22 the
 * introduction slideshow (combat, monsters, lava, castle, statistics) with their captions.
 */
enum { PictureScreenMainFrame = 1, PictureScreenTitleMenu = 2, PictureScreenCharacterSheet = 3, PictureScreenRoster = 4 };

typedef struct {
    uint16_t width, height;
    uint32_t base; /* file offset of picture 0 */
    unsigned count;
} PictureCategory;

/* Byte offset of the master palette inside WORLD.DAT. */
uint32_t pictureMasterPaletteOffset(GameKind game);

const PictureCategory *pictureCategory(GameKind game, unsigned category);

/* Byte offset and size of picture `id` of `category`; false if the category or id does not exist. */
bool pictureLocate(GameKind game, unsigned category, unsigned id, uint32_t *offset, uint32_t *size);

#endif
