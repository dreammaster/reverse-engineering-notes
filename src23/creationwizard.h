#ifndef YENDOR23_CREATIONWIZARD_H
#define YENDOR23_CREATIONWIZARD_H

#include <stdbool.h>
#include <stdint.h>

#include "charcreate.h"
#include "creation.h"
#include "savegame.h"
#include "textfield.h"

/*
 * The title menu's character creation (C: ShowPartyMembers yendor2.asm:35211, with ShowCreateCharacterPrompt :37602) as a state machine over the screens charcreate.h draws.
 * The original runs one record through its steps and, from the summary, back into any of them:
 *   Class      ShowCharacterSkills (:35989): the hotkeys F M R O A P G D K pick class base 1-9 (rolls the attributes), Q quits creation
 *   Portrait   ShowCharacterEquipment (:36582): 1-9 pick a portrait of the current gender, M / F switch the gender (word +0x10), Q quits
 *   Roll       ShowCharacterStats (:36743): R rolls again, I accepts and goes on to the items, Q quits
 *   Items      ShowCharacterInventory (:36150): up to four of the eight fixed items (creation.h); the original takes them by click -- the digits 1-8 do here; N (or the fourth
 *              item) goes on to the name, Q quits
 *   Name       EditCharacterName (:36518): the text field of 12 characters; Enter keeps a non-empty name, Escape returns to the summary
 *   Summary    ShowCharacterSummary (:37176): K keeps the hero (ApplySecondaryClassTierFlags), C / P / R / I / N go back to the class, portrait, roll, item or name step (and
 *              return here), Q discards
 * The first pass runs Class, Portrait, Roll, Items, Name, Summary in that order. Re-entering the item step starts the pick over (the hero's items are cleared).
 * The record is the hero being made; when the wizard reports Done it is ready to be stored in an empty roster slot (the first record whose class [+0x0E] is 0).
 */
typedef enum {
    CreationStepClass,
    CreationStepPortrait,
    CreationStepRoll,
    CreationStepItems,
    CreationStepName,
    CreationStepSummary,
    CreationStepDone,
    CreationStepCancelled
} CreationStep;

typedef struct {
    GameKind game;
    const ItemCatalog *catalog;
    RandomState *rng;
    CreationStep step;
    bool firstPass; /* false once the summary has been reached: the steps then return to it */
    uint8_t record[SavePartyRecordSize];
    unsigned gender;     /* 1 male, 2 female */
    unsigned hiddenMask; /* items already taken: 0x80 for the first row down to 0x01 */
    unsigned taken;
    uint16_t offered[CreationItemChoices];
    TextField name;
} CreationWizard;

void creationWizardStart(CreationWizard *w, GameKind game, const ItemCatalog *catalog, RandomState *rng);

/* A key: lower-case letters and digits as themselves, 13 Enter, 27 Escape, 8 Backspace, ' '. False once the wizard is Done or Cancelled. */
bool creationWizardKey(CreationWizard *w, int key);

/* The current screen over the picture-3 backdrop (the sheet draws it itself). */
void creationWizardDraw(const CreationWizard *w, const ViewRenderer *r);

#endif
