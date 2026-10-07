/*
 * Renders Chapter 2's nine opening story cards (intro2.h) as stills: the card text read from SW.EXE, drawn the way DrawShadowedText does (a shadow pass in
 * colour 0x93, then the text one pixel up-left in 0x98, lines 6 rows apart) over the scene picture that is on screen at that point of the story.
 *
 * Build and run (from src23/tools):
 *   gcc -Wall -Wextra -std=c99 -I .. -o intro_cards intro_cards.c ../intro2.c ../exedata.c ../font.c ../pictures.c ../pictures_stdio.c ../palette.c
 *   ./intro_cards <game dir> <out.png>      (the 3 x 3 sheet, 960 x 600)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exedata.h"
#include "font.h"
#include "intro2.h"
#include "palette.h"
#include "pictures.h"
#include "pictures_stdio.h"
#include "pngwrite.h"

static uint8_t *readFile(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    *size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(*size);
    if (data && fread(data, 1, *size, f) != *size) {
        free(data);
        data = NULL;
    }
    fclose(f);
    return data;
}

/* The picture that is showing under each card (category index, picture id, x, y; category -1 = none): the dream's eye, the orb between hands, ... */
static const int kScene[IntroCardCount][4] = {{-1, 0, 0, 0}, {-1, 0, 0, 0}, {1, 0x27, 55, 47}, {1, 0x28, 55, 47}, {1, 0x28, 55, 47},
                                              {-1, 0, 0, 0},  {1, 0x27, 55, 47}, {-1, 0, 0, 0}, {-1, 0, 0, 0}};

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <game dir> <out.png>\n", argv[0]);
        return 2;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/SW.EXE", argv[1]);
    size_t exeSize;
    uint8_t *exeData = readFile(path, &exeSize);
    ExeData exe;
    if (!exeData || !exeDataOpen(&exe, GameYendor2, exeData, exeSize)) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", argv[1]);
    PictureFile *pictures = pictureFileOpen(path, GameYendor2);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", argv[1]);
    size_t worldSize;
    uint8_t *world = readFile(path, &worldSize);
    if (!pictures || !world) {
        fprintf(stderr, "cannot open the game files\n");
        return 1;
    }
    const uint8_t *palette = world + paletteBlockOffset(GameYendor2, 3);
    static uint8_t sheet[960 * 600];
    const IntroCard *cards = introCards();
    for (unsigned c = 0; c < IntroCardCount; c++) {
        static uint8_t screen[320 * 200];
        memset(screen, 0, sizeof(screen));
        if (kScene[c][0] >= 0) {
            const PictureCategory *cat = pictureCategory(GameYendor2, (unsigned)kScene[c][0]);
            const uint8_t *pixels = pictureFileGet(pictures, (unsigned)kScene[c][0], (unsigned)kScene[c][1]);
            for (unsigned row = 0; pixels && row < cat->height; row++) {
                for (unsigned col = 0; col < cat->width; col++) {
                    int x = kScene[c][2] + (int)col, y = kScene[c][3] + (int)row;
                    uint8_t pixel = pixels[row * cat->width + col];
                    if (x >= 0 && x < 320 && y >= 0 && y < 200 && pixel != 0xFF) {
                        screen[y * 320 + x] = pixel;
                    }
                }
            }
        }
        for (unsigned line = 0; line < cards[c].lines; line++) {
            char text[IntroCardLineMax];
            if (!introCardLine(&exe, &cards[c], line, text)) {
                break;
            }
            int y = cards[c].y + (int)line * 6;
            fontDrawString(GameYendor2, 0, screen, 320, cards[c].x, y, text, 0x93, 0, FontTransparent);
            fontDrawString(GameYendor2, 0, screen, 320, cards[c].x - 1, y - 1, text, 0x98, 0, FontTransparent);
        }
        int ox = (int)(c % 3) * 320, oy = (int)(c / 3) * 200;
        for (int y = 0; y < 200; y++) {
            memcpy(sheet + (oy + y) * 960 + ox, screen + y * 320, 320);
        }
    }
    return writePng(argv[2], sheet, palette, 960, 600, 1) ? 0 : 1;
}
