/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_savegame test_savegame.c ../savegame.c ../savegame_stdio.c && ./test_savegame
 *
 * The real-file checks read yendor2/game/CURGAME and SAVGAME1 (gitignored,
 * so they are skipped if absent). Set YENDOR2_GAME_DIR to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "savegame.h"
#include "savegame_stdio.h"

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

static void checkU32(const char *label, uint32_t actual, uint32_t expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %u, want %u\n", label, actual, expected);
    }
}

static SaveGame g_save;
static SaveGame g_other;
static uint8_t g_image[SaveFileSizeMax + 8];

static void testLayout(SaveGameKind kind, const char *label, uint32_t expectedTotal) {
    const SaveLayout *layout = saveLayoutFor(kind);
    char text[96];

    snprintf(text, sizeof(text), "%s total size", label);
    checkU32(text, layout->totalSize, expectedTotal);

    bool contiguous = true;
    bool recordsMatch = true;
    uint32_t next = 0;
    for (int s = 0; s < SaveSectionCount; s++) {
        const SaveSectionInfo *info = &layout->sections[s];
        if (info->offset != next) {
            contiguous = false;
        }
        if ((uint32_t)info->recordSize * info->recordCount != info->size) {
            recordsMatch = false;
        }
        next = info->offset + info->size;
    }
    snprintf(text, sizeof(text), "%s sections are contiguous", label);
    check(text, contiguous);
    snprintf(text, sizeof(text), "%s recordSize * recordCount == size", label);
    check(text, recordsMatch);
    snprintf(text, sizeof(text), "%s sections end exactly at total size", label);
    checkU32(text, next, expectedTotal);
}

static void testLayouts(void) {
    testLayout(SaveGameYendor2, "yendor2", SaveFileSizeYendor2);
    testLayout(SaveGameYendor3, "yendor3", SaveFileSizeYendor3);

    const SaveLayout *layout2 = saveLayoutFor(SaveGameYendor2);
    checkU32("header block + 9 party records = section 1",
             SaveHeaderRecordSize + SavePartyRecordCount * SavePartyRecordSize,
             layout2->sections[SaveSectionHeaderAndParty].size);
    checkU32("item instances: 16 chunks of 0xAC2 bytes", 16 * 0xAC2,
             layout2->sections[SaveSectionItemInstances].size);
    checkU32("monsters: 80 x 156 = 0x30C0",
             SaveMonsterCount * SaveMonsterRecordSize,
             layout2->sections[SaveSectionMonsters].size);
}

static void fillPattern(uint8_t *data, size_t size) {
    uint32_t state = 0xC0FFEEu;
    for (size_t i = 0; i < size; i++) {
        state = state * 1664525u + 1013904223u;
        data[i] = (uint8_t)(state >> 24);
    }
}

static void testLoadStore(void) {
    static const size_t sizes[] = {0, 1, SaveFileSizeYendor2 - 1, SaveFileSizeYendor2 + 1,
                                   SaveFileSizeYendor3 - 1, SaveFileSizeYendor3 + 1};
    bool rejected = true;
    fillPattern(g_image, sizeof(g_image));
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        if (saveGameLoad(&g_save, g_image, sizes[i])) {
            rejected = false;
        }
    }
    check("wrong-sized files are rejected", rejected);

    SaveGameKind kinds[] = {SaveGameYendor2, SaveGameYendor3};
    size_t fileSizes[] = {SaveFileSizeYendor2, SaveFileSizeYendor3};
    for (int k = 0; k < 2; k++) {
        char label[64];
        uint8_t out[SaveFileSizeMax];
        fillPattern(g_image, fileSizes[k]);
        snprintf(label, sizeof(label), "load infers kind (%zu bytes)", fileSizes[k]);
        check(label, saveGameLoad(&g_save, g_image, fileSizes[k]) && g_save.kind == kinds[k]);
        snprintf(label, sizeof(label), "store round-trips byte-exactly (%zu bytes)", fileSizes[k]);
        check(label, saveGameStore(&g_save, out, sizeof(out)) == fileSizes[k] &&
                         memcmp(out, g_image, fileSizes[k]) == 0);
        check("store refuses a too-small buffer", saveGameStore(&g_save, out, fileSizes[k] - 1) == 0);
    }
}

