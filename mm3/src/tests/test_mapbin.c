/* usage: test_mapbin MM3.CUR DGROUP.BIN -- parse every MAZEnn.BIN and sanity-check the monsters and objects. */
#include <stdio.h>
#include <stdlib.h>

#include "../cc.h"
#include "../mapbin.h"

int main(int argc, char **argv) {
	Mm3Cc cc;
	Mm3Dgroup dg;
	Mm3Party party;
	size_t len;
	uint8_t *d;
	unsigned maps = 0, monsters = 0, objects = 0, bad = 0, max_id = 0, max_pic = 0;
	if (argc < 3 || mm3_cc_open(&cc, argv[1]) || mm3_dgroup_load(&dg, argv[2])) return 2;
	d = mm3_cc_read(&cc, "MAZE.PTY", &len);
	if (!d || mm3_party_load(&party, d, len)) return 2;
	free(d);
	for (unsigned map = 1; map < 100; map++) {
		char name[32];
		Mm3MapBin m;
		snprintf(name, sizeof name, "MAZE%02u.BIN", map);
		if (!(d = mm3_cc_read(&cc, name, &len))) continue;
		if (mm3_mapbin_load(&m, d, len, map, &dg, &party) || m.monsters_overflow || m.objects_overflow) {
			printf("%s: parse problem (len %zu, monsters %u, objects %u)\n", name, len, m.monster_count, m.object_count);
			bad++;
		}
		for (unsigned i = 0; i < m.monster_count; i++) {
			if (m.monsters[i].id >= 90) { printf("%s: monster id %u out of range\n", name, m.monsters[i].id); bad++; }
			if (m.monsters[i].id > max_id) max_id = m.monsters[i].id;
			if (m.monsters[i].x > 31 && m.monsters[i].x != 0xFF) { /* world is 32x32 */ }
		}
		for (unsigned i = 0; i < m.object_count; i++)
			if (m.objects[i].pic > max_pic) max_pic = m.objects[i].pic;
		if (map <= 3 || map == 42)
			printf("%s: %u monsters, %u objects, slots %02X %02X %02X %02X %02X\n", name, m.monster_count, m.object_count,
				m.pic_slots[0], m.pic_slots[1], m.pic_slots[2], m.pic_slots[3], m.pic_slots[4]);
		maps++; monsters += m.monster_count; objects += m.object_count;
		free(d);
	}
	printf("%u maps, %u monsters (max id %u), %u objects (max picture %u), %u problems\n", maps, monsters, max_id, objects, max_pic, bad);
	return bad != 0;
}
