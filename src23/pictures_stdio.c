#include "pictures_stdio.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    unsigned category, id;
    uint8_t *pixels;
    bool used;
} Slot;

struct PictureFile {
    FILE *file;
    GameKind game;
    Slot slots[PictureFileCacheSlots];
    unsigned next;
};

PictureFile *pictureFileOpen(const char *path, GameKind game) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }
    PictureFile *pf = calloc(1, sizeof(*pf));
    if (!pf) {
        fclose(f);
        return NULL;
    }
    pf->file = f;
    pf->game = game;
    return pf;
}

void pictureFileClose(PictureFile *pf) {
    if (!pf) {
        return;
    }
    for (unsigned i = 0; i < PictureFileCacheSlots; i++) {
        free(pf->slots[i].pixels);
    }
    fclose(pf->file);
    free(pf);
}

const uint8_t *pictureFileGet(void *file, unsigned category, unsigned id) {
    PictureFile *pf = file;
    for (unsigned i = 0; i < PictureFileCacheSlots; i++) {
        if (pf->slots[i].used && pf->slots[i].category == category && pf->slots[i].id == id) {
            return pf->slots[i].pixels;
        }
    }
    uint32_t offset, size;
    if (!pictureLocate(pf->game, category, id, &offset, &size)) {
        return NULL;
    }
    Slot *slot = &pf->slots[pf->next];
    pf->next = (pf->next + 1) % PictureFileCacheSlots;
    free(slot->pixels);
    slot->used = false;
    slot->pixels = malloc(size);
    if (!slot->pixels || fseek(pf->file, (long)offset, SEEK_SET) != 0 || fread(slot->pixels, 1, size, pf->file) != size) {
        free(slot->pixels);
        slot->pixels = NULL;
        return NULL;
    }
    slot->category = category;
    slot->id = id;
    slot->used = true;
    return slot->pixels;
}
