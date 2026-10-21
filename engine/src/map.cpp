#include "fod/map.h"

namespace fod {

std::string Monster::name() const {
	return std::string(reinterpret_cast<const char *>(raw.data()), strnlen(reinterpret_cast<const char *>(raw.data()), 24));
}
int16_t Monster::hp() const { return int16_t(raw[0x44] | (raw[0x45] << 8)); }

Map::Map(const Bytes &chunk) {
	Reader r(chunk);
	_w = r.u16();
	_h = r.u16();
	_flags = r.u16();
	_minutesPerStep = r.u8();
	_encounterPct = r.u8();
	r.seek(0x09);
	const unsigned monsterCount = r.u8();
	r.seek(0x0B);
	const unsigned groupCount = r.u8();
	r.seek(0x404);
	r.u16(); // shops offset
	const unsigned monstersOfs = r.u16();
	r.u16(); // monsters end
	const unsigned groupsOfs = r.u16();
	const unsigned itemsOfs = r.u16();
	const unsigned itemCount = r.u16();

	r.seek(kHeaderSize);
	_cells.resize(size_t(_w) * _h);
	for (auto &c : _cells) {
		c.tile = r.u16();
		c.flags = r.u8();
		c.b3 = r.u8();
		c.w4 = r.u16();
		c.script = r.u16();
	}
	for (unsigned i = 0; i < monsterCount; ++i) {
		Monster m;
		m.raw.resize(0x68);
		r.seek(monstersOfs + i * 0x68);
		r.bytes(m.raw.data(), 0x68);
		_monsters.push_back(std::move(m));
	}
	for (unsigned i = 0; i < groupCount; ++i) {
		EnemyGroup g;
		g.raw.resize(0x66);
		r.seek(groupsOfs + i * 0x66);
		r.bytes(g.raw.data(), 0x66);
		_groups.push_back(std::move(g));
	}
	for (unsigned i = 0; i < itemCount; ++i) {
		r.seek(itemsOfs + i * 6);
		GroundItem it;
		it.qty = r.u16();
		it.x = r.u8();
		it.y = r.u8();
		it.item = r.u8();
		_items.push_back(it);
	}
}

} // namespace fod
