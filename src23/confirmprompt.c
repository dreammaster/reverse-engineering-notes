#include "confirmprompt.h"

#include <string.h>

#include "font.h"

unsigned confirmPromptTableOffset(GameKind game) {
    return game == GameYendor2 ? 0xA787 : 0xE265;
}

bool confirmPromptLoad(const ExeData *exe, GameKind game, unsigned id, ConfirmPrompt *out) {
    memset(out, 0, sizeof(*out));
    if (!exe || id == 0 || id > 0x30) {
        return false;
    }
    unsigned record = exeDataU16(exe, confirmPromptTableOffset(game) + (id - 1) * 2);
    if (record == 0) {
        return false;
    }
    out->flags = (uint16_t)exeDataU16(exe, record);
    unsigned groups = exeDataU16(exe, record + 2);
    out->firstRegion = exeDataU16(exe, record + 4);
    out->lastRegion = exeDataU16(exe, record + 6);
    if (groups > 6) {
        return false;
    }
    for (unsigned g = 0; g < groups; g++) {
        unsigned entry = record + 8 + 10 * g;
        unsigned colour = exeDataU16(exe, entry), x = exeDataU16(exe, entry + 2), y = exeDataU16(exe, entry + 4);
        unsigned text = exeDataU16(exe, entry + 6), lines = exeDataU16(exe, entry + 8);
        for (unsigned k = 0; k < lines && out->lineCount < ConfirmPromptMaxLines; k++) {
            ConfirmPromptLine *line = &out->lines[out->lineCount++];
            line->colour = (uint8_t)colour;
            line->x = (int)x;
            line->y = (int)y + 6 * (int)k;
            if (!exeDataString(exe, text, line->text, sizeof(line->text))) {
                return false;
            }
            text += (unsigned)strlen(line->text) + 1;
        }
    }
    return true;
}

void confirmPromptDraw(const ViewRenderer *r, const ConfirmPrompt *prompt) {
    for (unsigned i = 0; i < prompt->lineCount; i++) {
        const ConfirmPromptLine *line = &prompt->lines[i];
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, line->x, line->y, line->text, line->colour, 4, FontTransparent);
    }
}
