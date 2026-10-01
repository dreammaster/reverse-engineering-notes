/* TODO(review): these screens are my own simplified layouts, NOT the original ones.  The original texts (building titles,
 * "Sorry - you need more gold.", prices, menus) come from STR.DAT through the per-building string tables, e.g. 2SMITH
 * blacksmith_menu (IDA 0x1CCBA), 1RETINN inn_menu (0x1C5C0), 2MISC2 training_hall (0x1CE30); the original draws them in
 * windows with the 8x8 font from fixed text cells that I did not copy. */
#include "mm2_ui.h"
#include "mm2_gfx.h"

#include <stdio.h>
#include <string.h>

static const char *const CLASS_NAME[8] = {"Knight", "Paladin", "Archer", "Cleric", "Sorcerer", "Robber", "Ninja", "Barbarian"};
static const char *const RACE_NAME[5] = {"Human", "Elf", "Dwarf", "Gnome", "H-Orc"};

void mm2_ui_draw_inn(uint8_t *canvas, const Mm2Font *font, const Mm2Roster *roster, int town) {
	int list[MM2_ROSTER_CHARS], n, i, row = 2;
	char line[48];
	memset(canvas, 0, MM2_SCREEN_W * MM2_SCREEN_H);
	mm2_draw_text(canvas, font, 14, 0, " Inn ", 14, -1);
	n = mm2_inn_list(roster, town, list);
	for (i = 0; i < n && i < 24; i++) {
		const Mm2Char *c = &roster->chars[list[i]];
		int inParty = 0, k;
		for (k = 0; k < mm2_party_size(roster); k++)
			if (mm2_party_member(roster, k) == list[i]) inParty = 1;
		snprintf(line, sizeof(line), "%c) %-11.11s %-8s %-6s L%-2d %s", 'A' + i, (const char *)c->raw,
				 CLASS_NAME[c->raw[MC_CLASS] & 7], RACE_NAME[c->raw[MC_RACE] % 5], c->raw[MC_LEVEL], inParty ? "*" : "");
		mm2_draw_text(canvas, font, 0, row++, line, inParty ? 10 : 15, -1);
	}
	if (!n) mm2_draw_text(canvas, font, 0, row, "Nobody lives in this town.", 7, -1);
	snprintf(line, sizeof(line), "Party: %d (max 6 + 2 hirelings)", mm2_party_size(roster));
	mm2_draw_text(canvas, font, 0, 22, line, 11, -1);
	mm2_draw_text(canvas, font, 0, 23, "A-X add/remove   Z or Esc: leave", 7, -1);
}


const char *mm2_ui_class_name(int cls) { return CLASS_NAME[cls & 7]; }
const char *mm2_ui_race_name(int race) { return RACE_NAME[race % 5]; }

static const char *const STAT_NAME[6] = {"Might", "Intellect", "Personality", "Speed", "Accuracy", "Luck"};
static const char *const ALIGN_NAME[3] = {"Good", "Neutral", "Evil"};

static const char *condition_name(unsigned cond) {
	if (cond == 0) return "Good";
	if (cond == 0xFF) return "Eradicated";
	if (cond >= 0x82) return "Stone";
	if (cond >= 0x80) return "Dead";
	if (cond & 0x40) return "Unconscious";
	if (cond & 0x20) return "Paralyzed";
	if (cond & 0x10) return "Asleep";
	if (cond & 0x08) return "Poisoned";
	if (cond & 0x04) return "Diseased";
	if (cond & 0x02) return "Silenced";
	return "Cursed";
}

