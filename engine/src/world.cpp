#include "fod/world.h"
#include <algorithm>

namespace fod {

GameData::GameData(const std::string &gameDir)
    : dir(gameDir), globals(readFile(gameDir + "/GLOBALS")), weapons(readFile(gameDir + "/WEAPONS")),
      archetypes(readFile(gameDir + "/ARCHTYPE")), hdspct(readFile(gameDir + "/HDSPCT")) {}

// ---------------------------------------------------------------- Rng
void Rng::reseed(uint32_t seed) {
	uint32_t x = seed * 2654435761u + 12345u;
	for (auto &b : _s) {
		x ^= x << 13; x ^= x >> 17; x ^= x << 5;
		b = uint8_t(x);
	}
}

uint8_t Rng::next() {
	// KEH rngByte: add-with-carry chain over bytes 14..0 seeded by byte 15, then a "loopne" increment pass.
	unsigned dl = _s[15], carry = 0;
	for (int bx = 14; bx >= 0; --bx) {
		const unsigned sum = dl + _s[bx] + carry;
		carry = sum > 255;
		dl = sum & 0xFF;
		_s[bx] = uint8_t(dl);
	}
	int cx = 16, bx = 16;
	do {
		--bx;
		++_s[bx];
		--cx;
	} while (cx != 0 && _s[bx] != 0);
	return _s[0];
}

int Rng::range(int lo, int hi) {
	if (lo >= hi) return lo;
	for (;;) {
		const int v = next();
		if (v >= lo && v <= hi) return v;
	}
}

// ---------------------------------------------------------------- World
World::World(const GameData &d, Disk1 dk, uint32_t seed) : data(d), disk(std::move(dk)), rng(seed) {}

int World::conState(const Member &m) {
	const int hp = m.hp();
	if (hp >= 1) return 0;
	if (hp <= -60) return 5;
	return std::abs(hp) / 15 + 1;
}

int World::attrBonus(int v) {
	if (v != 0 && v < 14) return v - 14;
	if (v > 15) return v - 15;
	return 0;
}

bool World::isActive(const Member &m) const {
	int hp = m.hp();
	const int toughness = m.attr(3);
	if (toughness > 15) hp += toughness - 15;
	return hp > 0;
}

int World::armorClass(const Member &m) const {
	int sum = 0;
	for (int i = 0; i < 3; ++i) {
		const uint8_t slot = m.u8(0x4B + i);
		if (slot != 0xFF) sum += item(m.b[0x62 + 6 * slot]).weaponDef;
	}
	if (sum == 0) sum = data.archetypes.professions().at(m.profession()).baseAC;
	return sum;
}

bool World::isUnique(int id) const { return (item(id).flags & 0x1C) == 0x1C; }

void World::initItemSlot(Member &m, int slot, int id) {
	m.itemId(slot) = uint8_t(id);
	m.itemAmmoType(slot) = 0;
	m.itemAmmo(slot) = 0;
	m.itemFlags(slot) = 0;
	const ItemDef &it = item(id);
	if (it.isWeapon()) {
		const auto &w = data.weapons.weapons().at(it.weaponDef);
		if ((w.flags() & 1) && !w.modes.empty()) m.itemAmmo(slot) = w.modes[0].clip;
	}
}

void World::removeItemSlot(Member &m, int slot, int replaceWith) {
	if (m.readied() != 0xFF) {
		if (m.readied() == slot) m.readied() = 0xFF;
		else if (m.readied() > slot && replaceWith == 0xFF) --m.readied();
	}
	for (int i = 0; i < 3; ++i) {
		uint8_t &a = m.u8(0x4B + i);
		if (a == 0xFF) continue;
		if (a == slot) a = 0xFF;
		else if (a > slot && replaceWith == 0xFF) --a;
	}
	if (replaceWith == 0xFF) {
		--m.itemCount();
		for (int i = slot; i < m.itemCount(); ++i)
			std::memcpy(&m.b[0x62 + 6 * i], &m.b[0x68 + 6 * i], 6);
	} else {
		m.itemId(slot) = uint8_t(replaceWith);
		m.itemAmmo(slot) = 0;
		m.itemFlags(slot) = 0;
	}
}

bool World::addItem(Member &m, int id) {
	if (m.itemCount() >= 32) return false;
	initItemSlot(m, m.itemCount(), id);
	++m.itemCount();
	return true;
}

bool World::hasItem(const Member &m, int id) const {
	for (int i = 0; i < m.b[0x52]; ++i)
		if (m.b[0x62 + 6 * i] == id) return true;
	return false;
}

static int32_t clampI(int64_t v, int64_t lo, int64_t hi) { return int32_t(std::min(std::max(v, lo), hi)); }

int32_t World::getStat(int sel, int kind, int idx, int cur) {
	const int slot = resolveMember(sel, cur);
	if (slot < 0 || slot >= partyCount()) return 0;
	Member &m = party(slot);
	const int con = conState(m);
	switch (kind) {
	case 0: return con == 0 ? m.u8(0x4E) : 0;
	case 1: return con == 0 ? int32_t(m.exp()) : 0;
	case 2: return con < 5 ? m.hp() : 0;
	case 3: return con < 5 ? m.hpMax() : 0;
	case 4: return con == 0 ? m.attr(idx) : 0;
	case 5: return con == 0 ? m.activeSkill(idx) : 0;
	case 6: return con == 0 ? m.passiveSkill(idx) : 0;
	case 7: return int32_t(disk.cash);
	case 8: return m.u8(0x50) & 3;
	case 9: return m.profession();
	}
	return 0;
}

void World::setStat(int sel, int kind, int idx, int32_t v, int cur) {
	if (kind == 7) {
		disk.cash = uint32_t(clampI(v, 0, 999999));
		return;
	}
	const int slot = resolveMember(sel, cur);
	if (slot < 0 || slot >= partyCount()) return;
	Member &m = party(slot);
	const int con = conState(m);
	switch (kind) {
	case 0: if (con == 0) m.u8(0x4E) = uint8_t(clampI(v, 1, 99)); break;
	case 1: if (con == 0) m.setU32(0x5A, m.exp() + uint32_t(v)); break;   // level-ups are applied by the caller
	case 2:
		if (con != 5) {
			m.setHp(clampI(v, -60, m.hpMax()));
			if (m.hp() <= -60) disk.tail[m.rosterId()] |= 2;
		}
		break;
	case 3: if (con != 5) m.setHpMax(v); break;
	case 4: if (con == 0) m.attr(idx) = uint8_t(clampI(v, 1, 35)); break;
	case 5: if (con == 0) m.activeSkill(idx) = uint8_t(clampI(v, 1, 35)); break;
	case 6: if (con == 0) m.passiveSkill(idx) = uint8_t(clampI(v, 1, 35)); break;
	}
}

void World::advanceMinutes(int minutes) {
	int total = disk.minutes + minutes;
	disk.minutes = uint8_t(total % 60);
	const int hours = disk.hours + total / 60;
	const bool dayRolled = hours >= 24;
	disk.hours = uint8_t(hours % 24);
	if (dayRolled) disk.weekday = uint8_t((disk.weekday + 1) % 7);
}

// ---------------------------------------------------------------- maps
void World::loadMap(int number) {
	static const char *kinds[2] = {"map", "scr"};
	Archive maps(data.dir + "/" + std::string(mapFilePrefix(number)) + kinds[0], 28);
	Archive scr(data.dir + "/" + std::string(mapFilePrefix(number)) + kinds[1], 28);
	mapNo = number;
	map = std::make_unique<Map>(maps.chunk(size_t(number - 1)));
	scripts = std::make_unique<ScriptBank>(scr.chunkLzss(size_t(number - 1)));
	loadChangesFromDisk();
	for (const MapChange &c : changes)
		if (c.map == number) applyChange(c);
}

void World::setPartyPosition(int x, int y) {
	disk.mapX = int16_t(x);
	disk.mapY = int16_t(y);
}

void World::applyChange(const MapChange &c) {
	if (!map || !map->inBounds(c.x, c.y)) return;
	MapCell &cell = map->cell(c.x, c.y);
	if (c.kind & 0x80) cell.tile = uint16_t((cell.tile & 0xF800) | (c.tile & 0x7FF));
	if (c.kind & 0x40) {
		cell.b3 = c.b3;
		cell.w4 = c.w4;
		cell.flags = uint8_t(c.f2 | (cell.flags & 0x80));   // KEH applyMapChangeList
	}
	if (c.kind & 0x20) cell.script = c.script;
}

void World::applyCellChange(int x, int y, int op, int val, int extra) {
	if (!map || !map->inBounds(x, y)) return;
	MapCell &cell = map->cell(x, y);
	switch (op) {
	case 0x19: case 0x1C: cell.tile = uint16_t((cell.tile & 0xF800) | (val & 0x7FF)); break;
	case 0x1A: case 0x1D:
		// KEH setCellFields: b3 = val & 0x7F, flags bit0 = val bit7, w4 = extra & 0x7FFF, flags bit1 = extra bit15
		cell.b3 = uint8_t(val & 0x7F);
		cell.flags = uint8_t((cell.flags & ~1) | ((val >> 7) & 1));
		cell.w4 = uint16_t(extra & 0x7FFF);
		cell.flags = uint8_t((cell.flags & ~2) | ((extra >> 14) & 2));
		break;
	case 0x1B: case 0x1E: cell.script = uint16_t(val); break;
	}
}

void World::recordCellChange(int op, int mapNumber, int x, int y, int val, int extra) {
	if (mapNumber == mapNo) {
		applyCellChange(x, y, op, val, extra);
		return;
	}
	MapChange *c = nullptr;
	for (auto &e : changes)
		if (e.map == mapNumber && e.x == x && e.y == y) c = &e;
	if (!c) {
		if (changes.size() >= 150) return;   // "Flushing Needed!!"
		changes.push_back(MapChange{});
		c = &changes.back();
		c->map = uint8_t(mapNumber); c->x = uint8_t(x); c->y = uint8_t(y);
	}
	switch (op) {
	case 0x1C: c->kind |= 0x80; c->tile = uint16_t(val & 0x7FF); break;
	case 0x1D:
		c->kind |= 0x40;
		c->b3 = uint8_t(val & 0x7F);
		c->f2 = uint8_t(((val >> 7) & 1) | ((extra >> 14) & 2));
		c->w4 = uint16_t(extra & 0x7FFF);
		break;
	case 0x1E: c->kind |= 0x20; c->script = uint16_t(val); break;
	}
}

// Disk1.tail layout: +0x000 = roster flags (250 bytes), +0x100 = u16 count, then records (12-byte stride, fields at +2..+0xD).
void World::loadChangesFromDisk() {
	changes.clear();
	if (disk.tail.size() < 0x108) return;
	const unsigned count = disk.tail[0x100] | (disk.tail[0x101] << 8);
	for (unsigned i = 0; i < count && i < 150; ++i) {
		const size_t o = 0x100 + 12 * i;
		if (o + 14 > disk.tail.size()) break;
		MapChange c;
		c.map = disk.tail[o + 2]; c.x = disk.tail[o + 3]; c.y = disk.tail[o + 4]; c.kind = disk.tail[o + 5];
		c.tile = uint16_t(disk.tail[o + 6] | (disk.tail[o + 7] << 8));
		c.f2 = disk.tail[o + 8]; c.b3 = disk.tail[o + 9];
		c.w4 = uint16_t(disk.tail[o + 10] | (disk.tail[o + 11] << 8));
		c.script = uint16_t(disk.tail[o + 12] | (disk.tail[o + 13] << 8));
		changes.push_back(c);
	}
}

void World::syncChangesToDisk() {
	disk.tail[0x100] = uint8_t(changes.size());
	disk.tail[0x101] = uint8_t(changes.size() >> 8);
	for (size_t i = 0; i < changes.size(); ++i) {
		const size_t o = 0x100 + 12 * i;
		const MapChange &c = changes[i];
		disk.tail[o + 2] = c.map; disk.tail[o + 3] = c.x; disk.tail[o + 4] = c.y; disk.tail[o + 5] = c.kind;
		disk.tail[o + 6] = uint8_t(c.tile); disk.tail[o + 7] = uint8_t(c.tile >> 8);
		disk.tail[o + 8] = c.f2; disk.tail[o + 9] = c.b3;
		disk.tail[o + 10] = uint8_t(c.w4); disk.tail[o + 11] = uint8_t(c.w4 >> 8);
		disk.tail[o + 12] = uint8_t(c.script); disk.tail[o + 13] = uint8_t(c.script >> 8);
	}
}

} // namespace fod
