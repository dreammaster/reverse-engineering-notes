#include "creationwizard.h"

#include <string.h>

#include "chargen.h"
#include "font.h"
#include "party.h"
#include "statsheet.h"

static void put16(uint8_t *record, unsigned offset, unsigned value) {
    record[offset] = (uint8_t)value;
    record[offset + 1] = (uint8_t)(value >> 8);
}

void creationWizardStart(CreationWizard *w, GameKind game, const ItemCatalog *catalog, RandomState *rng) {
    memset(w, 0, sizeof(*w));
    w->game = game;
    w->catalog = catalog;
    w->rng = rng;
    w->step = CreationStepClass;
    w->firstPass = true;
    w->gender = 1;
    put16(w->record, 0x10, 1);
    creationItemIds(game, w->offered);
    textFieldInit(&w->name, CharCreateNameFieldSize);
}

static void clearItems(CreationWizard *w) {
    uint8_t *group = partyInventoryGroup(w->record, PartyGroupMain);
    for (unsigned slot = 1; slot <= CreationItemsMax; slot++) {
        itemSlotSet(inventoryGroupSlot(group, slot), 0, 0);
    }
    inventoryGroupSetWeight(group, 0);
    w->hiddenMask = 0;
    w->taken = 0;
}

static void takeItem(CreationWizard *w, unsigned pick) {
    if (pick >= CreationItemChoices || (w->hiddenMask & (0x80u >> pick)) || w->taken >= CreationItemsMax) {
        return;
    }
    const uint8_t *item = itemCatalogRecord(w->catalog, w->offered[pick]);
    uint8_t *group = partyInventoryGroup(w->record, PartyGroupMain);
    itemSlotSet(inventoryGroupSlot(group, 1 + w->taken), w->offered[pick], 0);
    if (item) {
        inventoryGroupSetWeight(group, (uint16_t)(inventoryGroupWeight(group) + (item[ItemFieldWeight] | (item[ItemFieldWeight + 1] << 8))));
    }
    w->hiddenMask |= 0x80u >> pick;
    w->taken++;
    partyRecomputeEquipmentStatBonuses(w->record, w->catalog, w->game);
}

static void enter(CreationWizard *w, CreationStep step) {
    w->step = step;
    if (step == CreationStepItems) {
        clearItems(w);
    } else if (step == CreationStepName) {
        textFieldInit(&w->name, CharCreateNameFieldSize);
    }
}

/* After a step finishes: the next one on the first pass, the summary afterwards. */
static void advance(CreationWizard *w, CreationStep next) {
    enter(w, w->firstPass ? next : CreationStepSummary);
}

static bool classKey(CreationWizard *w, int key) {
    static const char kHotkeys[] = "fmroapgdk";
    const char *at = key > 0 && key < 128 ? strchr(kHotkeys, key) : NULL;
    if (key == 'q') {
        w->step = CreationStepCancelled;
    } else if (at && *at) {
        partyBeginClassSelection(w->record);
        partyChooseClass(w->record, (unsigned)(at - kHotkeys) + 1, w->game, w->catalog, w->rng);
        advance(w, CreationStepPortrait);
    }
    return true;
}

static bool portraitKey(CreationWizard *w, int key) {
    if (key == 'q') {
        w->step = CreationStepCancelled;
    } else if (key == 'm' || key == 'f') {
        w->gender = key == 'm' ? 1 : 2;
        put16(w->record, 0x10, w->gender);
    } else if (key >= '1' && key <= '9') {
        charCreateChoosePortrait(w->record, w->gender, (unsigned)(key - '0'));
        advance(w, CreationStepRoll);
    }
    return true;
}

static bool rollKey(CreationWizard *w, int key) {
    if (key == 'q') {
        w->step = CreationStepCancelled;
    } else if (key == 'r') {
        partyRerollAttributes(w->record, w->game, w->catalog, w->rng);
    } else if (key == 'i') {
        advance(w, CreationStepItems);
    }
    return true;
}

