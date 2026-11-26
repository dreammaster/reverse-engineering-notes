#include "describe.h"
#include <stdio.h>
#include <string.h>

int describe_load_tables(const uint8_t *mp1, size_t len, uint16_t spell_str[100], uint16_t potion_str[100]) {
	static const uint8_t pat[8] = {0x00, 0x54, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};   /* entry 0 = string 84 (MAGIC MISSILE) */
	for (size_t o = 0; o + 0x640 <= len; o++)
		if (!memcmp(mp1 + o, pat, 8)) {
			for (int k = 0; k < 100; k++) {
				spell_str[k] = (uint16_t)((mp1[o + 8 * k] << 8) | mp1[o + 8 * k + 1]);
				potion_str[k] = (uint16_t)((mp1[o + 0x320 + 8 * k] << 8) | mp1[o + 0x320 + 8 * k + 1]);
			}
			return 1;
		}
	return 0;
}

static const char *str(const LngTable *l, int id) { return id >= 0 && id < l->count ? l->strings[id] : ""; }

int item_description(const LngTable *lng, const uint16_t spell_str[100], const uint16_t potion_str[100], int cls, const uint8_t *a,
                     const uint8_t *ammo_a, int ammo_name, char *out, size_t cap) {
	size_t l = 0;
#define ADD(...) do { if (l < cap) l += (size_t)snprintf(out + l, cap - l, __VA_ARGS__); } while (0)
	if (cls == 1 || cls == 2) {
		int nid = a[0] < 100 ? (cls == 1 ? spell_str[a[0]] : potion_str[a[0]]) : -1;
		ADD("%s", str(lng, 5));
		ADD("%s", (a[3] & 1) ? str(lng, 39) : str(lng, nid));                       /* string 39 = UNKNOWN */
		ADD("%s%d%s%d", str(lng, cls == 1 ? 9 : 11), a[2], str(lng, cls == 1 ? 10 : 12), a[1]);
		if (a[3] & 4) ADD("  %s", str(lng, 34));                                 /* a bad potion */
		return 1;
	}
	if (cls == 4 || (cls & 0x80)) {
		const uint8_t *w = a;
		if (cls == 4) {
			if (a[2] != 0) {                                                    /* needs ammunition in the rucksack: strings 19 + its name + 20 */
				ADD("%s%s%s", str(lng, 19), ammo_name < 0x8000 ? str(lng, ammo_name) : "", str(lng, 20));
				return 1;
			}
			ADD("%s ", str(lng, 21));
			if (ammo_a) w = ammo_a;                                            /* the shots are those of the ammunition thing */
		}
		ADD("%s", str(lng, 16));
		if (w[2] == 0xFF) ADD("%s", str(lng, 18)); else ADD("%d", w[2]);
		ADD("%s%d", str(lng, 17), w[1]);
		return 1;
	}
#undef ADD
	return 0;
}
