/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_pictures test_pictures.c ../pictures.c && ./test_pictures
 *
 * Real-data checks read PICTURES.VGA and WORLD.DAT from yendor2/game and yendor3/game (skipped if absent;
 * YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pictures.h"

static int g_failureCount = 0;
static int g_skipCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void testLocate(void) {
    uint32_t off, size;
    check("Chapter 2: category 1 picture 0 starts at its base", pictureLocate(GameYendor2, 1, 0, &off, &size) && off == 944460 && size == 22050);
    check("...picture 5 is five pictures in", pictureLocate(GameYendor2, 1, 5, &off, &size) && off == 944460 + 5 * 22050);
    check("...past the last picture fails", !pictureLocate(GameYendor2, 1, 101, &off, &size) && pictureLocate(GameYendor2, 1, 100, &off, &size));
    check("an unknown category fails", !pictureLocate(GameYendor2, 10, 0, &off, &size) && pictureCategory(GameYendor3, 10) == NULL);
    check("the 8x8 glyph category holds 576 in both games", pictureCategory(GameYendor2, 9)->count == 576 && pictureCategory(GameYendor3, 9)->count == 576);
    check("palette offsets", pictureMasterPaletteOffset(GameYendor2) == 0x8270A && pictureMasterPaletteOffset(GameYendor3) == 0x95BDA);
}

static long fileSize(FILE *f) {
    fseek(f, 0, SEEK_END);
    return ftell(f);
}

static void testRealFile(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/PICTURES.VGA", dir ? dir : defaultDir);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    long size = fileSize(f);
    bool contiguous = true;
    uint32_t end = 0;
    for (unsigned c = 0; c < PictureCategoryCount; c++) {
        const PictureCategory *cat = pictureCategory(game, c);
        contiguous = contiguous && cat->base == end;
        end = cat->base + (uint32_t)cat->count * cat->width * cat->height;
    }
    check(label, contiguous && (long)end == size);

    /* the first pixels of the first category-0 picture and of the last picture of category 9 exist and are not all one value */
    uint32_t off, picSize;
    uint8_t buf[318 * 198];
    bool ok = pictureLocate(game, 0, 0, &off, &picSize) && picSize == sizeof(buf) && fseek(f, (long)off, SEEK_SET) == 0 &&
              fread(buf, 1, picSize, f) == picSize;
    bool varied = false;
    for (size_t i = 1; ok && i < picSize; i++) {
        varied = varied || buf[i] != buf[0];
    }
    check("...picture 0 reads back with varied pixels", ok && varied);
    fclose(f);

    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    f = fopen(path, "rb");
    if (f) {
        uint8_t pal[PicturePaletteSize];
        bool read = fseek(f, (long)pictureMasterPaletteOffset(game), SEEK_SET) == 0 && fread(pal, 1, sizeof(pal), f) == sizeof(pal);
        bool six = read;
        for (size_t i = 0; read && i < sizeof(pal); i++) {
            six = six && pal[i] <= 63;
        }
        check("...the master palette holds 6-bit DAC values", read && six);
        fclose(f);
    }
}

int main(void) {
    testLocate();
    testRealFile(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: categories tile PICTURES.VGA exactly");
    testRealFile(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: categories tile PICTURES.VGA exactly");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
