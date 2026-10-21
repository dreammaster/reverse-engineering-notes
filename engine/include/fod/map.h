#pragma once
#include "fod/bytes.h"

namespace fod {

struct MapCell {
	uint16_t tile;        // low 11 bits: tile number; high 5 bits: overlay entity id
	uint8_t flags;        // bit0 passable, bit6 cursor, bit7 item/seen
	uint8_t b3;
	uint16_t w4;
	uint16_t script;      // 0xFFFF = none
	int tileIndex() const { return tile & 0x7FF; }
	int overlay() const { return tile >> 11; }
	bool passable() const { return flags & 1; }
};

struct Monster {          // 0x68 bytes (shares its first 0x54 bytes with a party member)
	Bytes raw;
	std::string name() const;
	int16_t hp() const;
	int8_t x() const { return int8_t(raw[0x5B]); }
	int8_t y() const { return int8_t(raw[0x5C]); }
	uint8_t id() const { return raw[0x49]; }
	uint16_t script() const { return uint16_t(raw[0x66] | (raw[0x67] << 8)); }
};

struct EnemyGroup {       // 0x66 bytes
	Bytes raw;
	int x() const { return int8_t(raw[2]); }
	int y() const { return int8_t(raw[3]); }
	int slotCount() const { return raw[8]; }
};

struct GroundItem {       // 6 bytes
	uint16_t qty;
	uint8_t x, y;         // y bit7 = hidden
	uint8_t item;
};

class Map {
public:
	static constexpr size_t kHeaderSize = 0x414;
	explicit Map(const Bytes &chunk);
	int width() const { return _w; }
	int height() const { return _h; }
	uint16_t flags() const { return _flags; }
	uint8_t minutesPerStep() const { return _minutesPerStep; }
	uint8_t encounterPct() const { return _encounterPct; }
	const MapCell &cell(int x, int y) const { return _cells[size_t(y) * _w + x]; }
	std::vector<MapCell> &cells() { return _cells; }
	const std::vector<Monster> &monsters() const { return _monsters; }
	const std::vector<EnemyGroup> &groups() const { return _groups; }
	const std::vector<GroundItem> &groundItems() const { return _items; }
	std::vector<Monster> &monsters() { return _monsters; }
	std::vector<EnemyGroup> &groups() { return _groups; }
	std::vector<GroundItem> &groundItems() { return _items; }
	MapCell &cell(int x, int y) { return _cells[size_t(y) * _w + x]; }
	bool inBounds(int x, int y) const { return x >= 0 && y >= 0 && x < _w && y < _h; }
	uint16_t &flagsRef() { return _flags; }
	uint8_t offMapMsg() const { return _offMapMsg; }
	uint8_t groupTemplates() const { return _groupTemplates; }
private:
	int _w = 0, _h = 0;
	uint16_t _flags = 0;
	uint8_t _minutesPerStep = 0, _encounterPct = 0, _groupTemplates = 0, _offMapMsg = 0;
	std::vector<MapCell> _cells;
	std::vector<Monster> _monsters;
	std::vector<EnemyGroup> _groups;
	std::vector<GroundItem> _items;
};

} // namespace fod
