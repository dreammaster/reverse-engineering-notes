#pragma once
// Game state: the Disk1 save struct (0xEC0 bytes) and the party-member record (0x14C bytes).
#include "fod/types.h"
#include <array>

namespace fod {

// One party member (and, for the first 0x54 bytes, the layout shared with monsters). Offsets verified in KEH.EXE.
struct Member {
	static constexpr size_t kSize = 0x14C;
	std::array<uint8_t, kSize> b{};

	uint8_t &u8(size_t o) { return b[o]; }
	uint8_t u8(size_t o) const { return b[o]; }
	uint16_t u16(size_t o) const { return uint16_t(b[o] | (b[o + 1] << 8)); }
	void setU16(size_t o, uint16_t v) { b[o] = uint8_t(v); b[o + 1] = uint8_t(v >> 8); }
	uint32_t u32(size_t o) const { return u16(o) | (uint32_t(u16(o + 2)) << 16); }
	void setU32(size_t o, uint32_t v) { setU16(o, uint16_t(v)); setU16(o + 2, uint16_t(v >> 16)); }

	std::string name() const { return std::string(reinterpret_cast<const char *>(b.data()), strnlen(reinterpret_cast<const char *>(b.data()), 24)); }
	uint8_t &attr(int i) { return b[0x18 + i]; }                 // 7 attributes
	uint8_t attr(int i) const { return b[0x18 + i]; }
	uint8_t &activeSkill(int i) { return b[0x24 + i]; }          // 16
	uint8_t &passiveSkill(int i) { return b[0x34 + i]; }         // 16
	int16_t hp() const { return int16_t(u16(0x44)); }
	void setHp(int v) { setU16(0x44, uint16_t(v)); }
	int16_t hpMax() const { return int16_t(u16(0x46)); }
	void setHpMax(int v) { setU16(0x46, uint16_t(v)); }
	uint8_t profession() const { return b[0x48]; }
	uint8_t rosterId() const { return b[0x49]; }
	uint8_t &readied() { return b[0x4A]; }                       // slot index or 0xFF
	uint8_t &rank() { return b[0x4E]; }
	uint8_t &flags() { return b[0x50]; }                         // bits0-1: sex
	uint8_t &itemCount() { return b[0x52]; }
	uint8_t &status() { return b[0x58]; }                        // affliction bits
	uint32_t exp() const { return u32(0x5A); }
	uint32_t expNext() const { return u32(0x5E); }
	// inventory: 32 slots of 6 bytes {item, ammoType, ammoCount, flags, -, -}
	uint8_t &itemId(int s) { return b[0x62 + 6 * s]; }
	uint8_t &itemAmmoType(int s) { return b[0x63 + 6 * s]; }
	uint8_t &itemAmmo(int s) { return b[0x64 + 6 * s]; }
	uint8_t &itemFlags(int s) { return b[0x65 + 6 * s]; }
	uint8_t &command() { return b[0x144]; }
	uint8_t &cmdArg() { return b[0x145]; }
	uint8_t &cmdArg2() { return b[0x146]; }
	uint8_t &cmdArg3() { return b[0x147]; }
};

struct Disk1 {
	static constexpr size_t kSize = 0xEC0;
	uint32_t saveCounter = 0;
	uint16_t regionAndSpeed = 0;
	uint8_t gfxMode = 0;
	uint8_t uniqueItems = 0;
	uint8_t hours = 9, minutes = 0, weekday = 1;
	int16_t mapX = 10, mapY = 6;
	uint32_t cash = 100;
	uint8_t maps[10]{}, mapsX[10]{}, mapsY[10]{};
	uint8_t mapIndex = 0;
	uint8_t partyCount = 0;
	uint8_t partyIds[5]{};
	uint16_t facing = 0;
	std::array<Member, 5> roster{};
	Bytes tail;                   // +0x6B6 .. 0xEC0 (persistent map-change records etc.)

	static Disk1 parse(const Bytes &file);
	Bytes serialize() const;
	static Disk1 newGame();       // what FOD.EXE writes for a fresh game (docs/fod-menu.md)
};

} // namespace fod
