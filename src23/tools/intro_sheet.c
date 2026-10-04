/*
 * Renders four frames of Chapter 2's opening story (the tall backdrop panned down, with the animated cells over it) as a 2x2 contact sheet.
 *
 * Build and run (from src23/tools):
 *   gcc -Wall -Wextra -std=c99 -I .. -o intro_sheet intro_sheet.c ../intro2.c ../exedata.c ../pictures.c ../pictures_stdio.c ../palette.c
 *   ./intro_sheet <game dir> <out.png> [scroll1 scroll2 scroll3 scroll4]
 * Every frame also shows the cells' own tick state: the cells are advanced `scroll` times at the animation tick before the shot.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intro2.h"
#include "palette.h"
#include "pictures.h"
#include "pictures_stdio.h"
#include "pngwrite.h"
#include "viewrender.h"

enum { BackdropRows = 196 + 198 + 4 };

static void blitRows(uint8_t *screen, const uint8_t *picture, unsigned width, unsigned firstRow, unsigned rows, int x, int y, bool transparent) {
    for (unsigned r = 0; r < rows; r++) {
        int dy = y + (int)r;
        if (dy < 0 || dy >= ViewScreenHeight) {
            continue;
        }
        for (unsigned c = 0; c < width; c++) {
            int dx = x + (int)c;
            uint8_t pixel = picture[(firstRow + r) * width + c];
            if (dx >= 0 && dx < ViewScreenWidth && !(transparent && pixel == 0xFF)) {
                screen[dy * ViewScreenWidth + dx] = pixel;
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <game dir> <out.png> [4 scroll offsets]\n", argv[0]);
        return 2;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", argv[1]);
    PictureFile *pictures = pictureFileOpen(path, GameYendor2);
    if (!pictures) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }
    snprintf(path, sizeof(path), "%s/WORLD.DAT", argv[1]);
    FILE *f = fopen(path, "rb");
    static uint8_t palette[PicturePaletteSize];
    if (!f || fseek(f, (long)paletteBlockOffset(GameYendor2, 3), SEEK_SET) != 0 || fread(palette, 1, sizeof(palette), f) != sizeof(palette)) {
        fprintf(stderr, "cannot read the palette\n");
        return 1;
    }
    fclose(f);

    static uint8_t backdrop[ViewScreenWidth * BackdropRows];
    const PictureCategory *screens = pictureCategory(GameYendor2, 0);
    blitRows(backdrop, pictureFileGet(pictures, 0, 5), screens->width, 0, screens->height, 1, 0, false);
    blitRows(backdrop, pictureFileGet(pictures, 0, 6), screens->width, 0, screens->height, 1, 196, false);

    int scrolls[4] = {0, 60, 120, 198};
    for (int i = 0; i < 4 && 3 + i < argc; i++) {
        scrolls[i] = atoi(argv[3 + i]);
    }
    static uint8_t sheet[640 * 400];
    for (int shot = 0; shot < 4; shot++) {
        static uint8_t screen[ViewScreenWidth * ViewScreenHeight];
        IntroCell cells[IntroCellCount];
        IntroCellDraw draws[IntroCellCount];
        introCellsInit(cells);
        cells[5].flags |= IntroCellOn | IntroCellAnimating;
        for (int step = 0; step < scrolls[shot]; step++) {
            introCellsFrame(cells, 0, step, step % 4 == 0, draws);
        }
        memcpy(screen, backdrop + scrolls[shot] * ViewScreenWidth, sizeof(screen));
        unsigned n = introCellsFrame(cells, 0, scrolls[shot], false, draws);
        for (unsigned i = 0; i < n; i++) {
            const PictureCategory *c = pictureCategory(GameYendor2, draws[i].category);
            const uint8_t *pixels = pictureFileGet(pictures, draws[i].category, draws[i].picture);
            if (pixels) {
                unsigned rows = draws[i].rows;
                if (draws[i].firstRow + rows > c->height) {
                    rows = c->height - draws[i].firstRow; /* the original reads on into the next picture; stop at this one's end */
                }
                blitRows(screen, pixels, c->width, draws[i].firstRow, rows, draws[i].x, draws[i].y, true);
            }
        }
        int ox = (shot % 2) * 320, oy = (shot / 2) * 200;
        for (int y = 0; y < ViewScreenHeight; y++) {
            memcpy(sheet + (oy + y) * 640 + ox, screen + y * ViewScreenWidth, ViewScreenWidth);
        }
    }
    return writePng(argv[2], sheet, palette, 640, 400, 1) ? 0 : 1;
}
