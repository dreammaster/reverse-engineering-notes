/* Text-mode screens drawn on the 320x200 canvas (shared by the SDL explorer and the screenshot tool). */
#ifndef MM2_UI_H
#define MM2_UI_H

#include "mm2_data.h"
#include "mm2_inn.h"
#include "mm2_party.h"
#include "mm2_smith.h"
#include "mm2_spells.h"
#include "mm2_strings.h"
#include "mm2_tavern.h"
#include "mm2_text.h"
#include "mm2_town.h"

void mm2_ui_draw_inn(uint8_t *canvas, const Mm2Font *font, const Mm2Roster *roster, int town);

/* Character sheet (stats, vitals, equipment names from `items`). */
void mm2_ui_draw_sheet(uint8_t *canvas, const Mm2Font *font, const Mm2Char *c, const Mm2Item *items);

/* Training hall: one line per party member with level, experience needed, cost and whether training is possible. */
void mm2_ui_draw_training(uint8_t *canvas, const Mm2Font *font, const Mm2Roster *roster, int town, const char *message);

/* Temple (guild = 0) or mage guild (guild = 1) for the party member `slot`. */
void mm2_ui_draw_temple(uint8_t *canvas, const Mm2Font *font, const Mm2Roster *roster, int town, int slot, int guild, const char *message);
/* Blacksmith: mode 1-4 buy categories, 5 sell, 6 identify. */
void mm2_ui_draw_smith(uint8_t *canvas, const Mm2Font *font, const Mm2Roster *roster, int town, int slot, int mode, int day,
					   const Mm2Item *items, const char *message);

/* Tavern: submenu 0 = main menu, 1 = drinks, 2 = specialties; `lines` = up to 3 free message lines shown at the bottom. */
void mm2_ui_draw_tavern(uint8_t *canvas, const Mm2Font *font, const Mm2Roster *roster, int town, int slot, int submenu,
						const Mm2BuildingText *text, const char *const lines[3]);

const char *mm2_ui_class_name(int cls);
const char *mm2_ui_race_name(int race);

#endif
