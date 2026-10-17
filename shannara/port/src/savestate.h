// The complete mutable game state and its savegame/undo file format (analysis sections 6 and 23).
//   [0x20 header: save title] [object record prefixes: 33/41/48/24/49 bytes for types 1-5, objects 1..744]
//   [92 registered regions, 9,512 bytes] [4 bytes at 0xADDEC] [0x30 bytes at 0xCCBB8]   = 32,668 bytes
// Older builds (the shipped HELP.DAT) omit the two regions 89 and 90 (-8 bytes); load() accepts both sizes and keeps the exe's
// initial value for what is missing. Checked on the real HELP.DAT: all 744 records and the region layout line up.
#pragma once
#include "retail_data.h"

namespace shannara {

class SaveState {
public:
	static const size_t kHeaderSize = 0x20;
	static const int kRegionCount = 92;
	static const uint32_t kFullSize = 32668, kLegacySize = 32660;

	// Power-on state: object records and region contents as stored in the executable (BSS regions are zero).
	static SaveState initial(const LeExe &exe);

	void load(const Bytes &file); // throws FormatError unless the size is kFullSize or kLegacySize
	Bytes save(bool legacy = false) const;

	uint8_t header[kHeaderSize] = {};
	std::vector<ObjectRecord> objects;
	std::vector<Bytes> regions; // kRegionCount, sizes as in save_regions.inc
	Bytes tailA, tailB;         // 4 bytes at 0xADDEC and 0x30 bytes at 0xCCBB8

	static size_t objectPrefix(int type) { static const size_t p[6] = {0, 33, 41, 48, 24, 49}; return p[type]; }
	static uint32_t regionAddress(int i);
	static uint32_t regionSize(int i);
	// pointer to the byte at linear address `addr` inside a region (nullptr if it is not saved)
	uint8_t *at(uint32_t addr);
	const uint8_t *at(uint32_t addr) const { return const_cast<SaveState *>(this)->at(addr); }

	// convenience views of well-known state (addresses from the IDB)
	int currentRoom() const { const uint8_t *p = at(0xADE96); return p ? p[0] | (p[1] << 8) : 0; } // high word of _current_room
	bool gameFlag(int n) const; // 1-based bit in the 800-bit _game_flags (0xCD8AC)
	uint32_t score() const { const uint8_t *p = at(0xADEAC); return p ? p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24) : 0; }
};

} // namespace shannara