static void testAccessors(void) {
    saveGameInit(&g_save, SaveGameYendor2);
    const SaveLayout *layout = saveLayoutFor(SaveGameYendor2);

    check("section pointer at its offset",
          saveGameSection(&g_save, SaveSectionMonsters) == g_save.bytes + 0xFE05);
    check("invalid section is NULL", saveGameSection(&g_save, SaveSectionCount) == NULL);
    check("last item instance is in range",
          saveGameRecord(&g_save, SaveSectionItemInstances, 1295) ==
              g_save.bytes + 0x4BC8 + 1295 * 34);
    check("one past the last item instance is NULL",
          saveGameRecord(&g_save, SaveSectionItemInstances, 1296) == NULL);
    check("monster 79 is the last section byte range",
          saveGameRecord(&g_save, SaveSectionMonsters, 79) + SaveMonsterRecordSize ==
              g_save.bytes + layout->totalSize);
    check("monster 80 is NULL", saveGameRecord(&g_save, SaveSectionMonsters, 80) == NULL);

    check("party record 0 follows the 500-byte header",
          saveGamePartyRecord(&g_save, 0) == g_save.bytes + 500);
    check("party record 8 ends the header+party section",
          saveGamePartyRecord(&g_save, 8) + 500 == g_save.bytes + 5000);
    check("party record 9 is NULL", saveGamePartyRecord(&g_save, 9) == NULL);
    check("id 1 maps to record 0 (SelectPartyRecordById)",
          saveGamePartyRecordById(&g_save, 1) == saveGamePartyRecord(&g_save, 0));
    check("id 0 is NULL", saveGamePartyRecordById(&g_save, 0) == NULL);
    check("id 10 is NULL", saveGamePartyRecordById(&g_save, 10) == NULL);

    saveGameInit(&g_other, SaveGameYendor3);
    check("yendor3 explored map uses 168 rows",
          saveGameRecord(&g_other, SaveSectionExploredMap, 167) != NULL &&
              saveGameRecord(&g_other, SaveSectionExploredMap, 168) == NULL);
    check("yendor2 explored map uses 144 rows",
          saveGameRecord(&g_save, SaveSectionExploredMap, 143) != NULL &&
              saveGameRecord(&g_save, SaveSectionExploredMap, 144) == NULL);
}

static void testHeaderFields(void) {
    saveGameInit(&g_save, SaveGameYendor2);
    saveHeaderSetU16(&g_save, SaveHeaderWorldX, 0x1234);
    check("u16 is stored little-endian",
          g_save.bytes[SaveHeaderWorldX] == 0x34 && g_save.bytes[SaveHeaderWorldX + 1] == 0x12);
    checkU32("u16 round-trips", saveHeaderGetU16(&g_save, SaveHeaderWorldX), 0x1234);

    uint8_t *gold = saveHeaderBcd4(&g_save, SaveHeaderGold);
    check("gold is at header +0xB4", gold == g_save.bytes + 0xB4);

    g_save.bytes[SaveHeaderPartySlots + 2] = 8;
    checkU32("party slot 1 reads its u16", saveGetPartySlot(&g_save, 1), 8);
    checkU32("party slot 4 is out of range", saveGetPartySlot(&g_save, 4), 0);

    check("facing 0x8000 is NORTH", strcmp(saveFacingName(0x8000), "NORTH") == 0);
    check("facing 0x4000 is SOUTH", strcmp(saveFacingName(0x4000), "SOUTH") == 0);
    check("facing 0x1000 is EAST", strcmp(saveFacingName(0x1000), "EAST") == 0);
    check("facing 0x2000 is WEST", strcmp(saveFacingName(0x2000), "WEST") == 0);
    check("other facing values have no name", saveFacingName(3) == NULL);
}

static void testName(void) {
    char name[SaveNameBufferSize];
    saveGameInit(&g_save, SaveGameYendor2);

    /*
     * Matches the real SAVGAME1 header, which reads "DAN\0HWARE PARTY\0":
     * the game saves over a "SMITHWARE PARTY" header with a plain string
     * copy, so the tail of the old name stays behind.
     */
    check("set default name", saveSetName(&g_save, "SMITHWARE PARTY"));
    check("set shorter name", saveSetName(&g_save, "DAN"));
    saveGetName(&g_save, name);
    check("name reads back as DAN", strcmp(name, "DAN") == 0);
    check("stale tail is preserved", memcmp(g_save.bytes, "DAN\0HWARE PARTY\0", 16) == 0);

    check("24-character name fits", saveSetName(&g_save, "ABCDEFGHIJKLMNOPQRSTUVWX"));
    saveGetName(&g_save, name);
    check("24-character name reads back", strcmp(name, "ABCDEFGHIJKLMNOPQRSTUVWX") == 0);

    uint8_t before[SaveNameBufferSize + 1];
    memcpy(before, g_save.bytes, sizeof(before));
    check("25-character name is rejected", !saveSetName(&g_save, "ABCDEFGHIJKLMNOPQRSTUVWXY"));
    check("rejected name changes nothing", memcmp(before, g_save.bytes, sizeof(before)) == 0);

    memset(g_save.bytes, 'Z', SaveNameBufferSize + 4);
    saveGetName(&g_save, name);
    check("unterminated name is clipped at 24", strlen(name) == SaveNameMaxLength);
}

