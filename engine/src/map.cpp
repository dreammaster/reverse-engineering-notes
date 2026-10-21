#include "fod/map.h"
#include <algorithm>

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
	_groupTemplates = chunk[8];
	_offMapMsg = chunk[0x0D];
	r.seek(0x09);
	const unsigned monsterCount = r.u8();
	r.seek(0x0B);
	const unsigned groupCount = r.u8();
	r.seek(0x404);
	const unsigned shopsOfs = r.u16();
	const unsigned monstersOfs = r.u16();
	r.u16(); // monsters end
	const unsigned groupsOfs = r.u16();
	const unsigned itemsOfs = r.u16();
	const unsigned itemCount = r.u16();

	for (size_t pos = shopsOfs; pos + 0x2C <= monstersOfs && pos + 0x2C <= chunk.size();) {
		const size_t count = chunk[pos + 0x2A] | (chunk[pos + 0x2B] << 8);
		const size_t size = 0x2C + 0x10 * count;
		if (pos + size > chunk.size()) break;
		_shops.emplace_back(chunk.begin() + long(pos), chunk.begin() + long(pos + size));
		pos += size;
	}

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
	const unsigned stored = std::min<unsigned>(chunk[0x1E] | (chunk[0x1F] << 8), 3);
	for (unsigned i = 0; i < stored; ++i) {
		const size_t o = 0x20 + 0x14C * size_t(i);
		if (o + 0x14C > chunk.size()) break;
		_stored.emplace_back(chunk.begin() + long(o), chunk.begin() + long(o) + 0x14C);
	}
}

} // namespace fod
