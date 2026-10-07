#include "cluemonster.h"

#include <string.h>

#include "bcd4.h"
#include "cluebook.h"
#include "clueitem.h"
#include "effect.h"
#include "font.h"

typedef enum { RowBcd, RowStat, RowImmune, RowResistant } RowKind;

typedef struct {
    uint8_t kind;
    uint16_t x, y;
    uint16_t label; /* data segment address of the label */
    uint16_t arg;   /* record field offset (BCD, stat) or flag mask (immune, resistant) */
} MonsterRow;

enum { MonsterRowCount = ClueMonsterRows };

static const MonsterRow kYendor2Rows[MonsterRowCount] = {
    {0, 185, 16, 0x7DBB, 0x008A},
    {0, 221, 22, 0x890B, 0x007E},
    {0, 191, 28, 0x7C61, 0x0086},
    {0, 215, 34, 0x7C6D, 0x0082},
    {1, 233, 46, 0x8911, 0x0050},
    {1, 221, 52, 0x8919, 0x0054},
    {1, 215, 58, 0x8923, 0x0056},
    {1, 209, 64, 0x892E, 0x0058},
    {1, 233, 70, 0x893A, 0x005A},
    {1, 203, 76, 0x8942, 0x0064},
    {1, 203, 82, 0x894F, 0x0066},
    {2, 221, 94, 0x895C, 0x8000},
    {2, 215, 100, 0x8964, 0x4000},
    {2, 203, 106, 0x896D, 0x2000},
    {2, 209, 112, 0x8978, 0x1000},
    {2, 221, 118, 0x8982, 0x0800},
    {2, 215, 124, 0x898A, 0x0400},
    {2, 233, 130, 0x8993, 0x0008},
    {2, 233, 136, 0x8999, 0x0004},
    {2, 209, 142, 0x899F, 0x0002},
    {2, 227, 148, 0x89A9, 0x0001},
    {3, 185, 154, 0x89B0, 0x3A00},
    {3, 167, 160, 0x89BE, 0xC000},
};
static const MonsterRow kYendor3Rows[MonsterRowCount] = {
    {0, 185, 16, 0x80E8, 0x008A},
    {0, 221, 22, 0x8C2C, 0x007E},
    {0, 221, 28, 0x7F93, 0x0086},
    {0, 215, 34, 0x7F9A, 0x0082},
    {1, 233, 46, 0x8C32, 0x0050},
    {1, 221, 52, 0x8C3A, 0x0054},
    {1, 215, 58, 0x8C44, 0x0056},
    {1, 209, 64, 0x8C4F, 0x0058},
    {1, 233, 70, 0x8C5B, 0x005A},
    {1, 203, 76, 0x8C63, 0x0064},
    {1, 203, 82, 0x8C70, 0x0066},
    {2, 221, 94, 0x8C7D, 0x8000},
    {2, 215, 100, 0x8C85, 0x4000},
    {2, 203, 106, 0x8C8E, 0x2000},
    {2, 209, 112, 0x8C99, 0x1000},
    {2, 221, 118, 0x8CA3, 0x0800},
    {2, 215, 124, 0x8CAB, 0x0400},
    {2, 233, 130, 0x8CB4, 0x0008},
    {2, 233, 136, 0x8CBA, 0x0004},
    {2, 209, 142, 0x8CC0, 0x0002},
    {2, 227, 148, 0x8CCA, 0x0001},
    {3, 185, 154, 0x8CD1, 0x3A00},
    {3, 167, 160, 0x8CDF, 0xC000},
};

static const struct {
    unsigned immuneMark, resistantMark, heading;
} kMarks[2] = {{0x89DF, 0x89E6, 0x8876}, {0x8D00, 0x8D07, 0x8B97}};

bool clueMonsterTextLoad(ClueMonsterText *t, const ExeData *exe, GameKind game) {
    const MonsterRow *rows = game == GameYendor2 ? kYendor2Rows : kYendor3Rows;
    for (unsigned i = 0; i < ClueMonsterRows; i++) {
        if (!exeDataString(exe, rows[i].label, t->label[i], sizeof(t->label[i]))) {
            return false;
        }
    }
    static const uint16_t kAttackWords[2][ClueAttackWordCount + 1] = {
        {0x8B6E, 0x8DFE, 0x8B8D, 0x8B96, 0x8BA0, 0x8BAB, 0x8BB4, 0x8BBE, 0x8BC8, 0x8BD1, 0x8BDB, 0x8BE8, 0x8BF4, 0x8B7D, 0x8B84, 0x8C02, 0x8C0F, 0x8C18, 0x89CF},
        {0x8E8B, 0x9115, 0x8EAA, 0x8EB3, 0x8EBD, 0x8EC8, 0x8ED1, 0x8EDB, 0x8EE5, 0x8EEE, 0x8EF8, 0x8F05, 0x8F12, 0x8E9A, 0x8EA1, 0x8F20, 0x8F2D, 0x8F36, 0x8CF0}};
    for (unsigned i = 0; i < ClueAttackWordCount; i++) {
        if (!exeDataString(exe, kAttackWords[game == GameYendor2 ? 0 : 1][i], t->attackWord[i], sizeof(t->attackWord[i]))) {
            return false;
        }
    }
    if (!exeDataString(exe, kAttackWords[game == GameYendor2 ? 0 : 1][ClueAttackWordCount], t->attackLabel, sizeof(t->attackLabel))) {
        return false;
    }
    const unsigned g = game == GameYendor2 ? 0 : 1;
    return exeDataString(exe, kMarks[g].immuneMark, t->immuneMark, sizeof(t->immuneMark)) &&
           exeDataString(exe, kMarks[g].resistantMark, t->resistantMark, sizeof(t->resistantMark)) &&
           exeDataString(exe, kMarks[g].heading, t->heading, sizeof(t->heading));
}

