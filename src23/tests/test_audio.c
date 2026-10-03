/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_audio test_audio.c ../audio.c && ./test_audio
 *
 * Real-data checks read WORLD.DAT from yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"

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

static void testTables(void) {
    uint32_t offset, length;
    check("ids are 1-based; 0 and past the end have no block", !audioMusicTrack(GameYendor2, 0, &offset, &length) && audioMusicTrack(GameYendor2, 21, &offset, &length) &&
                                                                    !audioMusicTrack(GameYendor2, 22, &offset, &length) && audioMusicTrack(GameYendor3, 24, &offset, &length) &&
                                                                    !audioEffect(GameYendor2, 81, &offset, &length) && audioEffect(GameYendor3, 141, &offset, &length));
    check("Chapter 2's first track", audioMusicTrack(GameYendor2, 1, &offset, &length) && offset == 541383 && length == 5357);
    check("the counts", audioMusicTrackCount(GameYendor2) == 21 && audioMusicTrackCount(GameYendor3) == 24 && audioEffectCount(GameYendor2) == 80 &&
                            audioEffectCount(GameYendor3) == 141);
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : defaultDir);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    bool musicOk = true, effectsOk = true, contiguous = true;
    uint32_t previousEnd = 0;
    for (unsigned id = 1; id <= audioMusicTrackCount(game); id++) {
        uint32_t offset, length;
        char head[4];
        audioMusicTrack(game, id, &offset, &length);
        musicOk = musicOk && fseek(f, (long)offset, SEEK_SET) == 0 && fread(head, 1, 4, f) == 4 && memcmp(head, "CTMF", 4) == 0;
        contiguous = contiguous && (previousEnd == 0 || previousEnd == offset);
        previousEnd = offset + length;
    }
    for (unsigned id = 1; id <= audioEffectCount(game); id++) {
        uint32_t offset, length;
        char head[19];
        audioEffect(game, id, &offset, &length);
        effectsOk = effectsOk && fseek(f, (long)offset, SEEK_SET) == 0 && fread(head, 1, 19, f) == 19 && memcmp(head, "Creative Voice File", 19) == 0;
        contiguous = contiguous && previousEnd == offset;
        previousEnd = offset + length;
    }
    fclose(f);
    check(label, musicOk);
    check("...every effect starts with a Creative Voice File header", effectsOk);
    check("...and the blocks are contiguous, music then effects", contiguous);
}

int main(void) {
    testTables();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: every music track starts with a CTMF header");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: every music track starts with a CTMF header");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
