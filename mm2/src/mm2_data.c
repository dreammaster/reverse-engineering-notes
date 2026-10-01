#include "mm2_data.h"

#include <string.h>

static const int DIE[4] = {1, 10, 100, 1000};
static const int PCT[8] = {0, 10, 20, 35, 50, 75, 90, 100};

int mm2_load_items(const Mm2Game *g, Mm2Item items[MM2_ITEMS]) {
	Mm2Blob b = mm2_read_file(g, "ITEMS.DAT");
	int i, k;
	if (!b.data || b.size < 5120) {
		mm2_blob_free(&b);
		return 0;
	}
	for (i = 0; i < MM2_ITEMS; i++) {
		const uint8_t *r = b.data + i * 20;
		memcpy(items[i].name, r, 12);
		items[i].name[12] = 0;
		for (k = 11; k >= 0 && items[i].name[k] == ' '; k--)
			items[i].name[k] = 0;
		items[i].classMask = r[0x0D];
		items[i].bonus = r[0x0E];
		items[i].useEffect = r[0x0F];
		items[i].value = (uint16_t)(r[0x10] | (r[0x11] << 8));
		items[i].price = (uint16_t)(r[0x12] | (r[0x13] << 8));
	}
	mm2_blob_free(&b);
	return 1;
}

Mm2ItemKind mm2_item_kind(int id) {
	if (id >= 1 && id <= 65) return MM2_ITEM_ONEHAND;
	if (id >= 66 && id <= 91) return MM2_ITEM_TWOHAND;
	if (id >= 92 && id <= 114) return MM2_ITEM_MISSILE;
	if (id >= 115 && id <= 126) return MM2_ITEM_SHIELD;
	if (id >= 127 && id <= 154) return MM2_ITEM_ARMOUR;
	if (id >= 155 && id <= 159) return MM2_ITEM_HELM;
	return MM2_ITEM_MISC;
}

static int sized(int b) {
	int v = (b & 0x1F) + 1;
	if (b & 0x20) v *= 10;
	return v > 250 ? 250 : v;
}

int mm2_load_monsters(const Mm2Game *g, Mm2Monster mons[MM2_MONSTERS]) {
	Mm2Blob b = mm2_load_lzw_file(g, "MONSTERS.DAT");
	int i, k;
	if (!b.data || b.size < 256 * 26) {
		mm2_blob_free(&b);
		return 0;
	}
	for (i = 0; i < MM2_MONSTERS; i++) {
		const uint8_t *r = b.data + i * 26;
		Mm2Monster *m = &mons[i];
		int e = r[0x0F], grp;
		memset(m, 0, sizeof(*m));
		for (k = 0; k < 14; k++)
			m->name[k] = (char)(r[k] & 0x7F);
		for (k = 13; k >= 0 && m->name[k] == ' '; k--)
			m->name[k] = 0;
		m->hp = ((r[0x0E] & 0x3F) + 1) * DIE[r[0x0E] >> 6];
		m->exp = ((e & 0x1F) + 1) * ((e & 0x80) ? 1000 : DIE[(e >> 5) & 3]);
		m->itemClass = r[0x10] & 3;
		m->dropsGems = (r[0x10] >> 2) & 1;
		m->goldClass = (r[0x10] >> 3) & 3;
		m->bribeFood = (r[0x10] >> 5) & 1;
		m->bribeGold = (r[0x10] >> 6) & 1;
		m->bribeGems = (r[0x10] >> 7) & 1;
		m->spell = r[0x11] & 0x1F;
		m->castChancePct = PCT[r[0x11] >> 5];
		m->touch = r[0x12] & 0x1F;
		m->noSteal = (r[0x12] >> 5) & 1;
		m->ranged = (r[0x12] >> 6) & 1;
		m->undead = (r[0x12] >> 7) & 1;
		grp = (r[0x13] & 15) + 1;
		m->groupSize = (r[0x13] & 0x10) ? grp * 10 : grp;
		m->verb = (r[0x13] >> 5) & 3;
		m->summoner = (r[0x13] >> 7) & 1;
		m->blows = (r[0x14] & 15) + 1;
		m->specialUses = (r[0x14] >> 4) + 1;
		m->picture = r[0x15] & 0x7F;
		m->ac = sized(r[0x16]);
		m->damageDie = sized(r[0x17]);
		m->speed = sized(r[0x18]);
		m->immune[1] = (r[0x17] >> 6) & 1;
		m->immune[2] = (r[0x17] >> 7) & 1;
		m->immune[3] = (r[0x18] >> 6) & 1;
		m->immune[4] = (r[0x18] >> 7) & 1;
		m->immune[5] = (r[0x19] >> 1) & 1;
		m->immune[6] = r[0x19] & 1;
		m->immune[7] = (r[0x19] >> 2) & 1;
		m->magicResistPct = PCT[r[0x19] >> 5];
	}
	mm2_blob_free(&b);
	return 1;
}

int mm2_load_spells(const Mm2Game *g, Mm2Spell spells[MM2_SPELLS]) {
	Mm2Blob b = mm2_read_file(g, "SPELLS.DAT");
	int i;
	if (!b.data || b.size < 192) {
		mm2_blob_free(&b);
		return 0;
	}
	for (i = 0; i < MM2_SPELLS; i++) {
		int b0 = b.data[i * 2], b1 = b.data[i * 2 + 1];
		spells[i].usage = b0 >> 6;
		spells[i].gems = (b0 & 0x3F) > 50 ? 100 : (b0 & 0x3F);
		spells[i].spCost = b1 & 15;
		spells[i].perLevel = (b1 >> 4) & 3;
		spells[i].locRestrict = (b1 >> 7) & 1;
	}
	mm2_blob_free(&b);
	return 1;
}

int mm2_load_roster(const Mm2Game *g, Mm2Roster *r) {
	Mm2Blob b = mm2_read_file(g, "ROSTER.DAT");
	if (!b.data || b.size < 0x1860 + 2052) {
		mm2_blob_free(&b);
		return 0;
	}
	memcpy(r->chars, b.data, 0x1860);
	memcpy(r->state, b.data + 0x1860, 2052);
	mm2_blob_free(&b);
	return 1;
}
