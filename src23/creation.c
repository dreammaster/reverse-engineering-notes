#include "creation.h"

#include <string.h>

void creationItemIds(GameKind game, uint16_t out[CreationItemChoices]) {
    static const uint16_t kYendor2[CreationItemChoices] = {0x21E, 0x19C, 0x1A2, 0x1A8, 0x16A, 0x138, 6, 0x36};
    static const uint16_t kYendor3[CreationItemChoices] = {8, 0x0E, 0x12, 0x16, 4, 0x19, 0x1C, 0x1F};
    memcpy(out, game == GameYendor3 ? kYendor3 : kYendor2, sizeof(kYendor2));
}

bool creationMakeHero(uint8_t *record, GameKind game, const ItemCatalog *catalog, RandomState *rng, const CreationChoice *choice) {
    memset(record, 0, SavePartyRecordSize);
    if (choice->gender < 1 || choice->gender > 2 || choice->portrait < 1 || choice->portrait > CharCreatePortraits || choice->classBase < 1 || choice->classBase > 9) {
        return false;
    }
    TextField field;
    textFieldInit(&field, 13);
    for (const char *p = choice->name; p && *p; p++) {
        textFieldKey(&field, (uint8_t)*p);
    }
    if (!charCreateAcceptName(record, &field)) {
        return false;
    }
    partySetU16(record, 0x10, (uint16_t)choice->gender);
    charCreateChoosePortrait(record, choice->gender, choice->portrait);
    partyBeginClassSelection(record);
    partyChooseClass(record, choice->classBase, game, catalog, rng);
    uint16_t offered[CreationItemChoices];
    creationItemIds(game, offered);
    unsigned taken = 0, mask = 0, slot = 1;
    uint8_t *group = partyInventoryGroup(record, PartyGroupMain);
    for (unsigned i = 0; i < CreationItemsMax; i++) {
        unsigned pick = choice->items[i];
        if (pick >= CreationItemChoices || (mask & (1u << pick))) {
            continue;
        }
        mask |= 1u << pick;
        const uint8_t *item = itemCatalogRecord(catalog, offered[pick]);
        itemSlotSet(inventoryGroupSlot(group, slot++), offered[pick], 0);
        if (item) {
            inventoryGroupSetWeight(group, (uint16_t)(inventoryGroupWeight(group) + (item[ItemFieldWeight] | (item[ItemFieldWeight + 1] << 8))));
        }
        taken++;
    }
    (void)taken;
    partyRecomputeEquipmentStatBonuses(record, catalog, game);
    partyApplyStartingAbilities(record, game);
    return true;
}

void creationReroll(uint8_t *record, GameKind game, const ItemCatalog *catalog, RandomState *rng) {
    partyRerollAttributes(record, game, catalog, rng);
}
