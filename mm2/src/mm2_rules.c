#include "mm2_tables.h"

uint32_t mm2_exp_for_level(int cls, int level) {
	int group = (cls == MM2_PALADIN || cls == MM2_ARCHER || cls == MM2_SORCERER || cls == MM2_NINJA) ? 1 : 0;
	int idx = level > 10 ? 10 : level;
	uint32_t e = MM2_EXP_TABLE[group * 9 + (idx - 2)];
	if (level >= 11) e += 192000;
	if (level >= 12) e += 192000;
	if (level >= 13) e += 192000;
	if (level >= 14) e += 384000;
	if (level >= 15) e += 384000;
	if (level >= 16) e += (uint32_t)(level - 15 > 5 ? 5 : level - 15) * 768000u;
	if (level >= 21) e += (uint32_t)(level - 20 > 10 ? 10 : level - 20) * 1536000u;
	if (level >= 31) e += (uint32_t)(level - 30 > 20 ? 20 : level - 30) * 3072000u;
	if (level >= 51) e += (uint32_t)(level - 50 > 25 ? 25 : level - 50) * 0x190000u;
	if (level >= 76) e += (uint32_t)(level - 75) * 6144000u;
	return e;
}

uint32_t mm2_training_cost(int town, int level) {
	return (uint32_t)MM2_TOWN_MULT[town] * (uint32_t)level * 50u;
}

int mm2_bracket(int stat) {
	int n = -3, i = 0;
	while (MM2_STAT_BRACKET[i] < stat) {
		n++;
		i++;
	}
	return n;
}
