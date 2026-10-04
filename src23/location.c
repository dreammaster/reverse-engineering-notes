#include "location.h"

#include <string.h>

typedef struct {
    size_t records, names;
    unsigned blocks;
} Layout;

static const Layout kYendor2 = {460800, 461520, 120};
static const Layout kYendor3 = {537600, 538440, 140};

bool locationName(GameKind game, const uint8_t *worldDat, size_t size, unsigned block, const char *levelTemplate, const char *mapTemplate, LocationName *out) {
    const Layout *l = game == GameYendor2 ? &kYendor2 : &kYendor3;
    if (block >= l->blocks || l->records + 6 * (size_t)(block + 1) > size) {
        return false;
    }
    const uint8_t *record = worldDat + l->records + 6 * (size_t)block;
    size_t nameAt = l->names + 20 * (size_t)record[4];
    if (nameAt + 20 > size) {
        return false;
    }
    memset(out, 0, sizeof(*out));
    memcpy(out->name, worldDat + nameAt, 20);
    size_t length = 20;
    while (length > 0 && (out->name[length - 1] == ' ' || out->name[length - 1] == 0)) {
        out->name[--length] = 0;
    }
    const unsigned width = game == GameYendor2 ? 1 : 3;
    const char *tail = NULL;
    if (record[0] != '0') {
        strncpy(out->suffix, levelTemplate, sizeof(out->suffix) - 1);
        tail = (const char *)record;
        out->kind = 2;
        /* the placeholder characters are the last `width` of the template (" LEVEL X" / " LEVEL XXX") */
        size_t at = strlen(out->suffix) - width;
        memcpy(out->suffix + at, tail, width);
    } else if (record[1] != ' ') {
        strncpy(out->suffix, mapTemplate, sizeof(out->suffix) - 1);
        tail = (const char *)record + 1;
        out->kind = 1;
        size_t at = strlen(out->suffix) - width;
        memcpy(out->suffix + at, tail, width);
    }
    return true;
}
