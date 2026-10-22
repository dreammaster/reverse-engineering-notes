#include "fod/map.h"
#include <algorithm>

namespace fod {

std::string Monster::name() const {
	return std::string(reinterpret_cast<const char *>(raw.data()), strnlen(reinterpret_cast<const char *>(raw.data()), 24));
}
int16_t Monster::hp() const { return int16_t(raw[0x44] | (raw[0x45] << 8)); }

Map::Map(const Bytes &chunk) {
	Reader r(chunk);
	_header.assign(chunk.begin(), chunk.begin() + std::min<size_t>(chunk.size(), kHeaderSize));
	_header.resize(kHeaderSize);
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

Bytes Map::serialize() const {
	Bytes out = _header;
	auto put16 = [&](size_t o, unsigned v) { out[o] = uint8_t(v); out[o + 1] = uint8_t(v >> 8); };
	out[9] = uint8_t(_monsters.size());
	out[0xB] = uint8_t(_groups.size());
	put16(4, _flags);
	put16(0x1E, unsigned(_stored.size()));
	for (size_t i = 0; i < 3; ++i) {
		const size_t o = 0x20 + 0x14C * i;
		if (i < _stored.size()) std::copy(_stored[i].begin(), _stored[i].end(), out.begin() + long(o));
		else std::fill(out.begin() + long(o), out.begin() + long(o + 0x14C), uint8_t(0));
	}
	for (const MapCell &c : _cells) {
		const size_t o = out.size();
		out.resize(o + 8);
		out[o] = uint8_t(c.tile); out[o + 1] = uint8_t(c.tile >> 8);
		out[o + 2] = c.flags; out[o + 3] = c.b3;
		out[o + 4] = uint8_t(c.w4); out[o + 5] = uint8_t(c.w4 >> 8);
		out[o + 6] = uint8_t(c.script); out[o + 7] = uint8_t(c.script >> 8);
	}
	const size_t shopsOfs = out.size();
	for (const Bytes &b : _shops) out.insert(out.end(), b.begin(), b.end());
	const size_t monstersOfs = out.size();
	for (const Monster &m : _monsters) out.insert(out.end(), m.raw.begin(), m.raw.end());
	const size_t monstersEnd = out.size();
	for (const EnemyGroup &g : _groups) out.insert(out.end(), g.raw.begin(), g.raw.end());
	const size_t itemsOfs = out.size();
	for (const GroundItem &g : _items) {
		out.push_back(uint8_t(g.qty)); out.push_back(uint8_t(g.qty >> 8));
		out.push_back(g.x); out.push_back(g.y); out.push_back(g.item); out.push_back(0);
	}
	const size_t groupsOfs = monstersEnd;
	put16(0x404, unsigned(shopsOfs)); put16(0x406, unsigned(monstersOfs)); put16(0x408, unsigned(monstersEnd));
	put16(0x40A, unsigned(groupsOfs)); put16(0x40C, unsigned(itemsOfs));
	put16(0x40E, unsigned(_items.size())); put16(0x410, unsigned(_items.size()));
	return out;
}

} // namespace fod
