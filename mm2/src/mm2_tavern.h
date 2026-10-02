/* Tavern (ovl/2BRAIN.asm tavern_menu IDA 0x1D15A and its submenus 0x1CB08..0x1D0B4; docs/party-commands.md). */
#ifndef MM2_TAVERN_H
#define MM2_TAVERN_H

#include "mm2_combat.h"
#include "mm2_inn.h"
#include "mm2_town.h"

typedef enum {
	MM2_TAVERN_OK,
	MM2_TAVERN_NO_GOLD,
	MM2_TAVERN_DISABLED,   /* the character is not in good condition */
	MM2_TAVERN_SICK        /* the roll went wrong: poisoned (drink) or diseased (specialty) */
} Mm2TavernResult;

typedef struct {
	int drinks[6];   /* drinks taken per kind during this visit (reset on entry) */
} Mm2TavernVisit;

/* A: Feeding frenzy - pays the town's price once; every party member's food is raised to 40. */
Mm2TavernResult mm2_tavern_feed(Mm2Roster *r, Mm2Char *payer, int town);
/* B: drink `kind` (0-5). */
Mm2TavernResult mm2_tavern_drink(Mm2Char *c, int kind, Mm2TavernVisit *visit, const Mm2Rng *rng);
/* C: specialty `idx` (0-2) of the town: sets the town's flag bits in the character's word at +76h. */
Mm2TavernResult mm2_tavern_specialty(Mm2Char *c, int town, int idx, const Mm2Rng *rng);
/* D: tip the bartender (1 gold): returns 1 and *rumour (rumour index of the day) with a 1 in (endurance bracket + 5) chance. */
Mm2TavernResult mm2_tavern_tip(Mm2Char *c, int *heard, int *rumour, int dayOfYear, const Mm2Rng *rng);
/* The rumour index chosen from the day of the year (sub_1CA46). */
int mm2_tavern_rumour_index(int dayOfYear);

uint32_t mm2_tavern_specialty_price(int town, int idx);
uint32_t mm2_tavern_drink_price(int kind);

#endif
