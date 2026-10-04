#include "charcreate.h"

#include <stdio.h>
#include <string.h>

#include "font.h"
#include "party.h"

unsigned charCreateBodyPicture(unsigned gender, unsigned portrait) {
    return 2 * (portrait - 1) + (gender == 1 ? 0 : 1);
}

unsigned charCreateFacePicture(unsigned gender, unsigned portrait) {
    return charCreateBodyPicture(gender, portrait) + CharCreateFaceBase;
}

static void put16(uint8_t *record, unsigned offset, unsigned value) {
    record[offset] = (uint8_t)value;
    record[offset + 1] = (uint8_t)(value >> 8);
}

void charCreateChoosePortrait(uint8_t *record, unsigned gender, unsigned portrait) {
    put16(record, 0x14, charCreateBodyPicture(gender, portrait));
    put16(record, 0x12, charCreateFacePicture(gender, portrait));
}

void charCreatePortraitGridDraw(const ViewRenderer *r, unsigned gender) {
    for (unsigned row = 0; row < 3; row++) {
        for (unsigned column = 0; column < 3; column++) {
            viewDrawPicture(r, 7, charCreateFacePicture(gender, row * 3 + column + 1), 8 + 33 * (int)column, 42 + 33 * (int)row, false, 0);
        }
    }
}

void charCreateClassPickDraw(const ViewRenderer *r) {
    static const char kHotkeys[10] = "FMROAPGDK";
    static const char *const kHeaders[3] = {"NON-MAGIC USERS:", "CLERIC TYPES:", "WIZARD TYPES:"};
    static const int kHeaderY[3] = {42, 87, 132}, kFirstY[3] = {51, 96, 141};
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, 25, "PICK A CLASS", 0x8A, 0, FontTransparent);
    for (unsigned group = 0; group < 3; group++) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, kHeaderY[group], kHeaders[group], 0x8A, 0, FontTransparent);
        for (unsigned k = 0; k < 3; k++) {
            unsigned id = group * 3 + k + 1;
            const char *name = partyClassName(id);
            if (!name) {
                continue;
            }
            int x = 8, y = kFirstY[group] + 9 * (int)k;
            for (const char *c = name; *c; c++) {
                x = fontDrawChar(r->game, 0, r->screen, ViewScreenWidth, x, y, (unsigned char)*c, *c == kHotkeys[id - 1] && c == strchr(name, kHotkeys[id - 1]) ? 0x7B : 0xF, 0,
                                 FontTransparent);
            }
        }
    }
}

static void drawHotkeyLabel(const ViewRenderer *r, int x, int y, const char *text, unsigned highlight) {
    for (unsigned i = 0; text[i]; i++) {
        x = fontDrawChar(r->game, 0, r->screen, ViewScreenWidth, x, y, (unsigned char)text[i], i == highlight ? 0x7B : 0xF, 0, FontTransparent);
    }
}

void charCreateExitLabelDraw(const ViewRenderer *r, bool returning) {
    if (returning) {
        drawHotkeyLabel(r, 8, 185, "RETURN", 1);
    } else {
        drawHotkeyLabel(r, 8, 185, "QUIT \"CREATE\"", 0);
    }
}

void charCreateItemLabel(const uint8_t *itemRecord, char out[2 * ItemNameLineSize + 2]) {
    char first[ItemNameLineSize], second[ItemNameLineSize];
    itemGetNameLine(itemRecord, 0, first);
    itemGetNameLine(itemRecord, 1, second);
    snprintf(out, 2 * ItemNameLineSize + 2, "%s %s", first, second);
    size_t length = strlen(out);
    while (length > 0 && out[length - 1] == ' ') {
        out[--length] = 0;
    }
}

void charCreateItemListDraw(const ViewRenderer *r, const ItemCatalog *catalog, const uint16_t itemIds[8], unsigned hiddenMask, bool returning) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, 25, "TAKE UP TO FOUR", 0x8A, 0, FontTransparent);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, 31, "ITEMS", 0x8A, 0, FontTransparent);
    for (unsigned row = 0; row < 8; row++) {
        if ((hiddenMask & (0x80u >> row)) || itemIds[row] == 0) {
            continue;
        }
        const uint8_t *item = itemCatalogRecord(catalog, itemIds[row]);
        if (!item) {
            continue;
        }
        int y = 0x2A + 0x10 * (int)row;
        viewDrawPicture(r, 8, itemGetU16(item, ItemFieldIcon), 8, y, true, 0);
        char label[2 * ItemNameLineSize + 2];
        charCreateItemLabel(item, label);
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 0x19, y + 5, label, 0xF, 0, FontTransparent);
    }
    if (!returning) {
        drawHotkeyLabel(r, 8, 176, "NAME CHARACTER", 0);
    }
    charCreateExitLabelDraw(r, returning);
}

void charCreateRollOptionsDraw(const ViewRenderer *r) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, 25, "SELECT AN", 0x8A, 0, FontTransparent);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, 31, "OPTION", 0x8A, 0, FontTransparent);
    drawHotkeyLabel(r, 8, 51, "ROLL ATTRIBUTES", 0);
    drawHotkeyLabel(r, 8, 69, "PICK ITEMS", 5);
    charCreateExitLabelDraw(r, false);
}

void charCreateSummaryDraw(const ViewRenderer *r) {
    static const char *const kOptions[6] = {"KEEP CHARACTER", "CLASS", "PORTRAIT", "ROLL ATTRIBUTES", "PICK ITEMS", "NAME CHARACTER"};
    static const unsigned kHotkey[6] = {0, 0, 0, 0, 5, 0};
    static const int kY[6] = {51, 69, 78, 87, 96, 105};
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, 25, "SELECT AN", 0x8A, 0, FontTransparent);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, 31, "OPTION", 0x8A, 0, FontTransparent);
    for (unsigned i = 0; i < 6; i++) {
        drawHotkeyLabel(r, 8, kY[i], kOptions[i], kHotkey[i]);
    }
    charCreateExitLabelDraw(r, false);
}

void charCreateNamePromptDraw(const ViewRenderer *r, const TextField *field) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, 25, "ENTER THE NAME", 0x8A, 0, FontTransparent);
    int x = 8;
    for (unsigned i = 0; i < CharCreateNameFieldSize; i++) {
        char c = i < field->length ? field->text[i] : (i == field->length ? '-' : ' ');
        x = fontDrawChar(r->game, 0, r->screen, ViewScreenWidth, x, 51, (unsigned char)c, 0xF, 0x33, FontOpaque);
    }
}

bool charCreateAcceptName(uint8_t *record, const TextField *field) {
    unsigned length = field->length;
    while (length > 0 && field->text[length - 1] == ' ') {
        length--;
    }
    if (length == 0) {
        return false;
    }
    memset(record + PartyFieldName, 0, PartyNameBufferSize);
    memcpy(record + PartyFieldName, field->text, length < PartyNameMaxLength ? length : PartyNameMaxLength);
    return true;
}
