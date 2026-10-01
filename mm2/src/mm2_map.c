#include "mm2_map.h"

int mm2_map_style(int map) {
	if (map <= 4) return 0;
	if (map <= 16) return 3;
	if (map <= 32) return 1;
	if (map <= 40) return 6;
	if (map <= 44) return 4;
	if (map <= 54) return 5;
	return 2;
}

int mm2_style_is_outdoor(int style) {
	return style == 3 || style == 4 || style == 6;
}

const char *mm2_style_name(int style) {
	switch (style) {
	case 0: return "TOWN";
	case 1: return "CAVE";
	default: return "CASTLE";
	}
}

const char *mm2_special_bank(int a) {
	switch (a & 15) {
	case 9: return "DESERT";
	case 10: return "OCEAN";
	case 11: return "SWAMP";
	case 12: return "TUNDRA";
	default: return "OCEAN";
	}
}

int mm2_side_block_bit(char side) {
	switch (side) {
	case 'N': return 0x40;
	case 'E': return 0x10;
	case 'S': return 0x04;
	default: return 0x01;
	}
}

int mm2_can_step(const uint8_t *map, int x, int y, char facing) {
	uint8_t flags = map[256 + (((y & 15) << 4) | (x & 15))];
	return !(flags & mm2_side_block_bit(facing));
}
