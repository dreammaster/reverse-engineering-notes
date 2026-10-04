/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_cmf test_cmf.c ../cmf.c ../audio.c && ./test_cmf
 *
 * Real-data checks parse every music track of WORLD.DAT in yendor2/game and yendor3/game (skipped if absent; YENDOR2_GAME_DIR /
 * YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "cmf.h"

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
    uint8_t t[64];
    memset(t, 0, sizeof(t));
    memcpy(t, "CTMF", 4);
    t[4] = 1;
    t[5] = 1;
    t[6] = 40; /* instruments at 40 */
    t[8] = 56; /* music at 56: one instrument */
    t[10] = 48;
    t[12] = 96;
    t[36] = 1;
    t[38] = 120;
    static const uint8_t music[] = {0x00, 0x90, 0x3C, 0x40, /* note on ch0 */
                                    0x30, 0x3E, 0x41,       /* running status, delta 0x30 */
                                    0x81, 0x00, 0xC1, 0x05, /* delta 128, program change ch1 */
                                    0x00, 0xFF, 0x2F, 0x00};
    memcpy(t + 56, music, sizeof(music));
    CmfTrack track;
    check("a synthetic CTMF block opens", cmfOpen(&track, t, 56 + sizeof(music)) && track.ticksPerQuarter == 48 && track.tempo == 120 && track.instrumentCount == 1);
    CmfReader reader;
    CmfEvent e;
    cmfReaderStart(&reader, &track);
    check("note on, delta 0", cmfReadEvent(&reader, &e) && e.delta == 0 && e.status == 0x90 && e.data1 == 0x3C && e.data2 == 0x40);
    check("running status reuses it", cmfReadEvent(&reader, &e) && e.delta == 0x30 && e.status == 0x90 && e.data1 == 0x3E && e.data2 == 0x41);
    check("a two-byte delta (128) and a one-data-byte program change", cmfReadEvent(&reader, &e) && e.delta == 128 && e.status == 0xC1 && e.data1 == 5);
    check("the end of track meta", cmfReadEvent(&reader, &e) && e.status == 0xFF && e.metaType == 0x2F && e.payloadLength == 0);
    check("nothing after it", !cmfReadEvent(&reader, &e));
    check("a bad signature or a truncated header is refused", !cmfOpen(&track, (const uint8_t *)"CTMX", 4) && !cmfOpen(&track, t, 20));
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
    bool allOk = true, exactEnd = true, anyNotes = true;
    for (unsigned id = 1; id <= audioMusicTrackCount(game); id++) {
        uint32_t offset, length;
        audioMusicTrack(game, id, &offset, &length);
        uint8_t *block = malloc(length);
        bool read = block && fseek(f, (long)offset, SEEK_SET) == 0 && fread(block, 1, length, f) == length;
        CmfTrack track;
        if (!read || !cmfOpen(&track, block, length)) {
            allOk = false;
            free(block);
            continue;
        }
        CmfReader reader;
        CmfEvent e;
        cmfReaderStart(&reader, &track);
        unsigned notes = 0;
        bool ended = false;
        while (cmfReadEvent(&reader, &e)) {
            notes += (e.status & 0xF0) == 0x90 && e.data2 != 0;
            ended = e.status == 0xFF && e.metaType == 0x2F;
        }
        allOk = allOk && ended;
        exactEnd = exactEnd && reader.position == block + length;
        anyNotes = anyNotes && notes > 0;
        free(block);
    }
    fclose(f);
    check(label, allOk);
    check("...each track ends exactly at the end of its block", exactEnd);
    check("...and plays notes", anyNotes);
}

int main(void) {
    testSynthetic();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "Chapter 2: every music track parses to its end-of-track marker");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "Chapter 3: every music track parses to its end-of-track marker");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
