/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_charcreate test_charcreate.c ../charcreate.c ../textfield.c ../font.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c ../uiregions.c && ./test_charcreate
 *
 * Face ids 20-33 are what the real Chapter 2 heroes carry.
 */
#include <stdio.h>
#include <string.h>

#include "charcreate.h"
#include "pictures.h"
#include "party.h"
#include "textfield.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static const uint8_t *face(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[32 * 32];
    (void)ctx;
    (void)category;
    memset(pixels, (uint8_t)id, sizeof(pixels));
    return pixels;
}

int main(void) {
    check("male portrait 1: body 0, face 19", charCreateBodyPicture(1, 1) == 0 && charCreateFacePicture(1, 1) == 19);
    check("female portrait 1: body 1, face 20", charCreateBodyPicture(2, 1) == 1 && charCreateFacePicture(2, 1) == 20);
    check("portrait 9: bodies 16 / 17, faces 35 / 36", charCreateBodyPicture(1, 9) == 16 && charCreateBodyPicture(2, 9) == 17 && charCreateFacePicture(1, 9) == 35 &&
                                                              charCreateFacePicture(2, 9) == 36);
    uint8_t record[500];
    memset(record, 0, sizeof(record));
    charCreateChoosePortrait(record, 2, 4);
    check("choosing writes [+0x14] body and [+0x12] face", record[0x14] == 7 && record[0x12] == 7 + 0x13);
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, face, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    charCreatePortraitGridDraw(&r, 1);
    check("the grid: nine faces, row by row, 33 pixels apart from (8, 42)", screen[42 * 320 + 8] == 19 && screen[42 * 320 + 41] == 21 && screen[75 * 320 + 8] == 25 &&
                                                                             screen[108 * 320 + 74] == 35 && screen[41 * 320 + 8] == 0xEE && screen[42 * 320 + 40] == 0xEE);
    memset(screen, 0xEE, sizeof(screen));
    charCreateClassPickDraw(&r);
    bool header = false, hot = false, normal = false, second = false;
    for (int y = 25; y < 31; y++) {
        for (int x = 8; x < 80; x++) {
            header = header || screen[y * 320 + x] == 0x8A;
        }
    }
    for (int y = 51; y < 57; y++) {
        for (int x = 8; x < 14; x++) {
            hot = hot || screen[y * 320 + x] == 0x7B; /* the F of FIGHTER */
        }
        for (int x = 14; x < 50; x++) {
            normal = normal || screen[y * 320 + x] == 0xF;
        }
    }
    for (int y = 105; y < 111; y++) {
        for (int x = 8; x < 20; x++) {
            second = second || screen[y * 320 + x] == 0x7B; /* ALCHEMIST: the A is first */
        }
    }
    check("the class screen: header, FIGHTER with a highlighted F, ALCHEMIST at the second row of the second group", header && hot && normal && second);

    static ItemCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.itemCount = 10;
    memcpy(catalog.items + 1 * ItemRecordSize + ItemFieldName1, "LONG SWORD   ", 13);
    memcpy(catalog.items + 1 * ItemRecordSize + ItemFieldName2, "+1           ", 13);
    memcpy(catalog.items + 2 * ItemRecordSize + ItemFieldName1, "ROPE         ", 13);
    catalog.items[1 * ItemRecordSize + ItemFieldIcon] = 5;
    catalog.items[2 * ItemRecordSize + ItemFieldIcon] = 6;
    char label[2 * ItemNameLineSize + 2];
    charCreateItemLabel(catalog.items + 1 * ItemRecordSize, label);
    check("an item's label joins both name fields with a space", strcmp(label, "LONG SWORD +1") == 0);
    charCreateItemLabel(catalog.items + 2 * ItemRecordSize, label);
    check("...and drops trailing spaces when the second is empty", strcmp(label, "ROPE") == 0);
    memset(screen, 0xEE, sizeof(screen));
    uint16_t ids[8] = {2, 3, 2, 0, 0, 0, 0, 3};
    charCreateItemListDraw(&r, &catalog, ids, 0x20 | 0x01, false); /* row 3 and row 8 hidden */
    bool header2 = false, label1 = false, labelHidden = false, nameHot = false, quitHot = false;
    for (int y = 31; y < 37; y++) {
        for (int x = 8; x < 40; x++) {
            header2 = header2 || screen[y * 320 + x] == 0x8A;
        }
    }
    for (int y = 47; y < 53; y++) {
        for (int x = 25; x < 60; x++) {
            label1 = label1 || screen[y * 320 + x] == 0xF;
        }
    }
    for (int y = 74; y < 80; y++) {
        for (int x = 25; x < 60; x++) {
            labelHidden = labelHidden || screen[y * 320 + x] == 0xF;
        }
    }
    for (int y = 176; y < 182; y++) {
        for (int x = 8; x < 14; x++) {
            nameHot = nameHot || screen[y * 320 + x] == 0x7B;
        }
    }
    for (int y = 185; y < 191; y++) {
        for (int x = 8; x < 14; x++) {
            quitHot = quitHot || screen[y * 320 + x] == 0x7B;
        }
    }
    check("the item pick: second header line, a label beside the first icon, nothing in a hidden row", header2 && label1 && !labelHidden);
    check("icons: item 2's picture (5) at (8, 42), item 3's (6) at (8, 58), hidden rows empty", screen[42 * 320 + 8] == 5 && screen[58 * 320 + 8] == 6 && screen[26 * 320 + 8] != 5 &&
                                                                                       screen[74 * 320 + 8] == 0xEE && screen[154 * 320 + 8] == 0xEE);
    check("NAME CHARACTER and QUIT \"CREATE\" carry highlighted first letters", nameHot && quitHot);
    memset(screen, 0xEE, sizeof(screen));
    charCreateItemListDraw(&r, &catalog, ids, 0, true);
    bool anyName = false;
    for (int y = 176; y < 182; y++) {
        for (int x = 8; x < 100; x++) {
            anyName = anyName || screen[y * 320 + x] != 0xEE;
        }
    }
    bool returnE = false;
    for (int y = 185; y < 191; y++) {
        for (int x = 14; x < 20; x++) {
            returnE = returnE || screen[y * 320 + x] == 0x7B;
        }
    }
    check("viewing an existing hero: no NAME CHARACTER, RETURN with its E highlighted", !anyName && returnE);

    memset(screen, 0xEE, sizeof(screen));
    charCreateRollOptionsDraw(&r);
    bool rollHot = false, pickHot = false, pickIPlain = false;
    for (int y = 51; y < 57; y++) {
        for (int x = 8; x < 14; x++) {
            rollHot = rollHot || screen[y * 320 + x] == 0x7B;
        }
    }
    for (int y = 69; y < 75; y++) {
        for (int x = 8; x < 14; x++) {
            pickIPlain = pickIPlain || screen[y * 320 + x] == 0x7B; /* the P is not the hotkey */
        }
        for (int x = 38; x < 44; x++) {
            pickHot = pickHot || screen[y * 320 + x] == 0x7B; /* PICK I: the sixth character */
        }
    }
    check("the roll screen: R of ROLL ATTRIBUTES and the I of PICK ITEMS are the highlighted hotkeys", rollHot && pickHot && !pickIPlain);

    memset(screen, 0xEE, sizeof(screen));
    charCreateSummaryDraw(&r);
    bool rows[6] = {false}, pickHotI = false;
    static const int summaryY[6] = {51, 69, 78, 87, 96, 105};
    for (unsigned i = 0; i < 6; i++) {
        for (int y = summaryY[i]; y < summaryY[i] + 6; y++) {
            for (int x = 8; x < 60; x++) {
                rows[i] = rows[i] || screen[y * 320 + x] == 0xF || screen[y * 320 + x] == 0x7B;
            }
        }
    }
    for (int y = 96; y < 102; y++) {
        for (int x = 38; x < 44; x++) {
            pickHotI = pickHotI || screen[y * 320 + x] == 0x7B;
        }
    }
    check("the summary menu: six option rows (51, 69, 78, 87, 96, 105), PICK ITEMS highlighting its I",
          rows[0] && rows[1] && rows[2] && rows[3] && rows[4] && rows[5] && pickHotI);

    TextField field;
    textFieldInit(&field, CharCreateNameFieldSize);
    for (const char *k = "GRIMBLEWORTHXYZ"; *k; k++) {
        textFieldKey(&field, (uint8_t)*k);
    }
    check("the name field keeps 12 characters, the rest beep", field.length == 12 && strcmp(field.text, "GRIMBLEWORTH") == 0 && textFieldKey(&field, 'Q') == TextFieldBeep);
    memset(screen, 0xEE, sizeof(screen));
    textFieldInit(&field, CharCreateNameFieldSize);
    textFieldKey(&field, 'A');
    charCreateNamePromptDraw(&r, &field);
    check("the prompt: opaque field of 13 cells on 0x33, text then cursor",
          screen[51 * 320 + 8 + 77] == 0x33 && screen[56 * 320 + 8] != 0xEE && screen[50 * 320 + 8] == 0xEE && screen[51 * 320 + 8 + 78] == 0xEE);
    memset(record, 0xAA, sizeof(record));
    textFieldInit(&field, CharCreateNameFieldSize);
    for (const char *k = "BOB  "; *k; k++) {
        textFieldKey(&field, (uint8_t)*k);
    }
    check("accepting a name trims trailing spaces and stores it NUL-terminated",
          charCreateAcceptName(record, &field) && strcmp((const char *)record, "BOB") == 0 && record[PartyNameMaxLength] == 0);
    textFieldInit(&field, CharCreateNameFieldSize);
    textFieldKey(&field, ' ');
    check("a blank name is refused", !charCreateAcceptName(record, &field));

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}
