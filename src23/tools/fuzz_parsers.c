/*
 * Feeds truncated and corrupted copies of the real WORLD.DAT to every parser that takes a memory image. The copy ends exactly at the end of a
 * committed page that is followed by a no-access page, so any read past the end the parser should not make crashes the program (Windows only).
 *
 * Build and run (from src23/tools):
 *   gcc -Wall -Wextra -std=c99 -I .. -o fuzz_parsers fuzz_parsers.c ../dialog.c ../document.c ../item.c ../lockcatalog.c ../monster.c ../spellrecord.c \
 *       ../worldmap.c ../worldobjects.c ../bcd4.c ../effect.c ../random.c ../globalflags.c ../movement.c ../party.c ../savegame.c ../newgame.c ../chargen.c ../windowbake.c ../interact.c ../dungeongrid.c ../monsterpool.c && ./fuzz_parsers <2|3> <game dir> [rounds]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "dialog.h"
#include "document.h"
#include "item.h"
#include "lockcatalog.h"
#include "monster.h"
#include "newgame.h"
#include "savegame.h"
#include "spellrecord.h"
#include "windowbake.h"
#include "worldmap.h"
#include "worldobjects.h"

static uint8_t *guarded(const uint8_t *source, size_t size, void **base) {
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    size_t page = info.dwPageSize;
    size_t committed = (size + page - 1) / page * page;
    uint8_t *region = VirtualAlloc(NULL, committed + page, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!region) {
        return NULL;
    }
    DWORD old;
    VirtualProtect(region + committed, page, PAGE_NOACCESS, &old);
    *base = region;
    uint8_t *data = region + committed - size;
    memcpy(data, source, size);
    return data;
}

static void runAll(GameKind game, const uint8_t *data, size_t size, unsigned *accepted) {
    static ItemCatalog items;
    static MonsterCatalog monsters;
    static SpellCatalog spells;
    static WorldMap map;
    static DialogCatalog dialog;
    static DocumentCatalog documents;
    static LockCatalog locks;
    static WorldObjectTable objects;
    *accepted += itemCatalogParseWorldDat(&items, game, data, size);
    *accepted += monsterCatalogParseWorldDat(&monsters, game, data, size);
    *accepted += spellCatalogParseWorldDat(&spells, game, data, size);
    *accepted += worldMapParseWorldDat(&map, game, data, size);
    *accepted += dialogCatalogParseWorldDat(&dialog, game, data, size);
    *accepted += documentCatalogParseWorldDat(&documents, game, data, size);
    *accepted += lockCatalogParseWorldDat(&locks, game, data, size);
    *accepted += worldObjectTableParseWorldDat(&objects, game, data, size);
    static SaveGame fresh;
    saveGameInit(&fresh, game);
    *accepted += saveGameNewGame(&fresh, game, data, size);
    /* the window pass over whatever the parsers made of the damaged data (bake needs a save, any save will do) */
    static DungeonGrid grid;
    for (int step = 0; step < 4; step++) {
        dungeonGridBuild(&grid, game, &map, &fresh, 80 + step * 150, 20 + step * 35);
        *accepted += dungeonGridBakeMarkers(&grid, game, &objects, &locks, &fresh) > 0;
    }
}

static void runSave(GameKind game, const uint8_t *data, size_t size, unsigned *accepted) {
    static SaveGame save;
    saveGameInit(&save, game);
    if (saveGameLoad(&save, data, size)) {
        (*accepted)++;
        char name[SaveNameBufferSize];
        saveGetName(&save, name);
        for (unsigned i = 0; i < 9; i++) {
            (void)saveGamePartyRecord(&save, i)[0];
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <2|3> <game dir> [rounds]\n", argv[0]);
        return 2;
    }
    GameKind game = atoi(argv[1]) == 3 ? GameYendor3 : GameYendor2;
    unsigned rounds = argc > 3 ? (unsigned)atoi(argv[3]) : 40;
    char path[512];
    snprintf(path, sizeof(path), "%s/WORLD.DAT", argv[2]);
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *original = malloc(size);
    if (!original || fread(original, 1, size, f) != size) {
        fprintf(stderr, "cannot read %s\n", path);
        return 1;
    }
    fclose(f);

    /* a real saved game (SAVGAME1), if the game directory has one, gets the same treatment */
    snprintf(path, sizeof(path), "%s/SAVGAME1", argv[2]);
    FILE *sf = fopen(path, "rb");
    uint8_t *saved = NULL;
    size_t savedSize = 0;
    if (sf) {
        fseek(sf, 0, SEEK_END);
        savedSize = (size_t)ftell(sf);
        fseek(sf, 0, SEEK_SET);
        saved = malloc(savedSize);
        if (!saved || fread(saved, 1, savedSize, sf) != savedSize) {
            saved = NULL;
        }
        fclose(sf);
    }
    srand(12345);
    unsigned accepted = 0, runs = 0;
    for (unsigned round = 0; round < rounds; round++) {
        size_t cut = round == 0 ? size : (size_t)(((unsigned long long)rand() * 32768ull + (unsigned)rand()) % (size + 1));
        void *base;
        uint8_t *copy = guarded(original, cut, &base);
        if (!copy) {
            fprintf(stderr, "cannot allocate\n");
            return 1;
        }
        if (round > 20) { /* corrupt a few bytes as well */
            for (unsigned k = 0; k < 200 && cut; k++) {
                copy[(size_t)(((unsigned long long)rand() * 32768ull + (unsigned)rand()) % cut)] = (uint8_t)rand();
            }
        }
        runAll(game, copy, cut, &accepted);
        runs++;
        VirtualFree(base, 0, MEM_RELEASE);
        if (saved) {
            size_t savedCut = round == 0 ? savedSize : (size_t)(((unsigned long long)rand() * 32768ull + (unsigned)rand()) % (savedSize + 1));
            uint8_t *savedCopy = guarded(saved, savedCut, &base);
            for (unsigned k = 0; round > 20 && k < 100 && savedCut; k++) {
                savedCopy[(size_t)(((unsigned long long)rand() * 32768ull + (unsigned)rand()) % savedCut)] = (uint8_t)rand();
            }
            runSave(game, savedCopy, savedCut, &accepted);
            VirtualFree(base, 0, MEM_RELEASE);
        }
    }
    printf("%u rounds survived, %u parser acceptances\n", runs, accepted);
    free(original);
    return 0;
}