static void testSlotFileNames(void) {
    char name[SaveSlotFileNameSize];
    check("slot 1 is SAVGAME1", saveSlotFileName(name, 1) && strcmp(name, "SAVGAME1") == 0);
    check("slot 6 is SAVGAME6", saveSlotFileName(name, 6) && strcmp(name, "SAVGAME6") == 0);
    check("slot 0 is invalid", !saveSlotFileName(name, 0));
    check("slot 7 is invalid", !saveSlotFileName(name, 7));
}

static bool loadReal(const char *dir, const char *file, SaveGame *out) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir, file);
    return saveGameReadFile(out, path);
}

static bool partyRecordNameIs(SaveGame *save, unsigned id, const char *expected) {
    const uint8_t *record = saveGamePartyRecordById(save, id);
    return record && strncmp((const char *)record, expected, 13) == 0;
}

static void testRealFiles(void) {
    const char *dir = getenv("YENDOR2_GAME_DIR");
    if (!dir) {
        dir = "../../yendor2/game";
    }
    if (!loadReal(dir, "CURGAME", &g_save)) {
        printf("SKIP real-file checks (no CURGAME in %s)\n", dir);
        g_skipCount++;
        return;
    }

    char name[SaveNameBufferSize];
    check("real CURGAME is detected as yendor2", g_save.kind == SaveGameYendor2);
    saveGetName(&g_save, name);
    check("real CURGAME name is SMITHWARE PARTY", strcmp(name, "SMITHWARE PARTY") == 0);
    checkU32("real CURGAME world X", saveHeaderGetU16(&g_save, SaveHeaderWorldX), 166);
    checkU32("real CURGAME world Y", saveHeaderGetU16(&g_save, SaveHeaderWorldY), 36);
    check("real CURGAME faces west",
          strcmp(saveFacingName(saveHeaderGetU16(&g_save, SaveHeaderFacing)), "WEST") == 0);
    checkU32("real CURGAME game day", saveHeaderGetU16(&g_save, SaveHeaderGameDay), 4);
    checkU32("real CURGAME game month", saveHeaderGetU16(&g_save, SaveHeaderGameMonth), 11);
    checkU32("real CURGAME game year", saveHeaderGetU16(&g_save, SaveHeaderGameYear), 546);

    if (!loadReal(dir, "SAVGAME1", &g_other)) {
        printf("SKIP SAVGAME1 checks (not found in %s)\n", dir);
        g_skipCount++;
        return;
    }
    saveGetName(&g_other, name);
    check("real SAVGAME1 name is DAN", strcmp(name, "DAN") == 0);
    check("real SAVGAME1 keeps the stale name tail",
          memcmp(g_other.bytes, "DAN\0HWARE PARTY\0", 16) == 0);
    check("real SAVGAME1 party slots are 7,8,9,6",
          saveGetPartySlot(&g_other, 0) == 7 && saveGetPartySlot(&g_other, 1) == 8 &&
              saveGetPartySlot(&g_other, 2) == 9 && saveGetPartySlot(&g_other, 3) == 6);
    check("real SAVGAME1 slot 0 member is DIANA",
          partyRecordNameIs(&g_other, saveGetPartySlot(&g_other, 0), "DIANA"));
    check("real SAVGAME1 slot 1 member is YENDOR",
          partyRecordNameIs(&g_other, saveGetPartySlot(&g_other, 1), "YENDOR"));
    check("real SAVGAME1 slot 2 member is JOSEPHINE",
          partyRecordNameIs(&g_other, saveGetPartySlot(&g_other, 2), "JOSEPHINE"));
    check("real SAVGAME1 slot 3 member is SQUIRE",
          partyRecordNameIs(&g_other, saveGetPartySlot(&g_other, 3), "SQUIRE"));

    /* Round-trip the real files through the writer and compare bytes. */
    char tempPath[512];
    snprintf(tempPath, sizeof(tempPath), "%s/roundtrip.tmp", ".");
    bool wrote = saveGameWriteFile(&g_other, tempPath);
    static SaveGame reread;
    bool read = wrote && saveGameReadFile(&reread, tempPath);
    remove(tempPath);
    check("real SAVGAME1 write/read round-trips byte-exactly",
          read && reread.kind == g_other.kind &&
              memcmp(reread.bytes, g_other.bytes, SaveFileSizeYendor2) == 0);
}

int main(void) {
    testLayouts();
    testLoadStore();
    testAccessors();
    testHeaderFields();
    testName();
    testSlotFileNames();
    testRealFiles();

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
