#include "fod/world.h"
#include <algorithm>
#include <map>
#include <cstdlib>

namespace fod {

GameData::GameData(const std::string &gameDir)
    : dir(gameDir), globals(readFile(gameDir + "/GLOBALS")), weapons(readFile(gameDir + "/WEAPONS")),
      archetypes(readFile(gameDir + "/ARCHTYPE")), packets(readFile(gameDir + "/PACKETS")), hdspct(readFile(gameDir + "/HDSPCT")) {}

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

void World::damageMember(Member &m, int damage) {
	if (damage <= 0) return;
	const int hp = m.hp();
	if (hp > 0 && hp - damage < 1) {
		const int maxHp = m.hpMax() != 0 ? m.hpMax() : 1;
		m.setU16(0x122, uint16_t(100 * hp / maxHp > 50 ? std::abs(maxHp) / 4 + 1 : hp));
	}
	if (!(disk.tail[m.rosterId()] & 2)) m.setHp(m.hp() - damage);
	if (m.hp() <= -60) disk.tail[m.rosterId()] |= 2;
}

bool World::partyWiped() const {
	for (int i = 0; i < partyCount(); ++i)
		if (party(i).hp() > -15) return false;
	return true;
}

bool World::passTime(bool combat) {
	const int healInterval = combat ? 5 : 10;
	const int step = combat ? 1 : (map ? map->minutesPerStep() : 1);
	for (int guard = 0; guard < 100000; ++guard) {
		++timeTicks;
		advanceMinutes(step);
		int alive = 0;
		for (int i = 0; i < partyCount(); ++i) {
			Member &m = party(i);
			if (m.status() & 0x7F) {
				for (int bit = 0; bit < 7; ++bit) {
					if (!(m.status() & (1 << bit))) continue;
					const size_t row = 0x16 * size_t(bit);
					const int period = data.hdspct.at(0x3EC + row) * step;
					if (period > 0 && timeTicks % period == 0) damageMember(m, data.hdspct.at(0x3ED + row));
				}
			}
			if (timeTicks % healInterval == 0) {
				int bound = (m.u8(0x54) != 0 && m.u8(0x21) == 0) ? 0 : -15;
				int con = m.hp();
				if (combat) {
					if (m.attr(3) > 15) con += m.attr(3) - 15;
					bound = 0;
				}
				if (con <= bound) {
					damageMember(m, 1);
				} else if (!(m.status() & 0x7F) && (m.u8(0x54) == 0 || m.u8(0x21) != 0)) {
					int hp = std::min<int>(m.hp() + 1, m.hpMax());
					m.setHp(hp);
					if (hp == 1) m.setHp(int16_t(m.u16(0x122)));
				}
			}
			if (m.hp() > 0) ++alive;
		}
		if (alive != 0) return true;
		if (partyWiped()) return false;
	}
	return true;
}

// ---------------------------------------------------------------- wandering enemies
bool World::isCellPassable(int x, int y) const {
	if (!map || !map->inBounds(x, y) || !map->cell(x, y).passable()) return false;
	for (const Monster &m : map->monsters())
		if (m.x() == x && m.y() == y) return false;
	for (const EnemyGroup &g : map->groups())
		if ((g.raw[9] & 0x0F) && g.x() == x && g.y() == y) return false;
	return true;
}

void World::markEntityOnMap(int radius, int x, int y, int overlay) {
	if (!map || !map->inBounds(x, y)) return;
	MapCell &c = map->cell(x, y);
	c.tile &= 0x7FF;
	if (std::abs(x - disk.mapX) <= radius && std::abs(y - disk.mapY) <= radius) c.tile = uint16_t(c.tile | (overlay << 11));
}

// KEH spawnEnemyGroup (0x174A0) + rollGroupHp.
void World::spawnEnemyGroup(EnemyGroup &g) {
	const auto &enemies = data.archetypes.enemies();
	g.raw[2] = g.raw[0]; g.raw[3] = g.raw[1];
	g.raw[5] = 1; g.raw[4] = 1; g.raw[0xA] = 0; g.raw[0xB] = 1;
	for (int j = 0; j < g.raw[8]; ++j) {
		uint8_t *slot = &g.raw[0xE + 0x16 * j];
		g.raw[9] |= uint8_t(1 << j);
		const EnemyDef &e = enemies.at(slot[0]);
		g.raw[5] = std::max(g.raw[5], e.raw[0x37]);
		g.raw[4] = std::max(g.raw[4], e.raw[0x36]);
		g.raw[0xB] = std::max(g.raw[0xB], e.raw[0x35]);
		slot[0xF] = slot[0xE];
		if (slot[0x12] < 100) {
			if (slot[0x12] < rng.range(1, 100)) slot[0xF] = 1;
			else slot[0xF] = uint8_t(rng.range(1, slot[0xE]));
		}
		const int hpLo = e.raw[0x30] | (e.raw[0x31] << 8), hpHi = e.raw[0x32] | (e.raw[0x33] << 8);
		for (int i = 0; i < slot[0xF]; ++i) {
			slot[0xA + i] = 0;
			int hp;
			if (g.raw[6] & 0x38) {   // hit points scale with the party's average
				int avg = 0;
				for (int k = 0; k < partyCount(); ++k) avg += party(k).hpMax();
				avg /= std::max(1, partyCount());
				const int delta = avg * g.raw[7] / 100;
				hp = rng.range(std::max(1, avg - delta), std::min(9999, avg + delta));
				slot[1] = uint8_t(std::clamp(hp / std::max(1, hpHi), 1, 99));
			} else {
				hp = rng.range(hpLo * slot[1], hpHi * slot[1]);
			}
			slot[2 + 2 * i] = uint8_t(hp); slot[3 + 2 * i] = uint8_t(hp >> 8);
		}
	}
	markEntityOnMap(g.raw[5], g.x(), g.y(), g.raw[0xC]);
}

// KEH tryRandomEncounter (0x1767B).
bool World::tryRandomEncounter() {
	if (!map || map->groupTemplates() == 0) return false;
	if (map->encounterPct() < rng.range(1, 100)) return false;
	static const int kDir[8][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
	int d = rng.range(0, 7);
	bool found = false;
	for (int tries = 0; tries < 8; ++tries, d = (d + 1) % 8)
		if (isCellPassable(disk.mapX + kDir[d][0], disk.mapY + kDir[d][1])) { found = true; break; }
	if (!found || map->groups().size() < 8) return false;
	auto &groups = map->groups();
	int pick = 0, best = groups[0].raw[0xD];
	for (int i = 0; i < 8; ++i)
		if (groups[size_t(i)].raw[0xD] < best) { best = groups[size_t(i)].raw[0xD]; pick = i; }
	EnemyGroup &inst = groups[size_t(pick)];
	if (inst.raw[0xD] != 0 && map->inBounds(inst.x(), inst.y())) map->cell(inst.x(), inst.y()).tile &= 0x7FF;
	const int tmpl = rng.range(8, map->groupTemplates() + 7);
	if (size_t(tmpl) >= groups.size()) return false;
	inst.raw = groups[size_t(tmpl)].raw;
	inst.raw[0] = uint8_t(disk.mapX + kDir[d][0]);
	inst.raw[1] = uint8_t(disk.mapY + kDir[d][1]);
	spawnEnemyGroup(inst);
	for (int i = 0; i < 8; ++i)
		if (groups[size_t(i)].raw[0xD] > 0) --groups[size_t(i)].raw[0xD];
	inst.raw[0xD] = 0xFF;
	return true;
}

int World::partyLevelUnit() const {
	int sum = 0;
	for (int i = 0; i < partyCount(); ++i)
		if (party(i).hp() > 0) sum += party(i).attr(2);
	return std::max(1, sum / 5);
}

static const int kStepTable[24][2] = {{0, -1}, {1, -1}, {-1, -1}, {0, 1}, {-1, 1}, {1, 1}, {1, 0}, {1, 1},
                                      {1, -1}, {1, 1}, {0, 1}, {1, 0}, {1, -1}, {1, 0}, {0, -1}, {-1, 0},
                                      {-1, -1}, {-1, 1}, {-1, 1}, {0, 1}, {-1, 0}, {-1, -1}, {0, -1}, {-1, 0}};

int World::directionTowards(int x, int y, int px, int py, bool toward) {
	const int ax = toward ? px : x, ay = toward ? py : y;
	const int bx = toward ? x : px, by = toward ? y : py;
	if (ax == bx) return ay < by ? 0 : 3;
	if (ax > bx) return ay > by ? 9 : (ay < by ? 12 : 6);
	return ay > by ? 18 : (ay < by ? 21 : 15);
}

bool World::findFreeCellNear(uint8_t &px, uint8_t &py, int x, int y) {
	const int dir = directionTowards(x, y, disk.mapX, disk.mapY, true);
	for (int i = 0; i < 3; ++i) {
		const int nx = x + kStepTable[dir + i][0], ny = y + kStepTable[dir + i][1];
		if (!isCellPassable(nx, ny)) continue;
		if (nx == disk.mapX && ny == disk.mapY) continue;
		px = uint8_t(nx); py = uint8_t(ny);
		return true;
	}
	return false;
}

void World::setMapCellHighBits(int x, int y, int v) {
	if (!map || !map->inBounds(x, y)) return;
	MapCell &c = map->cell(x, y);
	c.tile = uint16_t((c.tile & 0x7FF) | (v << 11));
}

void World::moveEntityStep(uint8_t &px, uint8_t &py, int reach, int detect, int toward, int overlay) {
	const int x = int8_t(px), y = int8_t(py);
	if (toward <= 0 || !map || x < 0 || x >= map->width()) return;
	auto within = [&](int r, int ex, int ey) { return std::abs(ex - disk.mapX) <= r && std::abs(ey - disk.mapY) <= r; };
	int dist = 1;
	while (dist < 64 && !within(dist, x, y)) ++dist;
	if (dist != 1 && within(reach, x, y)) findFreeCellNear(px, py, x, y);
	setMapCellHighBits(x, y, 0);
	if (within(detect, int8_t(px), int8_t(py))) setMapCellHighBits(int8_t(px), int8_t(py), overlay);
}

void World::moveMapEntities() {
	if (!map) return;
	const int unit = partyLevelUnit();
	const unsigned speedAttr = (data.hdspct.at(0x3E8) | (data.hdspct.at(0x3E9) << 8)) & 7;
	for (Monster &m : map->monsters()) {
		if (m.raw[0x5D] < 0x79 || m.raw[0x60] == 0xFF) continue;
		m.raw[0x64] = uint8_t(m.raw[0x64] + m.raw[0x18 + speedAttr]);
		while (m.raw[0x64] >= unit) {
			moveEntityStep(m.raw[0x5B], m.raw[0x5C], m.raw[0x5E], m.raw[0x5F], m.hp(), m.raw[0x65]);
			m.raw[0x64] = uint8_t(m.raw[0x64] - unit);
		}
	}
	for (EnemyGroup &g : map->groups()) {
		if (!(g.raw[9] & 0x0F) || (g.raw[9] & 0xF0)) continue;
		g.raw[0xA] = uint8_t(g.raw[0xA] + g.raw[0xB]);
		while (g.raw[0xA] >= unit) {
			moveEntityStep(g.raw[2], g.raw[3], g.raw[4], g.raw[5], 1, g.raw[0xC]);
			g.raw[0xA] = uint8_t(g.raw[0xA] - unit);
		}
	}
}

void World::placeEntitiesOnMap() {
	if (!map) return;
	for (Monster &m : map->monsters()) markEntityOnMap(m.raw[0x5F], m.x(), m.y(), m.raw[0x65]);
	for (EnemyGroup &g : map->groups())
		if (g.raw[9] & 0x0F) markEntityOnMap(g.raw[5], g.x(), g.y(), g.raw[0xC]);
}

bool World::addGroundItem(int item, int qty, int x, int y) {
	for (GroundItem &g : map->groundItems()) {
		if (g.item != item || g.x != x || g.y != y) continue;
		if (g.qty + qty > 0xFFFF) g.qty = 0xFFFF; else g.qty = uint16_t(g.qty + qty);
		return true;
	}
	return false;
}

void World::dropPacketLoot(int packet, int x, int y, int overrideFirstItem) {
	const Bytes &pk = data.packets;
	auto u16 = [&](size_t o) { return int(pk.at(o) | (pk.at(o + 1) << 8)); };
	const int count = u16(2 * size_t(packet)), first = u16(0x30 + 2 * size_t(packet));
	std::map<int, int> scratch;
	for (int i = first; i < first + count; ++i) {
		const size_t o = 0x60 + 6 * size_t(i);
		const int item = (i == first && overrideFirstItem >= 0) ? overrideFirstItem : pk.at(o);
		if (pk.at(o + 1) < rng.range(1, 100)) continue;
		const int qty = rng.range(u16(o + 2), u16(o + 4));
		if (!addGroundItem(item, qty, x, y)) scratch[item] = qty;
	}
	bool any = false;
	for (const auto &kv : scratch) {
		if (kv.second == 0) continue;
		GroundItem gi;
		gi.qty = uint16_t(kv.second); gi.x = uint8_t(x); gi.y = uint8_t(y); gi.item = uint8_t(kv.first);
		map->groundItems().push_back(gi);
		any = true;
	}
	if (any && map->inBounds(x, y)) map->cell(x, y).flags |= 0x80;
}

// ---------------------------------------------------------------- monsters / roster
Monster *World::findMonsterById(int id, size_t &index) {
	auto &ms = map->monsters();
	for (size_t i = 0; i < ms.size(); ++i)
		if (ms[i].id() == id) { index = i; return &ms[i]; }
	return nullptr;
}

int World::findFreeRosterSlot() const {
	for (int i = 0; i < 5; ++i)
		if (disk.roster[size_t(i)].b[0] == 0) return i;
	return 5;
}

void World::removeMonster(size_t index, bool dropLoot) {
	auto &ms = map->monsters();
	if (index >= ms.size()) return;
	Monster &m = ms[index];
	const int x = int8_t(m.raw[0x5B]), y = int8_t(m.raw[0x5C]);
	if (dropLoot) {
		std::map<int, int> loot;
		for (int k = 0; k < m.raw[0x52]; ++k) ++loot[m.raw[0x53 + k]];
		bool added = false;
		for (const auto &kv : loot) {
			if (addGroundItem(kv.first, kv.second, x, y)) continue;
			GroundItem gi;
			gi.qty = uint16_t(kv.second); gi.x = uint8_t(x); gi.y = uint8_t(y); gi.item = uint8_t(kv.first);
			map->groundItems().push_back(gi);
			added = true;
		}
		if (added && map->inBounds(x, y)) map->cell(x, y).flags |= 0x80;
	}
	if (map->inBounds(x, y)) map->cell(x, y).tile &= 0x07FF;
	ms.erase(ms.begin() + long(index));
}

static const uint32_t kRankExp[14] = {0, 1500, 1000, 1500, 4500, 6500, 9500, 11500, 23000, 32000, 44000, 57000, 89000, 118000};

bool World::addNpcToParty(size_t mi) {
	if (partyCount() >= 5) return false;
	const int slot = findFreeRosterSlot();
	if (slot >= 5 || mi >= map->monsters().size()) return false;
	Monster &mo = map->monsters()[mi];
	Member &m = disk.roster[size_t(slot)];
	m = Member{};
	std::memcpy(m.b.data(), mo.raw.data(), 0x54);
	disk.partyIds[disk.partyCount] = uint8_t(slot);
	m.b[0x52] = mo.raw[0x52];
	for (int k = 0; k < mo.raw[0x52]; ++k) initItemSlot(m, k, mo.raw[0x53 + k]);
	m.setU32(0x5A, 0);
	m.setU32(0x5E, m.b[0x4E] > 13 ? 200000u : kRankExp[m.b[0x4E]]);
	m.b[0x58] = m.b[0x53] = m.b[0x54] = m.b[0x21] = m.b[0x57] = m.b[0x56] = m.b[0x55] = 0;
	disk.cash += uint32_t(mo.raw[0x14] | (mo.raw[0x15] << 8) | (mo.raw[0x16] << 16) | (uint32_t(mo.raw[0x17]) << 24));
	++disk.partyCount;
	disk.tail[mo.id()] |= 1;
	removeMonster(mi, false);
	return true;
}

void World::removeMemberFromParty(int slot) {
	Member &m = party(slot);
	disk.tail[m.rosterId()] &= 0xFE;
	m.b[0] = 0;
	--disk.partyCount;
	for (int i = slot; i < disk.partyCount; ++i) disk.partyIds[i] = disk.partyIds[i + 1];
}

// ---------------------------------------------------------------- maps
void World::loadMap(int number) {
	if (map && mapNo > 0) mapCache.insert_or_assign(mapNo, *map);
	Archive scr(data.dir + "/" + std::string(mapFilePrefix(number)) + "scr", 28);
	scripts = std::make_unique<ScriptBank>(scr.chunkLzss(size_t(number - 1)));
	mapNo = number;
	loadChangesFromDisk();
	const auto cached = mapCache.find(number);
	if (cached != mapCache.end()) {
		map = std::make_unique<Map>(cached->second);
		return;
	}
	Archive maps(data.dir + "/" + std::string(mapFilePrefix(number)) + "map", 28);
	map = std::make_unique<Map>(maps.chunk(size_t(number - 1)));
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
	const auto cachedMap = mapCache.find(mapNumber);
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
	if (cachedMap != mapCache.end() && cachedMap->second.inBounds(c->x, c->y)) {
		MapCell &cell = cachedMap->second.cell(c->x, c->y);
		if (c->kind & 0x80) cell.tile = uint16_t((cell.tile & 0xF800) | (c->tile & 0x7FF));
		if (c->kind & 0x40) { cell.b3 = c->b3; cell.w4 = c->w4; cell.flags = uint8_t(c->f2 | (cell.flags & 0x80)); }
		if (c->kind & 0x20) cell.script = c->script;
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
