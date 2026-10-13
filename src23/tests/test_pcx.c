/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_pcx test_pcx.c ../pcx.c ../exedata.c && ./test_pcx
 *
 * A synthetic PCX, and (skipped when absent; YENDOR3_GAME_DIR overrides) Chapter 3's real boot logo from yendor3/game/WORLD.DAT and REGISTER.EXE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcx.h"

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

static void testSynthetic(void) {
    uint8_t file[128 + 8 + 769];
    memset(file, 0, sizeof(file));
    file[0] = 0x0A;
    file[1] = 5;
    file[2] = 1;
    file[3] = 8;
    file[8] = 3; /* xmax 3, ymax 1: 4 x 2 */
    file[10] = 1;
    file[0x42] = 4; /* bytes per line */
    const uint8_t data[] = {0x05, 0xC3, 0x09, 0x07, 0xC1, 0x01, 0x02}; /* 5, 9 9 9, 7, 1, 2: eight pixels */
    memcpy(file + 128, data, sizeof(data));
    file[128 + sizeof(data)] = 0;
    size_t size = 128 + 8 + 769;
    file[size - 769] = 0x0C;
    for (unsigned i = 0; i < 768; i++) {
        file[size - 768 + i] = (uint8_t)(i % 256);
    }
    uint8_t pixels[16], palette[PcxPaletteSize];
    unsigned width = 0, height = 0;
    check("a small PCX decodes", pcxDecode(file, size, pixels, sizeof(pixels), &width, &height, palette) && width == 4 && height == 2);
    check("a run is a count and a value, a byte below 0xC0 a pixel", pixels[0] == 5 && pixels[1] == 9 && pixels[3] == 9 && pixels[4] == 7 && pixels[5] == 1 && pixels[6] == 2 && pixels[7] == 0);
    check("the palette is the last 768 bytes shifted to six bits", palette[0] == 0 && palette[4] == 1 && palette[255] == 63);
    check("a PCX that does not fit is refused", !pcxDecode(file, size, pixels, 4, &width, &height, palette));
    file[0] = 0;
    check("a wrong signature is refused", !pcxDecode(file, size, pixels, sizeof(pixels), &width, &height, palette));
    file[0] = 0x0A;
    file[size - 769] = 0;
    check("a missing palette marker is refused", !pcxDecode(file, size, pixels, sizeof(pixels), &width, &height, palette));
}

static void testReal(void) {
    const char *dir = getenv("YENDOR3_GAME_DIR");
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : "../../yendor3/game");
    size_t worldSize, exeSize;
    uint8_t *world = readFile(path, &worldSize);
    snprintf(path, sizeof(path), "%s/REGISTER.EXE", dir ? dir : "../../yendor3/game");
    uint8_t *exeBytes = readFile(path, &exeSize);
    ExeData exe;
    if (!world || !exeBytes || !exeDataOpen(&exe, GameYendor3, exeBytes, exeSize)) {
        printf("SKIP Chapter 3's boot logo (game files not found)\n");
        g_skipCount++;
        free(world);
        free(exeBytes);
        return;
    }
    size_t at, size;
    check("the logo's place is read from the executable", pcxLogoLocation(&exe, GameYendor3, &at, &size) && at == 0x41EAB7 && size == 0x721E && at + size <= worldSize);
    check("Chapter 2 has none", !pcxLogoLocation(&exe, GameYendor2, &at, &size));
    static uint8_t pixels[640 * 480];
    uint8_t palette[PcxPaletteSize];
    unsigned width, height;
    pcxLogoLocation(&exe, GameYendor3, &at, &size);
    check("the logo is a 640 x 480 PCX that decodes", pcxDecode(world + at, size, pixels, sizeof(pixels), &width, &height, palette) && width == 640 && height == 480);
    unsigned used[256] = {0}, colours = 0;
    for (size_t i = 0; i < sizeof(pixels); i++) {
        used[pixels[i]]++;
    }
    for (unsigned i = 0; i < 256; i++) {
        colours += used[i] > 0;
    }
    bool sixBit = true;
    for (unsigned i = 0; i < PcxPaletteSize; i++) {
        sixBit = sixBit && palette[i] < 64;
    }
    check("it uses several colours, all in a six-bit palette", colours > 8 && sixBit);
    free(world);
    free(exeBytes);
}

int main(void) {
    testSynthetic();
    testReal();
    printf("%s (%d skipped)\n", g_failureCount ? "FAILED" : "ALL PASSED", g_skipCount);
    return g_failureCount ? 1 : 0;
}
