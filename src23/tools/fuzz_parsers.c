/*
 * Feeds truncated and corrupted copies of the real WORLD.DAT to every parser that takes a memory image. The copy ends exactly at the end of a
 * committed page that is followed by a no-access page, so any read past the end the parser should not make crashes the program (Windows only).
 *
 * Build and run (from src23/tools):
 *   gcc -Wall -Wextra -std=c99 -I .. -o fuzz_parsers fuzz_parsers.c ../dialog.c ../document.c ../item.c ../lockcatalog.c ../monster.c ../spellrecord.c \
 *       ../worldmap.c ../worldobjects.c ../bcd4.c ../effect.c ../random.c ../globalflags.c ../movement.c ../party.c ../savegame.c && ./fuzz_parsers <2|3> <game dir> [rounds]
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
#include "spellrecord.h"
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
    }
    printf("%u rounds survived, %u parser acceptances\n", runs, accepted);
    free(original);
    return 0;
}