static bool itemsKey(CreationWizard *w, int key) {
    if (key == 'q') {
        w->step = CreationStepCancelled;
    } else if (key == 'n') {
        advance(w, CreationStepName);
    } else if (key >= '1' && key <= '8') {
        takeItem(w, (unsigned)(key - '1'));
        if (w->taken >= CreationItemsMax) {
            advance(w, CreationStepName);
        }
    }
    return true;
}

static bool nameKey(CreationWizard *w, int key) {
    if (key == 27) {
        w->step = CreationStepSummary;
        w->firstPass = false;
        return true;
    }
    if (key >= 'a' && key <= 'z') {
        key -= 32;
    }
    TextFieldResult result = textFieldKey(&w->name, (uint8_t)key);
    if (result == TextFieldConfirmed && charCreateAcceptName(w->record, &w->name)) {
        w->firstPass = false; /* the summary is next whichever way the name step was reached */
        w->step = CreationStepSummary;
    }
    return true;
}

static bool summaryKey(CreationWizard *w, int key) {
    switch (key) {
    case 'k':
        partyApplyStartingAbilities(w->record, w->game);
        w->step = CreationStepDone;
        break;
    case 'c':
        enter(w, CreationStepClass);
        break;
    case 'p':
        enter(w, CreationStepPortrait);
        break;
    case 'r':
        partyRerollAttributes(w->record, w->game, w->catalog, w->rng);
        break;
    case 'i':
        enter(w, CreationStepItems);
        break;
    case 'n':
        enter(w, CreationStepName);
        break;
    case 'q':
        w->step = CreationStepCancelled;
        break;
    default:
        break;
    }
    return true;
}

bool creationWizardKey(CreationWizard *w, int key) {
    switch (w->step) {
    case CreationStepClass:
        classKey(w, key);
        break;
    case CreationStepPortrait:
        portraitKey(w, key);
        break;
    case CreationStepRoll:
        rollKey(w, key);
        break;
    case CreationStepItems:
        itemsKey(w, key);
        break;
    case CreationStepName:
        nameKey(w, key);
        break;
    case CreationStepSummary:
        summaryKey(w, key);
        break;
    default:
        break;
    }
    if (w->step == CreationStepSummary) { /* reaching the summary ends the first pass */
        w->firstPass = false;
    }
    return w->step != CreationStepDone && w->step != CreationStepCancelled;
}

void creationWizardDraw(const CreationWizard *w, const ViewRenderer *r) {
    static const uint16_t kNoRoles[5];
    memset(r->screen, 0, (size_t)ViewScreenWidth * ViewScreenHeight);
    bool sheet = w->step == CreationStepRoll || w->step == CreationStepItems || w->step == CreationStepName || w->step == CreationStepSummary;
    if (sheet) {
        characterSheetDraw(r, w->catalog, w->record, 0, kNoRoles, "CHARACTER CREATION");
    } else {
        viewDrawPicture(r, 0, 3, 1, 1, false, 0);
    }
    switch (w->step) {
    case CreationStepClass:
        charCreateClassPickDraw(r);
        charCreateExitLabelDraw(r, false);
        break;
    case CreationStepPortrait:
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 8, 25, "PICK A PORTRAIT", 0x8A, 0, FontTransparent);
        charCreatePortraitGridDraw(r, w->gender);
        charCreateExitLabelDraw(r, false);
        break;
    case CreationStepRoll:
        charCreateRollOptionsDraw(r);
        break;
    case CreationStepItems:
        charCreateItemListDraw(r, w->catalog, w->offered, w->hiddenMask, false);
        break;
    case CreationStepName:
        charCreateNamePromptDraw(r, &w->name);
        break;
    case CreationStepSummary:
        charCreateSummaryDraw(r);
        break;
    default:
        break;
    }
}