static void put(const ViewRenderer *r, int x, int y, const char *s, uint8_t colour) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, x, y, s, colour, 0, FontTransparent);
}

void clueMonsterAttackWords(const ClueMonsterText *t, GameKind game, const uint8_t *m, char out[160], bool *hasWords) {
    out[0] = 0;
    *hasWords = false;
    unsigned special = monsterGetU16(m, MonsterFieldSpecialAttack);
    unsigned flags = monsterGetU16(m, MonsterFieldFlags);
    if (special == 0 && !(flags & MonsterFlagAreaAttack)) {
        return;
    }
    *hasWords = true;
    if (flags & MonsterFlagAreaAttack) {
        strcat(out, t->attackWord[0]);
    }
    EffectDef def;
    if (!effectGetDef(game, special, &def)) {
        memset(&def, 0, sizeof(def));
    }
    static const uint16_t kBits[12] = {0x8000, 0x4000, 0x2000, 0x1000, 0x0800, 0x0400, 0x0200, 0x0100, 0x0080, 0x0001, 0x0004, 0x0002};
    for (unsigned i = 0; i < 12; i++) {
        if (def.costFlags & kBits[i]) {
            strcat(out, t->attackWord[1 + i]);
        }
    }
    if (flags & MonsterFlagSpecialMask) {
        strcat(out, t->attackWord[def.modeFlags & 0x200 ? 14 : 13]);
        if (flags & 0x0800) {
            strcat(out, t->attackWord[15]);
        }
        if (flags & 0x0400) {
            strcat(out, t->attackWord[16]);
        }
        if (flags & 0x0200) {
            strcat(out, t->attackWord[17]);
        }
    }
    size_t len = strlen(out);
    if (len >= 2) {
        out[len - 2] = 0; /* cut the last ", " */
    }
}

void clueMonsterPageDraw(const ViewRenderer *r, const ClueMonsterText *t, const uint8_t *m, const char *name, uint16_t navFlags) {
    memset(r->screen, 0, (size_t)ViewScreenWidth * ViewScreenHeight);
    viewDrawPicture(r, 0, r->game == GameYendor2 ? 13 : 6, 1, 1, false, 0);
    put(r, 6, 4, name, 0x0D);
    put(r, clueHeadingX(r->game, 2, (unsigned)strlen(t->heading)), 4, t->heading, 0x0D);
    clueNavBarDraw(r, navFlags);
    {
        /* the sprite: its first idle frame, category 2 at (8, 7) or, for the alternate large sprites, category 3 at (6, 33); the palette remap of
           MonsterFlagRemapPalette is not applied */
        bool large = (monsterGetU16(m, MonsterFieldFlags) & MonsterFlagAltSprite) != 0;
        viewDrawPicture(r, large ? 3 : 2, monsterGetU16(m, MonsterFieldSpriteBase), large ? 6 : 8, large ? 33 : 7, true, 0);
    }

    const MonsterRow *rows = r->game == GameYendor2 ? kYendor2Rows : kYendor3Rows;
    for (unsigned i = 0; i < ClueMonsterRows; i++) {
        const MonsterRow *row = &rows[i];
        put(r, row->x, row->y, t->label[i], 0x0A);
        switch (row->kind) {
        case RowBcd: {
            char text[12];
            if (bcd4AtLeastU16(m + row->arg, 1)) {
                bcd4Format(m + row->arg, text);
                put(r, 251, row->y, text, 0x8A);
            }
            break;
        }
        case RowStat: {
            unsigned value = monsterGetU16(m, row->arg);
            if (value) {
                char text[16];
                clueFormatNumber(value, 0, text);
                put(r, 275, row->y, text, 0x59);
            }
            break;
        }
        case RowImmune:
            if (monsterGetU16(m, MonsterFieldImmunities) & row->arg) {
                put(r, 263, row->y, t->immuneMark, 0xA7);
            }
            break;
        case RowResistant: {
            bool marked = (monsterGetU16(m, MonsterFieldResistances) & row->arg) != 0;
            if (row->y == 154 && (monsterGetU16(m, MonsterFieldImmunities) & 0x10)) {
                marked = true;
            }
            if (marked) {
                put(r, 263, row->y, t->resistantMark, 0xA7);
            }
            break;
        }
        }
    }
    char words[160];
    bool hasWords;
    clueMonsterAttackWords(t, r->game, m, words, &hasWords);
    int labelX = 171;
    if (hasWords && strlen(words) > 8) {
        labelX = (uint16_t)((35 - (int)strlen(words)) * 6); /* 16-bit like the original */
    }
    put(r, labelX, 169, t->attackLabel, 0x0A);
    if (hasWords) {
        put(r, labelX + 90, 169, words, 0xCA);
    }
}