void mm2_ui_draw_sheet(uint8_t *canvas, const Mm2Font *font, const Mm2Char *c, const Mm2Item *items) {
	char line[64];
	int i;
	memset(canvas, 0, MM2_SCREEN_W * MM2_SCREEN_H);
	snprintf(line, sizeof(line), "%.11s", (const char *)c->raw);
	mm2_draw_text(canvas, font, 0, 0, line, 14, -1);
	snprintf(line, sizeof(line), "%s %s %s %s", RACE_NAME[mm2_c8(c, MC_RACE) % 5], CLASS_NAME[mm2_c8(c, MC_CLASS) & 7],
			 mm2_c8(c, MC_SEX) ? "F" : "M", ALIGN_NAME[mm2_c8(c, MC_ALIGN) % 3]);
	mm2_draw_text(canvas, font, 0, 1, line, 11, -1);
	for (i = 0; i < 6; i++) {
		snprintf(line, sizeof(line), "%-12s %3d/%3d", STAT_NAME[i], mm2_c8(c, MC_CUR_STATS + i), mm2_c8(c, MC_BASE_STATS + i));
		mm2_draw_text(canvas, font, 0, 3 + i, line, 15, -1);
	}
	snprintf(line, sizeof(line), "%-12s %3d/%3d", "Endurance", mm2_c8(c, MC_ENDURANCE), mm2_c8(c, MC_BASE_ENDURANCE));
	mm2_draw_text(canvas, font, 0, 9, line, 15, -1);
	snprintf(line, sizeof(line), "Level %d  Age %d  AC %d  Food %d", mm2_c8(c, MC_LEVEL), mm2_c8(c, MC_AGE), mm2_c8(c, MC_AC), mm2_c8(c, MC_FOOD));
	mm2_draw_text(canvas, font, 0, 11, line, 7, -1);
	snprintf(line, sizeof(line), "HP %u/%u   SP %u/%u", mm2_c16(c, MC_HP), mm2_c16(c, MC_HP_MAX), mm2_c16(c, MC_SP), mm2_c16(c, MC_SP_MAX));
	mm2_draw_text(canvas, font, 0, 12, line, 10, -1);
	snprintf(line, sizeof(line), "Exp %u  Gold %u  Gems %u", mm2_c32(c, MC_EXP), mm2_c32(c, MC_GOLD), mm2_c16(c, MC_GEMS));
	mm2_draw_text(canvas, font, 0, 13, line, 14, -1);
	snprintf(line, sizeof(line), "Condition: %s", condition_name(mm2_c8(c, MC_CONDITION)));
	mm2_draw_text(canvas, font, 0, 14, line, 12, -1);
	mm2_draw_text(canvas, font, 22, 3, "Equipped", 11, -1);
	for (i = 0; i < 6; i++) {
		int id = (int)mm2_c8(c, MC_EQUIP_ID + i);
		if (id) mm2_draw_text(canvas, font, 22, 4 + i, items[id].name, 15, -1);
	}
	mm2_draw_text(canvas, font, 0, 16, "Backpack", 11, -1);
	for (i = 0; i < 6; i++) {
		int id = (int)mm2_c8(c, MC_PACK_ID + i);
		if (id) mm2_draw_text(canvas, font, 0, 17 + i, items[id].name, 7, -1);
	}
}

void mm2_ui_draw_training(uint8_t *canvas, const Mm2Font *font, const Mm2Roster *roster, int town, const char *message) {
	char line[48];
	int i;
	memset(canvas, 0, MM2_SCREEN_W * MM2_SCREEN_H);
	mm2_draw_text(canvas, font, 12, 0, " Training Hall ", 14, -1);
	for (i = 0; i < mm2_party_size(roster); i++) {
		const Mm2Char *c = &roster->chars[mm2_party_member(roster, i)];
		const char *st;
		switch (mm2_train_check(c, town)) {
		case MM2_TRAIN_OK: st = "Ready"; break;
		case MM2_TRAIN_DISABLED: st = "Disabled"; break;
		case MM2_TRAIN_NEED_EXP: st = "Need XP"; break;
		default: st = "Need Au"; break;
		}
		snprintf(line, sizeof(line), "%d) %-7.7s L%-2d %7uxp %5ug %s", i + 1, (const char *)c->raw, mm2_c8(c, MC_LEVEL),
				 mm2_train_exp_needed(c), mm2_train_cost(c, town), st);
		mm2_draw_text(canvas, font, 0, 3 + i, line, strcmp(st, "Ready") == 0 ? 10 : 15, -1);
	}
	if (!mm2_party_size(roster)) mm2_draw_text(canvas, font, 0, 3, "Your party is empty.", 7, -1);
	if (message) mm2_draw_text(canvas, font, 0, 20, message, 12, -1);
	mm2_draw_text(canvas, font, 0, 23, "1-8: train that character   Esc: leave", 7, -1);
}
