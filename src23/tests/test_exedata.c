/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_exedata test_exedata.c ../exedata.c && ./test_exedata
 *
 * The real-data checks read yendor2/game/SW.EXE and yendor3/game/REGISTER.EXE (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "exedata.h"

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

static void testSynthetic(void) {
    static uint8_t image[0x30000];
    memset(image, 0, sizeof(image));
    memcpy(image + 0x21660 + 0x79E5, "QUIT \"CREATE\"", 14);
    memcpy(image + 0x21660 + 0x7A28, "TAKE UP TO FOUR", 16);
    image[0x21660 + 0x100] = 0x34;
    image[0x21660 + 0x101] = 0x12;
    ExeData exe;
    check("a synthetic Chapter 2 image opens", exeDataOpen(&exe, GameYendor2, image, sizeof(image)));
    char text[32];
    check("a string at DS:0x7A28", exeDataString(&exe, 0x7A28, text, sizeof(text)) && strcmp(text, "TAKE UP TO FOUR") == 0);
    check("truncated to the capacity", exeDataString(&exe, 0x7A28, text, 5) && strcmp(text, "TAKE") == 0);
    check("a word at DS:0x100", exeDataU16(&exe, 0x100) == 0x1234 && exeDataU8(&exe, 0x101) == 0x12);
    check("outside the file: nothing", !exeDataString(&exe, 0xFFFFF, text, sizeof(text)) && exeDataU8(&exe, 0xFFFFF) == 0);
    check("a file with the label missing or too short is refused", !exeDataOpen(&exe, GameYendor3, image, sizeof(image)) && !exeDataOpen(&exe, GameYendor2, image, 100));
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *name, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir ? dir : defaultDir, name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)size);
    bool read = data && fread(data, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    ExeData exe;
    char text[40];
    check(label, read && exeDataOpen(&exe, game, data, (size_t)size));
    if (read) {
        unsigned item = game == GameYendor2 ? 0x7A28 : 0x7D5A;
        check("...and the item-pick header is where the disassembly says", exeDataString(&exe, item, text, sizeof(text)) && strcmp(text, "TAKE UP TO FOUR") == 0);
    }
    free(data);
}

int main(void) {
    testSynthetic();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "SW.EXE", "Chapter 2: SW.EXE holds the data segment at 0x21660");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "REGISTER.EXE", "Chapter 3: REGISTER.EXE holds the data segment at 0x21DB0");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
