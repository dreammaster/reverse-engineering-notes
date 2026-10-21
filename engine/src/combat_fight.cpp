// The whole-fight loop of KEH.EXE (runCombat) and the helpers it drives. Names in comments are IDB names.
#include "fod/combat.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <map>

namespace fod {

namespace {
int16_t rd16(const uint8_t *p) { return int16_t(p[0] | (p[1] << 8)); }
void wr16(uint8_t *p, int v) { p[0] = uint8_t(v); p[1] = uint8_t(v >> 8); }
}

// ---------------------------------------------------------------- attack setup / multi shot
// KEH resetAttackState (mis-split at 0x13900): decides fire mode, shot count, blast radius and verb.
void Combat::setupAttack(const Combatant &c) {
	fireMode = 0; shots = 1; divisor = 1; calcOnly = false;
	_blast = 0; _attackerOverride.clear();
	const auto &profs = _w.data.archetypes.professions();
	const auto &enemies = _w.data.archetypes.enemies();
	auto profVerb = [&](int prof) {
		const Bytes &raw = profs.at(size_t(prof)).raw;
		const char *p = reinterpret_cast<const char *>(raw.data() + 0x58);
		return std::string(p, strnlen(p, 0x20));
	};
	int weapon = 0xFF, ammoType = 0, mode = 3, pct = 100;
	if (c.type == 0) {
		Member &m = _w.party(c.index);
		const uint8_t readied = m.b[0x4A];
		weapon = readied != 0xFF ? m.b[0x62 + 6 * readied] : 0xFF;
		_verb = profVerb(m.profession());
		fireMode = m.b[0x144];
		shots = std::max<int>(1, m.b[0x146]);
		mode = m.b[0x144];
		if (weapon != 0xFF) ammoType = m.b[0x63 + 6 * readied];
	} else if (c.type == 1) {
		const Monster &m = _w.map->monsters().at(c.index);
		const uint8_t readied = m.raw[0x4A];
		weapon = readied != 0xFF ? m.raw[0x53 + readied] : 0xFF;
		_verb = profVerb(m.raw[0x48]);
		const int maxHp = rd16(&m.raw[0x46]);
		pct = maxHp > 0 ? 100 * rd16(&m.raw[0x44]) / maxHp : 100;
	} else {
		const EnemyGroup &g = _w.map->groups().at(c.index);
		const uint8_t *slot = &g.raw[0xE + 0x16 * c.sub];
		weapon = slot[0x13];
		_verb = enemies.at(slot[0]).verb;
		pct = _w.rng.range(1, 50) * 10;
	}
	if (weapon == 0xFF) return;
	const WeaponDef &wd = _w.weaponOf(weapon);
	_verb = wd.verb();
	if (!(wd.flags() & 2) || wd.modes.empty()) return;
	const FireMode &fm = wd.modes.at(std::min<size_t>(size_t(ammoType), wd.modes.size() - 1));
	if (c.type != 0 && mode == 3 && fm.burst > 1) {
		if (pct < 50) { fireMode = 2; shots = _w.rng.range(1, std::max<int>(1, fm.clip)); }
		else { fireMode = 1; shots = fm.burst; }
	}
	_blast = fm.b11;
	if (fireMode == 1) _verb = "triggers a burst at";
	else if (fireMode == 2) _verb = "rips off a clip at";
}

// KEH attackMultiShot (0x13E7B): an enemy attacks a party member `rec.attacks` times; blast weapons splash the whole party.
void Combat::multiShot(const Combatant &rec) {
	if (rec.tType != 0) return;
	Member &target = _w.party(rec.tIndex);
	if (World::conState(target) == 5) return;
	for (int a = 0; a < rec.attacks; ++a) {
		setupAttack(rec);
		_hpPtr = &target.b[0x44];
		if (_blast) {
			calcOnly = true;
			const int chance = resolveAttack(rec);
			if (chance >= _w.rng.range(1, 100)) divisor = 2;
			calcOnly = false;
			const int saved = divisor;
			resolveAttack(rec);
			for (size_t j = 0; j < list.size(); ++j) {
				const Combatant &o = list[j];
				if (o.type != 0 || o.index == rec.tIndex) continue;
				Combatant r2 = rec;
				r2.tIndex = o.index;
				setupAttack(r2);
				_attackerOverride = "The blast";
				_verb = "also pounds";
				divisor = saved;
				_hpPtr = &_w.party(o.index).b[0x44];
				resolveAttack(r2);
			}
			_attackerOverride.clear();
		} else {
			resolveAttack(rec);
		}
	}
}

// KEH combatMoveEnemy (0x14A9A) + directionTowards: shift an enemy's engagement position one step toward/away from the party.
void Combat::combatMoveEnemy(Combatant &c) {
	static const int kDir[8][2] = {{0, -1}, {0, 1}, {1, 0}, {1, 1}, {1, -1}, {-1, 0}, {-1, 1}, {-1, -1}};
	auto direction = [&](int x, int y, bool toward) {
		const int ax = toward ? _w.disk.mapX : x, ay = toward ? _w.disk.mapY : y;
		const int bx = toward ? x : _w.disk.mapX, by = toward ? y : _w.disk.mapY;
		if (ax == bx) return ay < by ? 0 : 1;
		if (ax > bx) return ay > by ? 3 : (ay < by ? 4 : 2);
		return ay > by ? 6 : (ay < by ? 7 : 5);
	};
	int hp = 0;
	std::string name;
	if (c.type == 1) {
		Monster &m = _w.map->monsters().at(c.index);
		const int d = direction(int8_t(m.raw[0x60]), int8_t(m.raw[0x61]), c.tIndex != 0);
		m.raw[0x60] = uint8_t(m.raw[0x60] + kDir[d][0]);
		m.raw[0x61] = uint8_t(m.raw[0x61] + kDir[d][1]);
		name = m.name();
		hp = rd16(&m.raw[0x44]);
	} else if (c.type == 2) {
		EnemyGroup &g = _w.map->groups().at(c.index);
		uint8_t *slot = &g.raw[0xE + 0x16 * c.sub];
		const int d = direction(int8_t(slot[0x10]), int8_t(slot[0x11]), true);
		slot[0x10] = uint8_t(slot[0x10] + kDir[d][0]);
		slot[0x11] = uint8_t(slot[0x11] + kDir[d][1]);
		name = _w.data.archetypes.enemies().at(slot[0]).name;
		for (int i = 0; i < slot[0xF]; ++i) hp += rd16(slot + 2 + 2 * i);
	} else {
		return;
	}
	if (hp <= 0 || c.tSub == 0) return;
	_h.print(name + " moves " + (c.tType == 0 ? "to a better postion.\r\r" : "closer.\r\r"));
}

void Combat::autoCommand(int slot, Combatant &rec) {
	Member &m = _w.party(slot);
	rec.command = 0x0E;
	for (const Combatant &o : list) {
		if (o.type == 0) continue;
		int x, y, hp;
		if (combatantPos(o, x, y, hp) && hp > 0) {
			rec.tType = o.type; rec.tIndex = o.index; rec.tSub = o.sub;
			rec.command = 4;
			m.b[0x144] = 0; m.b[0x145] = m.b[0x4A]; m.b[0x146] = 1;
			return;
		}
	}
}

// ---------------------------------------------------------------- the fight
void Combat::rechooseEnemyActions() {
	for (size_t i = 0; i < list.size(); ++i) {
		const Combatant &c = list[i];
		if (c.type == 1) {
			const Monster &m = _w.map->monsters().at(c.index);
			const uint8_t readied = m.raw[0x4A];
			const int item = readied == 0xFF ? 0xFF : m.raw[std::min<size_t>(0x53 + 2 * size_t(readied), m.raw.size() - 1)];
			chooseEnemyAction(i, item, rd16(&m.raw[0x44]), rd16(&m.raw[0x46]), m.raw[0x5D], int8_t(m.raw[0x60]), int8_t(m.raw[0x61]));
		} else if (c.type == 2) {
			const uint8_t *slot = &_w.map->groups().at(c.index).raw[0xE + 0x16 * c.sub];
			chooseEnemyAction(i, slot[0x13], 1, 100, 0x79, int8_t(slot[0x10]), int8_t(slot[0x11]));
		}
	}
}

bool Combat::tryFlee() {
	int unit = 0;
	for (int i = 0; i < _w.partyCount(); ++i)
		if (_w.party(i).hp() > 0) unit += _w.party(i).attr(2);
	unit = std::max(1, unit / 5);
	bool success;
	if (unit == int(maxSpeed)) success = _w.rng.range(1, 100) <= 50;
	else if (unit > int(maxSpeed)) success = true;
	else {
		int chance = 0;
		for (int i = 0; i < _w.partyCount(); ++i)
			if (_w.isActive(_w.party(i))) chance += World::attrBonus(_w.party(i).attr(6)) * 3;
		success = chance > 0 && chance >= _w.rng.range(1, 100);
	}
	_h.print(std::string("You ") + (success ? "succeed" : "fail") + " in your attempt to run away.\r\r");
	return success;
}

void Combat::executeCombatant(size_t idx, bool &fled) {
	static const char *kPoss[4] = {"its", "his", "her", "its"};
	Combatant c = list[idx];
	Member *m = c.type == 0 ? &_w.party(c.index) : nullptr;
	const bool active = m && _w.isActive(*m);
	switch (c.command) {
	case 0:
		for (Combatant &o : list) {
			if (o.type == 0) continue;
			o.command = 1; o.tType = 1; o.tIndex = 1; o.tSub = 0;
			combatMoveEnemy(o);
		}
		_h.print(_w.partyCount() > 1 ? "Your party moves closer.\r\r" : "You move closer.\r\r");
		rechooseEnemyActions();
		break;
	case 1:
		if (m) { if (active) _h.print(m->name() + " evades.\r\r"); }
		else combatMoveEnemy(list[idx]);
		break;
	case 4:
		if (m) {
			m->b[0x22] = 0;
			memberAttack(c.index, c);
			if (m->b[0x22]) _h.skillRaised(m->name(), m->b[0x23], m->b[0x24 + m->b[0x23]]);
		} else if (c.type == 1) {
			Monster &mo = _w.map->monsters().at(c.index);
			if (rd16(&mo.raw[0x44]) > 0 && (mo.raw[0x50] & 4)) {
				_h.print(mo.name() + " reloads.\r\r");
				mo.raw[0x50] &= 0xFB;
			} else if (rd16(&mo.raw[0x44]) > 0) {
				multiShot(c);
				setupAttack(c);
				if (fireMode == 2) mo.raw[0x50] |= 4;
			}
		} else {
			EnemyGroup &g = _w.map->groups().at(c.index);
			uint8_t *slot = &g.raw[0xE + 0x16 * c.sub];
			for (int i = 0; i < slot[0xF]; ++i) {
				const int hp = rd16(slot + 2 + 2 * i);
				if (hp > 0 && (slot[0xA + i] & 1)) {
					_h.print(_w.data.archetypes.enemies().at(slot[0]).name + " reloads.\r\r");
					slot[0xA + i] &= 0xFE;
				} else if (hp > 0) {
					multiShot(c);
					setupAttack(c);
					if (fireMode == 2) slot[0xA + i] |= 1;
				}
			}
		}
		break;
	case 5:
		if (!active) break;
		_h.print(m->name() + " reloads.\r\r");
		{
			const int src = m->b[0x145], dst = m->b[0x4A];
			m->b[0x64 + 6 * dst] = m->b[0x64 + 6 * src];
			m->b[0x63 + 6 * dst] = m->b[0x63 + 6 * src];
			_w.removeItemSlot(*m, src, 0xFF);
		}
		break;
	case 6:
		if (!active) break;
		{
			const int slotIdx = m->b[0x145];
			const std::string itemName = _w.item(m->b[0x62 + 6 * slotIdx]).name;
			const std::string poss = kPoss[m->u8(0x50) & 3];
			if (std::min(100, 20 * m->b[0x3E]) >= _w.rng.range(1, 100)) {
				m->b[0x65 + 6 * slotIdx] &= 0xFE;
				_h.print(m->name() + " successfully unjams " + poss + " " + itemName + ".\r\r");
				if (trySkillImprove(c.index, 0x1A)) _h.skillRaised(m->name(), 0x1A, m->b[0x24 + 0x1A]);
			} else {
				_h.print(m->name() + " trashes " + poss + " " + itemName + " while trying to fix it.\r\r");
				_w.removeItemSlot(*m, slotIdx, 0xFF);
			}
		}
		break;
	case 7:
		if (!active) break;
		_h.print(m->name() + " switches.\r\r");
		m->b[0x4A] = m->b[0x145];
		break;
	case 2:
		if (!active) break;
		if (m->activeSkill(0xD) * 15 >= _w.rng.range(1, 100)) {
			for (size_t j = idx + 1; j < list.size(); ++j)
				if (list[j].type != 0) list[j].command = 0x0E;
			_h.print(m->name() + " shrieks at the enemy!\r\r");
		} else {
			_h.print(m->name() + " shrieks but no one notices!\r\r");
		}
		break;
	case 3:
		if (!active) break;
		selectTarget(c);
		if (targetHpValue() <= 0) break;
		if (m->activeSkill(0xE) * 15 >= _w.rng.range(1, 100)) {
			for (size_t j = idx + 1; j < list.size(); ++j)
				if (list[j].type == c.tType && list[j].index == c.tIndex && list[j].sub == c.tSub) list[j].command = 0x0E;
			_h.print(m->name() + " reaches out and stuns " + targetName(c) + "!\r\r");
		} else {
			_h.print(m->name() + " fails in an attempt to stun " + targetName(c) + "!\r\r");
		}
		break;
	case 13:
		if (!active) break;
		_h.print(m->name() + " drops " + kPoss[m->u8(0x50) & 3] + " " + _w.item(m->b[0x62 + 6 * m->b[0x145]]).name + ".\r\r");
		_w.removeItemSlot(*m, m->b[0x145], 0xFF);
		break;
	case 15:
		fled = tryFlee();
		break;
	default:   // 8/9 doctor, 10/11 use item, 12 trade: need the party screens (not ported yet)
		break;
	}
}

void Combat::removeDeadMonsters() {
	auto &monsters = _w.map->monsters();
	for (size_t i = 0; i < monsters.size();) {
		if (rd16(&monsters[i].raw[0x44]) >= 1) { ++i; continue; }
		_w.removeMonster(i, true);
	}
}

// KEH postCombatCleanup (0x136E0): squeeze dead members out of group slots; empty slots leave the fight.
void Combat::postCombatCleanup() {
	for (EnemyGroup &g : _w.map->groups()) {
		if (!(g.raw[9] & 0x0F)) continue;
		std::vector<int> packets, weapons;
		for (int j = 0; j < g.raw[8]; ++j) {
			if (!(g.raw[9] & (1 << j))) continue;
			uint8_t *slot = &g.raw[0xE + 0x16 * j];
			int kept = 0;
			for (int i = 0; i < slot[0xF]; ++i) {
				const int hp = rd16(slot + 2 + 2 * i);
				if (hp <= 0) continue;
				wr16(slot + 2 + 2 * kept, hp);
				slot[0xA + kept] = slot[0xA + i];
				++kept;
			}
			slot[0xF] = uint8_t(kept);
			if (kept == 0) {
				g.raw[9] &= uint8_t(~(1 << j));
				if (slot[0x15]) packets.push_back(slot[0x15] - 1);
				if (slot[0x13] != 0xFF) weapons.push_back(slot[0x13]);
			}
		}
		for (int w : weapons) _w.dropPacketLoot(23, g.x(), g.y(), w);
		for (int p : packets) _w.dropPacketLoot(p, g.x(), g.y());
		if (!(g.raw[9] & 0x0F) && _w.map->inBounds(g.x(), g.y())) _w.map->cell(g.x(), g.y()).tile &= 0x07FF;
	}
}

void Combat::awardExperience() {
	for (int i = 0; i < _w.partyCount(); ++i) {
		Member &m = _w.party(i);
		const int32_t xp = int32_t(xpAccum[i]);
		if (m.hp() <= -60 || xp == 0) continue;
		if (xp > 0) {
			_h.print(m.name() + " gains " + std::to_string(xp) + " experience points.\r\r");
			m.setU32(0x5A, m.exp() + uint32_t(xp));
		} else {
			_h.print(m.name() + " loses " + std::to_string(-xp) + " experience points for killing innocent bystanders.\r\r");
			m.setU32(0x5E, m.expNext() + uint32_t(-xp));
		}
	}
}

Combat::Outcome Combat::run(bool useMaxRange) {
	Outcome out;
	std::fill(std::begin(xpAccum), std::end(xpAccum), 0u);
	if (list.empty()) return out;
	_h.print("Encounter begins...\r\r");
	for (;;) {
		// command phase: enemy entries are already in list; append one per party member
		GroupAction ga = GroupAction::None;
		std::vector<Combatant> party;
		for (int slot = 0; slot < _w.partyCount(); ++slot) {
			Member &m = _w.party(slot);
			Combatant rec;
			rec.type = 0; rec.index = uint8_t(slot); rec.command = 0x0E;
			rec.speed = uint8_t(m.attr(2) * 2);
			if (_w.isActive(m)) {
				if (commands) ga = commands(*this, slot, rec);
				else autoCommand(slot, rec);
			}
			if (ga != GroupAction::None) break;
			party.push_back(rec);
		}
		if (ga != GroupAction::None) {
			party.clear();
			Combatant rec;
			rec.type = 0; rec.index = 0; rec.speed = 0xFF;
			rec.command = ga == GroupAction::GetCloser ? 0 : 0x0F;
			party.push_back(rec);
		}
		list.insert(list.end(), party.begin(), party.end());
		sortCombatants();

		bool fled = false;
		for (size_t i = 0; i < list.size() && !fled; ++i) {
			executeCombatant(i, fled);
			if (_w.partyWiped()) { out.wiped = true; return out; }
		}
		if (!_w.passTime(true)) { out.wiped = true; return out; }
		removeDeadMonsters();
		postCombatCleanup();
		_w.moveMapEntities();
		_w.placeEntitiesOnMap();
		const int n = buildCombatants(useMaxRange);
		if (fled) { out.fled = true; break; }
		if (n == 0) break;
	}
	awardExperience();
	for (EnemyGroup &g : _w.map->groups()) g.raw[9] &= 0x0F;
	for (Monster &m : _w.map->monsters()) {   // KEH tickMonsterStates
		uint8_t &st = m.raw[0x5D];
		if (st <= 0x3C) continue;
		if (st < 0x51) st = uint8_t(st - 0x3C);
		else if (st < 0x79) st = uint8_t(st + 0x3C);
	}
	return out;
}

} // namespace fod
