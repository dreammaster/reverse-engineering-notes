#include "mapbin.h"

#include <string.h>

int mm3_mapbin_load(Mm3MapBin *m, const uint8_t *d, size_t len, unsigned map, const Mm3Dgroup *dg, const Mm3Party *party) {
	size_t pos = 0;
	unsigned arg = map - 1; /* Map_load's argument */
	unsigned pic_row = (arg == 0x68) ? 0x40 : arg;
	memset(m, 0, sizeof(*m));

	while (pos < len && d[pos] != 0xFF) {
		uint8_t x, y, b, id;
		if (pos + 3 > len)
			return -1;
		x = d[pos]; y = d[pos + 1]; b = d[pos + 2];
		pos += 3;
		if ((b >> 2) != 0)
			id = (uint8_t)((b >> 2) + 0x28);
		else
			id = mm3_dg_u8(dg, MM3_DG_MAP_MONSTER_PICS + pic_row * 3 + (b & 3));
		if (m->monster_count < MM3_MAX_MONSTERS) {
			Mm3MapMonster *mon = &m->monsters[m->monster_count];
			mon->x = x; mon->y = y; mon->id = id; mon->pic_sel = b & 3;
			mon->anim_range = mm3_dg_u8(dg, MM3_DG_MON_ANIM_FRAMES + id);
			m->monster_count++;
		} else {
			m->monsters_overflow = 1;
		}
	}
	pos++; /* the FFh terminator */
	if (pos + 5 > len)
		return -1;
	memcpy(m->pic_slots, d + pos, 5);
	pos += 5;
	for (int i = 0; i < 5; i++) {
		if (m->pic_slots[i] != MM3_PIC_UNUSED)
			continue;
		/* quest-dependent replacement of an unused slot by picture 3Eh */
		if (party && ((arg == 0x29 && mm3_party_flag(party, 0x41)) || (arg == 0x2F && mm3_party_flag(party, 0x4E)) ||
		              (arg == 0x38 && mm3_party_flag(party, 0x5B))))
			m->pic_slots[i] = 0x3E;
	}
	while (pos + 3 <= len) {
		uint8_t x = d[pos], y = d[pos + 1], slot = d[pos + 2];
		pos += 3;
		if (m->object_count >= MM3_MAX_OBJECTS) {
			m->objects_overflow = 1;
			continue;
		}
		if (slot >= 5)
			return -1;
		Mm3MapObject *o = &m->objects[m->object_count++];
		o->x = x; o->y = y; o->slot = slot;
		o->pic = m->pic_slots[slot];
		o->anim_range = mm3_dg_u8(dg, MM3_DG_OBJECT_ANIM_FRAMES + o->pic);
	}
	return 0;
}
